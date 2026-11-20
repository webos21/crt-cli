#!/usr/bin/env python3
"""Fresh native Windows D3 CI replay using public SDK inputs and pinned test-host helpers.

Restricted tokens deny selected original inputs; job objects account for child images.
This is not network or executable denial. No existing ACL, account or machine policy is changed.
"""
import argparse
import importlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from ci_bootstrap import WINDOWS_IMPORT_ARCH
from ci_integration import host_key, verify_download, verify_oracle, windows_harness_pins
from replay_port_linux import digest, inventory, source_inputs, suite_outcome


PROBE = r'''#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--symlink")) {
        FILE *f = fopen("symlink-target", "wb"); if (!f) return 20;
        fputs("real-link", f); fclose(f);
        if (symlink("symlink-target", "symlink-alias")) { perror("real symlink requires Developer Mode"); return 21; }
        struct stat st;
        if (lstat("symlink-alias", &st) || !S_ISLNK(st.st_mode)) return 22;
        f = fopen("symlink-alias", "rb"); if (!f) return 23;
        fclose(f); unlink("symlink-alias"); unlink("symlink-target"); return 0;
    }
    for (int i = 1; i < argc; ++i) {
        FILE *f = fopen(argv[i], "rb");
        if (f) { fclose(f); fprintf(stderr, "original input remains readable: %s\n", argv[i]); return 10; }
    }
    return 0;
}
'''


def load_harness(directory):
    """Authenticate public helper bytes before import, including direct adapter invocation."""
    pins = Path(__file__).resolve().parent.parent / '.github/ci/integration-pins.json'
    directory = Path(directory).resolve(strict=True)
    for pin in windows_harness_pins(pins):
        verify_download(pin['name'], directory / pin['name'], pin)
    sys.path.insert(0, str(directory))
    return importlib.import_module('crt_windows_external'), importlib.import_module('crt_windows_trace')


