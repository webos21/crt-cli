#!/usr/bin/env python3
"""Native Linux D1 consumer replay. Python/openssl/bwrap/strace are test-host tools only.

The CLI and CRT tools execute with source/SDK/compiler paths hidden, host shells
and build drivers masked, no network, a read-only tool bundle and explicit data.
The copied working set is retained outside the repositories for inspection.
"""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--host-tools', type=Path, required=True)
    parser.add_argument('--resources', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work-parent', type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='d1-native-', dir=args.work_parent.resolve()))
    tools, package = work / 'tools space 한글', work / 'bootstrap space 한글'
    shutil.copytree(args.host_tools, tools, symlinks=True)
    shutil.copytree(args.resources, package, symlinks=True)
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    assert digest(cli) == digest(package / 'crt-cli'), 'resource package needs the selected executable'
    sources = work / 'scripts space 한글'; sources.mkdir()
    (work / 'home').mkdir(); (work / 'traces').mkdir()
    masks = set()
    for directory in ('/usr/bin', '/usr/local/bin', '/bin', '/usr/sbin', '/sbin'):
        if not Path(directory).is_dir(): continue
        for path in Path(directory).iterdir():
            if path.is_file() and (path.name in {'sh', 'bash', 'dash', 'zsh', 'fish', 'mksh', 'ksh', 'awk', 'gawk', 'mawk', 'make', 'gmake',
                                                   'cmake', 'ctest', 'ninja', 'perl', 'ruby', 'cc', 'c++', 'gcc', 'g++', 'clang', 'clang++', 'ld.lld', 'llvm-ar'}
                                   or path.name.startswith(('python', 'pypy'))): masks.add(path.resolve())
    bwrap, strace = shutil.which('bwrap'), shutil.which('strace')
    assert bwrap and strace, 'bwrap and strace are required'
    hidden = [p.resolve() for p in args.hide]
    assert all(not work.is_relative_to(p) for p in hidden), 'work must be outside hidden inputs'
    results, commands = [], []
    before = digest(tools / 'manifest.json')
    def run(label, tail, expected=0, check=None, binary=cli):
        argv = [bwrap, '--unshare-all', '--die-with-parent', '--new-session', '--ro-bind', '/', '/',
                '--proc', '/proc', '--dev', '/dev', '--tmpfs', '/tmp', '--clearenv',
                '--setenv', 'HOME', str(work / 'home'), '--setenv', 'PATH', '/hostile/sentinel',
                '--setenv', 'LC_ALL', 'C', '--setenv', 'CPATH', 'secret-poison', '--setenv', 'CRT_CLI_TEST_SECRET', 'secret-poison',
                '--setenv', 'MAKEFLAGS', 'SHELL=/hostile/sentinel', '--setenv', 'CRT_ROOTFS', '/hostile/rootfs']
        for path in hidden:
            argv += ['--tmpfs', str(path)] if path.is_dir() else ['--ro-bind', '/dev/null', str(path)]
        argv += ['--bind', str(work), str(work), '--ro-bind', str(tools), str(tools), '--ro-bind', str(package), str(package), '--chdir', str(work)]
        for path in sorted(masks): argv += ['--ro-bind', '/dev/null', str(path)]
        trace = work / 'traces' / (label + '.log')
        argv += [strace, '--seccomp-bpf', '-ff', '-s', '4096', '-qq', '-e', 'trace=execve', '-o', str(trace), '--', str(binary), '--json', *map(str, tail)]
        result = subprocess.run(argv, capture_output=True, stdin=subprocess.DEVNULL, timeout=90)
        (output / (label + '.stdout')).write_bytes(result.stdout)
        (output / (label + '.stderr')).write_bytes(result.stderr)
        reply = json.loads(result.stdout)
        okay = (result.returncode == expected if isinstance(expected, int) else result.returncode != 0)
        if check: okay &= bool(check(reply))
        summary = json.loads(json.dumps(reply))
        if 'stdout' in summary.get('data', {}):
            text = summary['data'].pop('stdout')
            summary['data']['stdout_bytes'] = len(text.encode('utf-8'))
            summary['data']['stdout_sha256'] = hashlib.sha256(text.encode('utf-8')).hexdigest()
        results.append({'label': label, 'pass': okay, 'exit': result.returncode, 'reply': summary})
        commands.append({'label': label, 'argv': argv})
        print(('PASS ' if okay else 'FAIL ') + label, flush=True)
        return reply
    pin = digest(tools / 'manifest.json')
    selection = ['--host-tools', tools, '--host-tools-sha256', pin]
    def shell(label, text, expected=0, timeout=20, check=None):
        script = sources / (label + '.sh'); script.write_text(text, encoding='utf-8')
        return run(label, ['shell', *selection, '--work-root', work / ('job space 한글 ' + label), '--script', script,
                           '--directory', sources, '--timeout', str(timeout)], expected, check)
    run('self-doctor', ['doctor'], check=lambda r: r['data']['scope'] == 'self')
    run('environment-doctor', ['doctor', '--scope', 'environment', *selection])
    env_job = work / 'diagnostic only'
    run('environment-report', ['env', '--profile', 'port-build', *selection, '--work-root', env_job],
        check=lambda r: 'CRT_SYSROOT' not in r['data']['fixed'] and r['data']['interactive'] is False and not env_job.exists())
    shell('tools-workflow', '''set -eu
test "${CRT_CLI_TEST_SECRET-unset}" = unset
test "${CPATH-unset}" = unset
test "${CRT_SYSROOT-unset}" = unset
test "$SHELL" = /system/bin/sh
test "$PATH" = '@TOOLS_PATH@'
printf '%s\\n' "$PATH" | grep '/system/bin$'
value=$(printf '20 22\\n' | awk '{print $1+$2}')
test "$value" = 42
cat <<'MAKEFILE' > Makefile
all:
\t@test "$(SHELL)" = /system/bin/sh
\t@printf '%s\\n' crt-make-shell
\t@$(MAKE) -f nested.mk
MAKEFILE
cat <<'MAKEFILE' > nested.mk
all:
\t@printf '%s\\n' recursive-make-ok
MAKEFILE
make -f Makefile
printf '%s\\n' alias-test > input.txt
cp input.txt copied.txt
cmp input.txt copied.txt
rm copied.txt
printf '%s\\n' tools-ok
'''.replace('@TOOLS_PATH@', str(tools / 'system/bin')), check=lambda r: 'recursive-make-ok' in r['data']['stdout'] and 'tools-ok' in r['data']['stdout'])
    # Both endings must stop a child that is already running, without name-based kills.
    for label, ending, expected in [('normal-cleanup', 'exit 0', 0), ('timeout-cleanup', 'wait', 124)]:
        marker = sources / (label + '.marker'); started = sources / (label + '.started')
        shell(label, f'''set -eu
(printf started > '{started}'; sleep 3; printf escaped > '{marker}') &
while test ! -f '{started}'; do sleep 0.1; done
{ending}
''', expected, timeout=1 if expected else 20)
        time.sleep(4)
        results.append({'label': label + '-marker', 'pass': started.exists() and not marker.exists()})
    shell('child-exit', 'exit 7\n', expected=7)
    shell('stdout-limit', 'dd if=/dev/zero bs=65536 count=160\n', check=lambda r: r['data']['output_truncated'] and len(r['data']['stdout']) == 8 * 1024 * 1024)
    old_job = work / 'job space 한글 child-exit'
    run('job-reuse-rejected', ['shell', *selection, '--work-root', old_job, '--script', sources / 'child-exit.sh'], expected=None)
    run('wrong-pin', ['doctor', '--scope', 'environment', '--host-tools', tools, '--host-tools-sha256', '0' * 64], expected=None)
    link = work / 'tools-parent-link'; link.symlink_to(tools, target_is_directory=True)
    run('linked-immutable-parent', ['env', '--profile', 'port-build', *selection, '--work-root', link / 'unsafe-job'], expected=None)
    victim = tools / 'system/bin/awk'; original = victim.read_bytes()
    victim.write_bytes(original + b'pollution')
    run('modified-tool', ['doctor', '--scope', 'environment', *selection], expected=None); victim.write_bytes(original)
    victim = tools / 'system/bin/printf'; mode = victim.stat().st_mode; victim.chmod(0o644)
    run('nonexecutable-alias', ['doctor', '--scope', 'environment', *selection], expected=None); victim.chmod(mode)
    extra = tools / 'extra'; extra.write_text('unexpected')
    run('unexpected-payload', ['doctor', '--scope', 'environment', *selection], expected=None); extra.unlink()
    missing = tools / 'system/bin/make'; saved = work / 'saved-make'; missing.rename(saved)
    run('missing-make-no-fallback', ['doctor', '--scope', 'environment', *selection], expected=None); saved.rename(missing)
    manifest = tools / 'manifest.json'; original = manifest.read_bytes(); changed = json.loads(original)
    changed['host']['arch'] = 'aarch64'; manifest.write_text(json.dumps(changed))
    run('wrong-execution-host', ['doctor', '--scope', 'environment', '--host-tools', tools, '--host-tools-sha256', digest(manifest)], expected=None)
    manifest.write_bytes(original)
    # Resource signature authenticates this exact local test package, not an official release publisher.
    openssl = shutil.which('openssl'); assert openssl
    def crypto(*args): subprocess.run([openssl, *map(str, args)], cwd=work, check=True, capture_output=True)
    crypto('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', 'test.key')
    crypto('pkey', '-in', 'test.key', '-pubout', '-out', 'test.pub')
    crypto('dgst', '-sha256', '-sign', 'test.key', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32',
           '-out', 'resources.sig', package / 'manifest.json')
    resources = ['doctor', '--scope', 'self', '--resources', package, '--resources-sha256', digest(package / 'manifest.json')]
    run('resource-pin', resources, binary=package / 'crt-cli', check=lambda r: r['data']['checks']['resources'] == 'pass')
    signed = [*resources, '--publisher-key', work / 'test.pub', '--signature', work / 'resources.sig']
    run('resource-signature', signed, binary=package / 'crt-cli', check=lambda r: r['data']['resources']['publisher_authentication'] == 'signature_verified')
    signature = work / 'resources.sig'; original_signature = signature.read_bytes(); signature.write_bytes(b'\0' * 256)
    run('resource-signature-invalid', signed, binary=package / 'crt-cli', expected=None); signature.write_bytes(original_signature)
    crypto('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', 'other.key')
    crypto('pkey', '-in', 'other.key', '-pubout', '-out', 'other.pub')
    wrong_key = [*resources, '--publisher-key', work / 'other.pub', '--signature', work / 'resources.sig']
    run('resource-wrong-key', wrong_key, expected=None)
    run('resource-missing-signature', [*resources, '--publisher-key', work / 'test.pub'], expected=None)
    resource_manifest = package / 'manifest.json'; original_resource_manifest = resource_manifest.read_bytes()
    for label, field, value in [('resource-wrong-host', 'target', {'os': 'linux', 'arch': 'aarch64'}), ('resource-unknown-schema', 'format', 99)]:
        data = json.loads(original_resource_manifest); data[field] = value; resource_manifest.write_text(json.dumps(data))
        run(label, ['doctor', '--resources', package, '--resources-sha256', digest(resource_manifest)], expected=None)
    resource_manifest.write_bytes(original_resource_manifest)
    notice = package / 'LICENSE.md'; saved_notice = notice.read_bytes(); notice.write_bytes(saved_notice + b'changed')
    run('resource-tampered-notice', resources, binary=package / 'crt-cli', expected=None); notice.write_bytes(saved_notice)
    notice.unlink()
    run('resource-missing-notice', resources, binary=package / 'crt-cli', expected=None); notice.write_bytes(saved_notice)
    binary = package / 'crt-cli'; saved_binary = binary.read_bytes(); binary.write_bytes(saved_binary + b'changed')
    run('resource-tampered-binary', resources, expected=None); binary.write_bytes(saved_binary)
    ca = package / 'share/trust/ca.pem'
    if ca.exists():
        old = ca.read_bytes(); ca.write_bytes(old + b'changed')
        run('resource-tampered-ca', resources, expected=None); ca.write_bytes(old)
    executed = set()
    for trace in (work / 'traces').iterdir():
        shutil.copy2(trace, output / trace.name)
        for line in trace.read_text().splitlines():
            match = re.search(r'execve\("([^"]+)"', line)
            if match and re.search(r'= 0$', line): executed.add(ast.literal_eval('b"' + match.group(1) + '"').decode('utf-8'))
    unexpected = [p for p in executed if not (Path(p).resolve().is_relative_to(tools) or Path(p).resolve() in {cli, package / 'crt-cli'})]
    results.append({'label': 'execve-closure', 'pass': not unexpected, 'unexpected': unexpected, 'executed': sorted(executed)})
    results.append({'label': 'immutable-manifest', 'pass': digest(tools / 'manifest.json') == before})
    receipt = {'pass': all(r['pass'] for r in results), 'cases': results, 'work': str(work), 'cli_sha256': digest(cli),
               'host_tools_manifest_sha256': pin, 'resources_manifest_sha256': digest(package / 'manifest.json'),
               'hidden': list(map(str, hidden)), 'masked': list(map(str, sorted(masks))), 'publisher': 'disposable explicit test key'}
    (output / 'result.json').write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + '\n')
    (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n')
    print('PASS' if receipt['pass'] else 'FAILED', len(results), 'cases', work, flush=True)
    raise SystemExit(0 if receipt['pass'] else 1)


if __name__ == '__main__':
    main()
