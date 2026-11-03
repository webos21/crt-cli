#!/usr/bin/env python3
"""Native macOS D1 replay; Python, Clang and OpenSSL are test-host tools only.

Seatbelt denies original source/SDK/toolchain access and permits execution only
of the selected CLI and CRT host tools. Evidence and work copies are retained.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def policy_text(hidden, executables, readonly):
    quote = lambda p: json.dumps(str(p), ensure_ascii=False)
    text = '(version 1)\n(allow default)\n(deny network*)\n'
    for path in hidden:
        text += '(deny file-read* file-write* (subpath ' + quote(path) + '))\n'
    for path in readonly:
        text += '(deny file-write* (subpath ' + quote(path) + '))\n'
    terms = ' '.join('(literal ' + quote(p) + ')' for p in sorted(set(executables)))
    return text + '(deny process-exec (require-not (require-any ' + terms + ')))\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cli', 'host-tools', 'resources', 'output', 'work-parent'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('requires native macOS Seatbelt')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='d1-macos-', dir=args.work_parent.resolve()))
    tools, package = work / 'tools space 한글', work / 'bootstrap space 한글'
    shutil.copytree(args.host_tools, tools, symlinks=True)
    shutil.copytree(args.resources, package, symlinks=True)
    cli = work / 'crt-cli'
    shutil.copy2(args.cli, cli)
    assert digest(cli) == digest(package / 'crt-cli'), 'package must contain the selected CLI'
    hidden = sorted({p.resolve(strict=True) for p in args.hide})
    assert hidden and all(not work.is_relative_to(p) for p in hidden), 'work must be outside hidden inputs'
    sources = work / 'scripts space 한글'
    sources.mkdir()
    (work / 'home').mkdir()
    before = (tools / 'manifest.json').read_bytes()
    manifest = json.loads(before)
    assert manifest['host'] == {'os': 'macos', 'arch': 'aarch64'}
    permitted = [cli, package / 'crt-cli', *{(tools / p).resolve() for p in manifest['executable_files']}]
    probe = work / 'boundary-probe'
    source = work / 'boundary-probe.c'
    source.write_text('''#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
int main(int argc, char **argv) {
  for (int i = 1; i + 1 < argc; i += 2) {
    if (!strcmp(argv[i], "--read")) {
      int fd = open(argv[i + 1], O_RDONLY);
      if (fd >= 0 || (errno != EPERM && errno != EACCES)) return 10;
    } else {
      pid_t pid = fork(); int status;
      if (pid < 0) return 11;
      if (!pid) {
        char *args[] = {argv[i + 1], 0}; execv(args[0], args);
        _exit(errno == EPERM || errno == EACCES ? 0 : 12);
      }
      if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status)) return 13;
    }
  }
  puts("boundary probe: denied"); return 0;
}
''')
    subprocess.run(['/usr/bin/xcrun', 'clang', str(source), '-o', str(probe)], check=True, capture_output=True)
    policy = work / 'consumer.sb'
    policy.write_text(policy_text(hidden, [*permitted, probe], [tools, package]), encoding='utf-8')
    environment = {'HOME': str(work / 'home'), 'PATH': '/hostile/sentinel', 'LC_ALL': 'C',
                   'CPATH': 'secret-poison', 'CRT_CLI_TEST_SECRET': 'secret-poison',
                   'MAKEFLAGS': 'SHELL=/hostile/sentinel', 'CRT_ROOTFS': '/hostile/rootfs',
                   'DYLD_LIBRARY_PATH': '/hostile/dylibs', 'DYLD_FALLBACK_LIBRARY_PATH': '/hostile/fallback'}
    commands, results = [], []

    def save(completed=False):
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n')
        receipt = {'pass': completed and all(r['pass'] for r in results), 'completed': completed,
                   'cases': results, 'work': str(work),
                   'cli_sha256': digest(cli), 'host_tools_manifest_sha256': digest(tools / 'manifest.json'),
                   'resources_manifest_sha256': digest(package / 'manifest.json'), 'hidden': list(map(str, hidden)),
                   'permitted_executables': list(map(str, permitted)), 'policy_sha256': digest(policy),
                   'publisher': 'disposable explicit test key', 'isolation': 'macOS Seatbelt, not an execve trace'}
        (output / 'result.json').write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + '\n')

    boundary = [str(probe)]
    for path in hidden:
        boundary += ['--read', str(path)]
    forbidden = [Path('/bin/sh'), Path('/bin/bash'), Path('/bin/zsh'), Path('/usr/bin/make'),
                 Path('/usr/bin/awk'), Path(sys.executable).resolve(), Path('/usr/bin/xcrun')]
    for name in ('cmake', 'ninja'):
        found = shutil.which(name)
        assert found, 'test-host prerequisite missing: ' + name
        forbidden.append(Path(found).resolve())
    for path in forbidden:
        boundary += ['--exec', str(path)]
    result = subprocess.run(['/usr/bin/sandbox-exec', '-f', str(policy), *boundary], env=environment,
                            capture_output=True, stdin=subprocess.DEVNULL, timeout=30)
    commands.append({'label': 'boundary-probe', 'argv': boundary, 'exit': result.returncode})
    results.append({'label': 'boundary-probe', 'pass': result.returncode == 0})
    (output / 'boundary-probe.stdout').write_bytes(result.stdout)
    (output / 'boundary-probe.stderr').write_bytes(result.stderr)
    shutil.copy2(policy, output / 'consumer.sb')
    save()
    assert result.returncode == 0, 'Seatbelt boundary probe failed'

    def run(label, tail, expected=0, check=None, binary=cli):
        argv = ['/usr/bin/sandbox-exec', '-f', str(policy), str(binary), '--json', *map(str, tail)]
        result = subprocess.run(argv, cwd=work, env=environment, capture_output=True,
                                stdin=subprocess.DEVNULL, timeout=90)
        (output / (label + '.stdout')).write_bytes(result.stdout)
        (output / (label + '.stderr')).write_bytes(result.stderr)
        reply = json.loads(result.stdout)
        okay = result.returncode == expected if expected is not None else result.returncode != 0
        if check:
            okay = okay and bool(check(reply))
        summary = json.loads(json.dumps(reply))
        if 'stdout' in summary.get('data', {}):
            text = summary['data'].pop('stdout')
            summary['data']['stdout_bytes'] = len(text.encode('utf-8'))
            summary['data']['stdout_sha256'] = hashlib.sha256(text.encode('utf-8')).hexdigest()
        commands.append({'label': label, 'argv': argv, 'cwd': str(work), 'exit': result.returncode})
        results.append({'label': label, 'pass': okay, 'exit': result.returncode, 'reply': summary})
        print(('PASS ' if okay else 'FAIL ') + label, flush=True)
        save()
        return reply

    selection = ['--host-tools', tools, '--host-tools-sha256', digest(tools / 'manifest.json')]
    def shell(label, text, expected=0, timeout=20, check=None):
        script = sources / (label + '.sh')
        script.write_text(text, encoding='utf-8')
        return run(label, ['shell', *selection, '--work-root', work / ('job space 한글 ' + label),
                           '--script', script, '--directory', sources, '--timeout', str(timeout)], expected, check)

    run('self-doctor', ['doctor'], check=lambda r: r['data']['scope'] == 'self')
    run('environment-doctor', ['doctor', '--scope', 'environment', *selection])
    diagnostic = work / 'diagnostic only'
    run('environment-report', ['env', '--profile', 'port-build', *selection, '--work-root', diagnostic],
        check=lambda r: 'CRT_SYSROOT' not in r['data']['fixed'] and not r['data']['interactive'] and not diagnostic.exists())
    shell('tools-workflow', '''set -eu
test "${CRT_CLI_TEST_SECRET-unset}" = unset
test "${CPATH-unset}" = unset
test "${CRT_SYSROOT-unset}" = unset
test "${DYLD_LIBRARY_PATH-unset}" = unset
test "${DYLD_FALLBACK_LIBRARY_PATH-unset}" = unset
test "$SHELL" = /system/bin/sh
test "$PATH" = '@TOOLS_PATH@'
test "$PWD" = '@SOURCE_PATH@'
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
'''.replace('@TOOLS_PATH@', str(tools / 'system/bin')).replace('@SOURCE_PATH@', str(sources)),
          check=lambda r: 'recursive-make-ok' in r['data']['stdout'] and 'tools-ok' in r['data']['stdout'])
    for label, ending, expected in [('normal-cleanup', 'exit 0', 0), ('timeout-cleanup', 'wait', 124)]:
        marker, started = sources / (label + '.marker'), sources / (label + '.started')
        shell(label, f"set -eu\n(printf started > '{started}'; sleep 3; printf escaped > '{marker}') &\n"
              f"while test ! -f '{started}'; do sleep 0.1; done\n{ending}\n", expected, 1 if expected else 20)
        time.sleep(4)
        results.append({'label': label + '-marker', 'pass': started.exists() and not marker.exists()})
    shell('child-exit', 'exit 7\n', 7)
    shell('stdout-limit', 'dd if=/dev/zero bs=65536 count=160\n', timeout=60,
          check=lambda r: r['data']['output_truncated'] and len(r['data']['stdout']) == 8 * 1024 * 1024)
    run('job-reuse-rejected', ['shell', *selection, '--work-root', work / 'job space 한글 child-exit',
                             '--script', sources / 'child-exit.sh'], expected=None)
    run('wrong-pin', ['doctor', '--scope', 'environment', '--host-tools', tools,
                      '--host-tools-sha256', '0' * 64], expected=None)
    link = work / 'tools-parent-link'
    link.symlink_to(tools, target_is_directory=True)
    run('linked-immutable-parent', ['env', '--profile', 'port-build', *selection, '--work-root', link / 'unsafe'], expected=None)
    victim = tools / 'system/bin/awk'
    original = victim.read_bytes()
    victim.write_bytes(original + b'pollution')
    run('modified-tool', ['doctor', '--scope', 'environment', *selection], expected=None)
    victim.write_bytes(original)
    victim = tools / 'system/bin/printf'
    mode = victim.stat().st_mode
    victim.chmod(0o644)
    run('nonexecutable-alias', ['doctor', '--scope', 'environment', *selection], expected=None)
    victim.chmod(mode)
    extra = tools / 'extra'
    extra.write_text('unexpected')
    run('unexpected-payload', ['doctor', '--scope', 'environment', *selection], expected=None)
    extra.unlink()
    missing, saved = tools / 'system/bin/make', work / 'saved-make'
    missing.rename(saved)
    run('missing-make-no-fallback', ['doctor', '--scope', 'environment', *selection], expected=None)
    saved.rename(missing)
    changed = json.loads(before)
    changed['host']['arch'] = 'x86_64'
    (tools / 'manifest.json').write_text(json.dumps(changed))
    run('wrong-execution-host', ['doctor', '--scope', 'environment', '--host-tools', tools,
                               '--host-tools-sha256', digest(tools / 'manifest.json')], expected=None)
    (tools / 'manifest.json').write_bytes(before)
    openssl = shutil.which('openssl')
    assert openssl, 'OpenSSL is a test-host prerequisite'
    def crypto(*args):
        subprocess.run([openssl, *map(str, args)], cwd=work, check=True, capture_output=True)
    crypto('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', 'test.key')
    crypto('pkey', '-in', 'test.key', '-pubout', '-out', 'test.pub')
    crypto('dgst', '-sha256', '-sign', 'test.key', '-sigopt', 'rsa_padding_mode:pss',
           '-sigopt', 'rsa_pss_saltlen:32', '-out', 'resources.sig', package / 'manifest.json')
    resources = ['doctor', '--scope', 'self', '--resources', package, '--resources-sha256', digest(package / 'manifest.json')]
    run('resource-pin', resources, binary=package / 'crt-cli', check=lambda r: r['data']['checks']['resources'] == 'pass')
    signed = [*resources, '--publisher-key', work / 'test.pub', '--signature', work / 'resources.sig']
    run('resource-signature', signed, binary=package / 'crt-cli',
        check=lambda r: r['data']['resources']['publisher_authentication'] == 'signature_verified')
    signature = work / 'resources.sig'
    old_signature = signature.read_bytes()
    signature.write_bytes(b'\0' * 256)
    run('resource-signature-invalid', signed, expected=None)
    signature.write_bytes(old_signature)
    crypto('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', 'other.key')
    crypto('pkey', '-in', 'other.key', '-pubout', '-out', 'other.pub')
    run('resource-wrong-key', [*resources, '--publisher-key', work / 'other.pub', '--signature', signature], expected=None)
    run('resource-missing-signature', [*resources, '--publisher-key', work / 'test.pub'], expected=None)
    resource_manifest = package / 'manifest.json'
    old_manifest = resource_manifest.read_bytes()
    for label, field, value in [('resource-wrong-host', 'target', {'os': 'linux', 'arch': 'x86_64'}),
                                ('resource-unknown-schema', 'format', 99)]:
        data = json.loads(old_manifest)
        data[field] = value
        resource_manifest.write_text(json.dumps(data))
        run(label, ['doctor', '--resources', package, '--resources-sha256', digest(resource_manifest)], expected=None)
    resource_manifest.write_bytes(old_manifest)
    for label, path in [('notice', package / 'LICENSE.md'), ('binary', package / 'crt-cli'), ('ca', package / 'share/trust/ca.pem')]:
        original = path.read_bytes()
        path.write_bytes(original + b'changed')
        run('resource-tampered-' + label, resources, expected=None)
        path.write_bytes(original)
        if label == 'notice':
            path.unlink()
            run('resource-missing-notice', resources, expected=None)
            path.write_bytes(original)
    results.append({'label': 'immutable-manifest', 'pass': (tools / 'manifest.json').read_bytes() == before})
    save(completed=True)
    print('PASS' if all(r['pass'] for r in results) else 'FAILED', len(results), 'cases', work, flush=True)
    raise SystemExit(0 if all(r['pass'] for r in results) else 1)


if __name__ == '__main__':
    main()