def copy_source(source, target):
    for name in ('src', 'include'):
        for path in (source / name).rglob('*'):
            if path.is_file():
                destination = target / path.relative_to(source)
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(path.read_bytes().replace(b'\r\n', b'\n'))
    (target / 'crt.json').write_bytes((source / 'crt.json').read_bytes().replace(b'\r\n', b'\n'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'sdk-root', 'contract', 'host-tools', 'archive', 'seed', 'output', 'work-parent',
                 'cc', 'cxx', 'linker', 'ar', 'os-sdk', 'windows-harness', 'oracle-python'):
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ('contract-sha256', 'host-tools-sha256', 'oracle-python-sha256'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--cases', nargs='+', required=True)
    args = parser.parse_args()
    if sys.platform != 'win32':
        parser.error('native Windows is required')
    host = host_key()
    arch = WINDOWS_IMPORT_ARCH[host]
    pe_machine = {'x64': 'IMAGE_FILE_MACHINE_AMD64', 'arm64': 'IMAGE_FILE_MACHINE_ARM64'}[arch]
    verify_oracle(str(args.oracle_python), args.oracle_python_sha256)
    if digest(args.contract) != args.contract_sha256 or digest(args.host_tools / 'manifest.json') != args.host_tools_sha256:
        parser.error('contract or host-tools manifest differs from its pin')
    sandbox, trace = load_harness(args.windows_harness)
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=False)
    work, sid = sandbox.create_work(args.work_parent / 'w')
    project, sdk, bundle = work / 'source', work / 'SDK space/dev', work / 'host-tools'
    project.mkdir(); copy_source(args.source, project)
    shutil.copytree(args.sdk_root, sdk, symlinks=True)
    shutil.copytree(args.host_tools, bundle, symlinks=True)
    contract, seed, archive = work / 'native-toolchain-v1.json', work / 'seed-cli.exe', work / args.archive.name
    for original, copied in ((args.contract, contract), (args.seed, seed), (args.archive, archive)):
        shutil.copy2(original, copied)
    for name in ('home', 'tmp'): (work / name).mkdir()
    oracle = work / 'oracle-python'
    shutil.copytree(args.oracle_python.resolve().parent, oracle, symlinks=True,
                    ignore=shutil.ignore_patterns('Doc', 'Tools', 'test', 'tests', '__pycache__', 'site-packages', 'idlelib', 'tkinter', 'turtledemo'))
    copied_python = oracle / args.oracle_python.name
    if digest(copied_python) != args.oracle_python_sha256:
        raise ValueError('copied oracle Python differs from its pin')
    tools = {name: getattr(args, name).resolve(strict=True) for name in ('cc', 'cxx', 'linker', 'ar')}
    helpers = {name: (tools['cc'].parent / name).resolve(strict=True) for name in ('llvm-ranlib.exe', 'lld.exe', 'llvm-readobj.exe')}
    declared = set(tools.values()) | {helpers['llvm-ranlib.exe'], helpers['lld.exe']}
    console = Path(os.environ['SystemRoot']) / 'System32/conhost.exe'
    declared |= {seed, console}
    generated = [project / 'out/native', work / 'probe-project/out/native', work / 'tmp', work / 'home/.crt/ports', bundle]
    hidden = [args.source.resolve() / 'crt.json', args.sdk_root.resolve() / 'manifest.json',
              args.host_tools.resolve() / 'manifest.json', args.contract.resolve(), args.seed.resolve()]
    for path in args.hide:
        path = path.resolve(strict=True)
        hidden.append(next(p for p in path.rglob('*') if p.is_file()) if path.is_dir() else path)
    if any(not path.is_file() or path.is_relative_to(work) for path in hidden):
        raise ValueError('visibility probes require existing original input files outside the native workspace')
    before = {'sdk': inventory(sdk), 'host_tools': inventory(bundle)}
    inputs = {'source': source_inputs(project), 'sdk': before['sdk'], 'host_tools': before['host_tools'],
              'contract_sha256': args.contract_sha256, 'archive_sha256': digest(archive), 'seed_sha256': digest(seed),
              'external_tools': {role: {'path': str(path), 'sha256': digest(path)} for role, path in tools.items()},
              'external_helpers': {name: {'path': str(path), 'sha256': digest(path)} for name, path in helpers.items()},
              'os_sdk': str(args.os_sdk), 'oracle_python': {'path': str(args.oracle_python.resolve()),
                  'sha256': args.oracle_python_sha256, 'copied_path': str(copied_python), 'scope': 'port-oracle-test only'},
              'driver_sha256': digest(Path(__file__)), 'work': str(work), 'hidden': list(map(str, hidden))}
    (output / 'inputs.json').write_text(json.dumps(inputs, indent=2) + '\n', encoding='utf-8')
    env = {'PATH': '', 'HOME': str(work / 'home'), 'USERPROFILE': str(work / 'home'),
           'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'TMPDIR': str(work / 'tmp'),
           'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C'}
    commands, cases, suites = [], [], {}
    bare, bare_sid = None, None
    result = {'passed': False, 'allow_dirty_sdk': False, 'host': host, 'cases': cases, 'suites': suites,
              'host_tools_sha256': args.host_tools_sha256, 'archive_sha256': digest(archive),
              'contract_sha256': args.contract_sha256, 'work': str(work),
              'isolation': 'restricted token, empty PATH, job-object process accounting; no network/exec denial'}

    def run(label, argv, *, oracle_allowed=False, extra_env=None, timeout=1800, bare_mode=False):
        child_env = dict(env, **(extra_env or {}))
        if bare_mode:
            child_env = {'PATH': '', 'HOME': str(bare / 'home'), 'USERPROFILE': str(bare / 'home'),
                         'TEMP': str(bare / 'tmp'), 'TMP': str(bare / 'tmp'), 'SystemRoot': os.environ['SystemRoot']}
        status = trace.run(list(map(str, argv)), work=bare if bare_mode else work, sid=bare_sid if bare_mode else sid,
                           env=child_env, cwd=bare if bare_mode else project,
                           stdout=output / (label + '.stdout'), stderr=output / (label + '.stderr'), timeout=timeout)
        def allowed(image):
            if not image: return False
            path = Path(image).resolve()
            if bare_mode:
                return path in {bare / 'crt-cli.exe', bare / 'probe.exe', console}
            return path in declared or any(path.is_relative_to(p) for p in generated) or (
                oracle_allowed and any(path.is_relative_to(p) for p in (oracle, sdk)))
        status['undeclared_images'] = [p for p in status['processes'] if not allowed(p['image'])]
        commands.append({'case': label, 'argv': list(map(str, argv)), 'oracle_host_tools_allowed': oracle_allowed, **status})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        stdout = (output / (label + '.stdout')).read_text(encoding='utf-8', errors='replace')
        stderr = (output / (label + '.stderr')).read_text(encoding='utf-8', errors='replace')
        return status, stdout, stderr

    def record(label, passed, detail=''):
        cases.append({'case': label, 'passed': bool(passed), 'detail': str(detail)[-1600:]})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)

    def require_run(label, argv, **kwargs):
        status, stdout, stderr = run(label, argv, **kwargs)
        passed = status['exit_code'] == 0 and not status['timed_out'] and not status['undeclared_images']
        record(label, passed, stderr or status)
        if not passed: raise ValueError(label + ' failed: ' + (stderr[-1000:] or str(status)))
        return stdout

    def configure(target):
        argv = [seed, '--json', 'configure', '--project', target, '--sdk-root', sdk, '--sdk-contract', contract,
                '--contract-sha256', args.contract_sha256, '--os-sdk', args.os_sdk]
        for role, path in tools.items(): argv += ['--' + role, path]
        require_run('configure-' + target.name, argv)

    try:
        probe = work / 'probe-project'; probe.mkdir()
        manifest = {'manifest_format': 0, 'package': {'name': 'visibility-probe', 'version': '0.1.0'},
                    'sdk': {'profile': 'dev', 'version': '*', 'target': 'native', 'storage': 'shared-immutable'},
                    'include_dirs': [], 'application': {'name': 'visibility-probe', 'sources': ['probe.c']}, 'tests': []}
        (probe / 'crt.json').write_text(json.dumps(manifest), encoding='utf-8')
        (probe / 'probe.c').write_text(PROBE, encoding='utf-8')
        configure(probe)
        require_run('build-probe', [seed, '--json', 'build', '--project', probe])
        probe_exe = probe / 'out/native/visibility-probe.exe'
        require_run('original-inputs-hidden', [probe_exe, *hidden])
        require_run('real-symlink-prerequisite', [probe_exe, '--symlink'])
        configure(project)
        cold = json.loads(require_run('build-cli', [seed, '--json', 'build', '--project', project]))
        if cold['data']['cache']['executed'] <= 0 or cold['data']['cache']['reused'] != 0:
            raise ValueError('CLI build did not execute a fresh source export')
        cli = project / 'out/native/crt-cli.exe'; declared.add(cli)
        warm = json.loads(require_run('warm-build', [cli, '--json', 'build', '--project', project]))
        record('warm build reuses every source', warm['data']['cache']['executed'] == 0 and
               warm['data']['cache']['reused'] == cold['data']['cache']['executed'])
        bare, bare_sid = sandbox.create_work(args.work_parent / 'bare')
        for name in ('home', 'tmp'): (bare / name).mkdir()
        shutil.copy2(cli, bare / 'crt-cli.exe'); shutil.copy2(probe_exe, bare / 'probe.exe')
        require_run('bare-original-and-build-inputs-hidden', [bare / 'probe.exe', *hidden,
                    project / 'crt.json', sdk / 'manifest.json', contract], bare_mode=True)
        for label, tail in [('bare-help', ['--help']), ('bare-version', ['--version']), ('bare-doctor', ['doctor', '--bootstrap'])]:
            reply = json.loads(require_run(label, [bare / 'crt-cli.exe', '--json', *tail], bare_mode=True))
            record(label + ' reports success', reply.get('ok') is True)
        build_env = {'CRT_CLI_TEST_PORT_PROJECT': str(project), 'CRT_CLI_TEST_PORT_HOST_TOOLS': str(bundle),
                     'CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256': args.host_tools_sha256, 'CRT_CLI_TEST_PORT_ARCHIVE': str(archive)}
        for suite in args.cases:
            require_run('build-' + suite, [cli, '--json', 'build', '--project', project, '--target', suite])
            oracle_allowed = suite == 'port-oracle-test'
            extra_env = dict(build_env)
            if oracle_allowed: extra_env['CRT_CLI_TEST_PORT_PYTHON'] = str(copied_python)
            status, stdout, stderr = run(suite, [project / ('out/native/' + suite + '.exe')],
                                         oracle_allowed=oracle_allowed, extra_env=extra_env)
            outcome = suite_outcome(suite, status['exit_code'], stdout, stderr)
            outcome['passed'] = outcome['passed'] and not status['timed_out'] and not status['undeclared_images']
            suites[suite] = outcome
            record(suite, outcome['passed'], stdout + '\n' + stderr)
        require_run('port-build-command', [cli, '--json', 'port', 'build', 'zlib', '--project', project,
                    '--archive', archive, '--host-tools', bundle, '--host-tools-sha256', args.host_tools_sha256])
        audit = subprocess.run([str(helpers['llvm-readobj.exe']), '--file-headers', '--coff-imports', str(cli)],
                               capture_output=True, text=True, encoding='utf-8', check=True, timeout=60)
        (output / 'cli-pe.txt').write_text(audit.stdout, encoding='utf-8')
        import re
        imports = re.findall(r'^\s*Name: (\S+\.dll)\s*$', audit.stdout, re.M | re.I)
        record('PE %s and only declared OS imports' % arch, pe_machine in audit.stdout and bool(imports) and
               set(name.lower() for name in imports) <= {'kernel32.dll', 'api-ms-win-core-synch-l1-2-0.dll'}, imports)
        result['cache'] = {'cold': cold['data']['cache'], 'warm': warm['data']['cache']}
        result['cli_sha256'] = digest(cli)
    except Exception as error:
        record('complete native replay', False, str(error))
    finally:
        unchanged = inventory(sdk) == before['sdk'] and inventory(bundle) == before['host_tools']
        unchanged = unchanged and inventory(args.sdk_root) == before['sdk'] and inventory(args.host_tools) == before['host_tools']
        record('original and copied SDK/tool inventories unchanged', unchanged)
        record('source inputs unchanged', source_inputs(project) == inputs['source'])
        result['passed'] = bool(cases) and all(case['passed'] for case in cases) and set(suites) == set(args.cases) and all(
            suite['passed'] for suite in suites.values())
        (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
