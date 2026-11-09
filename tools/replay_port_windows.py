#!/usr/bin/env python3
"""Replay the D3 suites in a retained Windows contract workspace; never a product dependency.

This deliberately returns failure for unsupported D3 suites. A successful bootstrap or a
skipped environment-gated test must not be reported as Windows source-port acceptance.
--sid is the unregistered build SID of the disposable contract workspace, not an account.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(root):
    return sorted([p.relative_to(root).as_posix(), 'link', os.readlink(p)] if p.is_symlink() else
                  [p.relative_to(root).as_posix(), 'file', digest(p)]
                  for p in root.rglob('*') if p.is_symlink() or p.is_file())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('contract-evidence', 'source', 'crt-tools', 'host-tools', 'archive', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--host-tools-sha256', required=True)
    parser.add_argument('--sid', required=True)
    parser.add_argument('--cases', nargs='+', choices=('port-recipe-test', 'port-build-test', 'port-store-test',
                        'port-project-test', 'port-command-test', 'port-oracle-test'))
    parser.add_argument('--hide', type=Path, action='append', required=True)
    args = parser.parse_args()
    driver_bytes = Path(__file__).read_bytes()
    assert os.name == 'nt' and re.fullmatch(r'S-1-5-21-(\d+-){3}\d+', args.sid)
    initial = json.loads((args.contract_evidence / 'inputs.json').read_bytes())
    previous_commands = json.loads((args.contract_evidence / 'commands.json').read_bytes())
    for label in ('provider', 'probe-build', 'build-visibility', 'seed-link', 'configure',
                  'build', 'self-build', 'cached-build'):
        assert any(c['label'] == label and c.get('exit') == 0 for c in previous_commands), label
    work = Path(initial['work']).resolve(strict=True)
    project, sdk = work / 'source', work / 'SDK space/dev'
    cli = work / 'bare/crt-cli.exe'
    # A failed aggregate may already have passed its long native/archive/store suites.
    # Preserve that receipt and synchronize only test fixtures, never product/SDK inputs.
    product = lambda name: name == 'crt.json' or name.startswith(('include/', 'src/main/'))
    for name, value in initial['source_inventory'].items():
        if product(name):
            assert value == 'file:' + digest(project / name)
            assert value == 'file:' + hashlib.sha256((args.source / name).read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    fixture_changes = []
    for path in (args.source / 'src/test').rglob('*'):
        if not path.is_file():
            continue
        target = project / path.relative_to(args.source)
        data = path.read_bytes().replace(b'\r\n', b'\n')
        if not target.is_file() or target.read_bytes() != data:
            fixture_changes.append({'path': path.relative_to(args.source).as_posix(),
                                    'before': digest(target) if target.is_file() else None,
                                    'after': hashlib.sha256(data).hexdigest()})
            target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(data)
    assert digest(args.host_tools / 'manifest.json') == args.host_tools_sha256
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    (output / 'driver.py').write_bytes(driver_bytes)
    bundle, archive = work / 'd3-host-tools', work / args.archive.name
    if bundle.exists():
        assert inventory(bundle) == inventory(args.host_tools)
    else:
        shutil.copytree(args.host_tools, bundle, symlinks=True)
    if archive.exists():
        assert digest(archive) == digest(args.archive)
    else:
        shutil.copy2(args.archive, archive)
    recipe = json.loads((sdk / 'porting/recipes/zlib.json').read_bytes())
    assert digest(archive) == recipe['source']['sha256']
    before = {'sdk': inventory(sdk), 'host_tools': inventory(bundle)}
    sys.path.insert(0, str(args.crt_tools.resolve()))
    import crt_windows_trace as trace
    import crt_windows_external as sandbox
    import crt_pe
    from test_dev_sdk_external import inventory as sdk_inventory, inventory_digest
    assert inventory_digest(sdk_inventory(sdk)) == initial['sdk_sha256'], 'SDK changed after contract replay'
    hidden = [p.resolve(strict=True) for p in args.hide]
    assert all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden)
    environment = {'PATH': '', 'HOME': str(work / 'home'), 'USERPROFILE': str(work / 'home'),
                   'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'TMPDIR': str(work / 'tmp'),
                   'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C',
                   'CRT_CLI_TEST_PORT_PROJECT': str(project), 'CRT_CLI_TEST_PORT_HOST_TOOLS': str(bundle),
                   'CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256': args.host_tools_sha256,
                   'CRT_CLI_TEST_PORT_ARCHIVE': str(archive), 'CRT_CLI_TEST_PORT_PYTHON': sys.executable}
    commands, results = [], []
    local = json.loads((project / '.crt/local.json').read_bytes())
    declared_tools = {Path(local[name]).resolve() for name in ('cc', 'cxx', 'linker', 'ar')}
    declared_tools |= {p.parent / name for p in declared_tools
                       for name in ('lld.exe', 'llvm-ranlib.exe')}
    generated = [project / 'out/native', work / 'tmp', bundle]
    system_console = Path(os.environ['SystemRoot'], 'System32/conhost.exe').resolve()

    def run(label, argv):
        status = trace.run(list(map(str, argv)), work=work, sid=args.sid, env=environment, cwd=project,
                           stdout=output / (label + '.stdout'), stderr=output / (label + '.stderr'), timeout=1800)
        def declared(image):
            if not image:
                return False
            path = Path(image).resolve()
            return (path in declared_tools or path in {cli.resolve(), (work / 'probe.exe').resolve(), system_console}
                    or any(path.is_relative_to(root.resolve()) for root in generated))
        status['undeclared_images'] = [p for p in status['processes'] if not declared(p['image'])]
        commands.append({'case': label, 'argv': list(map(str, argv)), **status})
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2), encoding='utf-8')
        return status, (output / (label + '.stdout')).read_text(encoding='utf-8', errors='replace')

    argv = [work / 'probe.exe']
    for path in hidden:
        argv += ['--read', path]
    status, _ = run('boundary', argv)
    assert status['exit_code'] == 0 and not status['timed_out'] and not status['undeclared_images'], \
        'the specified SID does not enforce the boundary'
    suites = ('port-recipe-test', 'port-build-test', 'port-store-test', 'port-project-test',
              'port-command-test', 'port-oracle-test')
    for suite in args.cases or suites:
        built, _ = run('build-' + suite, [cli, '--json', 'build', '--project', project, '--target', suite])
        if built['exit_code'] or built['timed_out'] or built['undeclared_images']:
            results.append({'suite': suite, 'passed': False, 'phase': 'build', 'exit_code': built['exit_code']})
            print('FAIL build ' + suite, flush=True)
            continue
        executable = project / 'out/native' / (suite + '.exe')
        assert executable.is_file()
        status, stdout = run(suite, [executable])
        matched = re.search(r'(\d+) checks passed', stdout)
        passed = (status['exit_code'] == 0 and not status['timed_out'] and not status['undeclared_images']
                  and matched and 'skipped' not in stdout)
        results.append({'suite': suite, 'passed': bool(passed),
                        'checks': int(matched[1]) if matched else None, 'exit_code': status['exit_code']})
        print(('PASS ' if passed else 'FAIL ') + suite, flush=True)
    status, _ = run('port-build-command', [cli, '--json', 'port', 'build', 'zlib', '--project', project,
                    '--archive', archive, '--host-tools', bundle, '--host-tools-sha256', args.host_tools_sha256])
    results.append({'suite': 'port-build-command', 'passed': status['exit_code'] == 0 and not status['timed_out']
                    and not status['undeclared_images'],
                    'exit_code': status['exit_code']})
    unchanged = (inventory(sdk) == before['sdk'] and inventory(bundle) == before['host_tools']
                 and inventory(args.host_tools) == before['host_tools'])
    # Complete the bare checks even when the original aggregate stopped at a fixture compile error.
    bare, bare_sid = sandbox.create_work()
    bare_checks = run_bare_checks(sandbox, trace, crt_pe, bare, bare_sid, cli, work, output, hidden,
                                  project, sdk, system_console)
    summary = {'passed': all(r['passed'] for r in results) and unchanged, 'suites': results, 'work': str(work),
               'contract_inputs_sha256': digest(args.contract_evidence / 'inputs.json'),
               'contract_commands_sha256': digest(args.contract_evidence / 'commands.json'),
               'cli_sha256': digest(cli), 'test_fixture_changes': fixture_changes,
               'driver_sha256': hashlib.sha256(driver_bytes).hexdigest(),
               'bare_work': str(bare), 'bare_checks': bare_checks,
               'host_tools_sha256': args.host_tools_sha256, 'archive_sha256': digest(archive),
               'inputs_unchanged': unchanged, 'hidden': list(map(str, hidden)),
               'isolation': 'restricted token, empty PATH, job-object process accounting; no network/exec denial claim'}
    summary['passed'] = summary['passed'] and all(c['passed'] for c in bare_checks)
    (output / 'inputs.json').write_text(json.dumps(before, ensure_ascii=False, indent=2), encoding='utf-8')
    (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    return 0 if summary['passed'] else 1


def run_bare_checks(sandbox, trace, crt_pe, bare, bare_sid, cli, work, output, hidden, project, sdk, system_console):
    for name in ('home', 'tmp'):
        (bare / name).mkdir()
    for name in ('crt-cli.exe', 'probe.exe'):
        shutil.copy2(cli if name == 'crt-cli.exe' else work / name, bare / name)
    bare_env = {'PATH': '', 'HOME': str(bare / 'home'), 'USERPROFILE': str(bare / 'home'),
                'TEMP': str(bare / 'tmp'), 'TMP': str(bare / 'tmp'), 'SystemRoot': os.environ['SystemRoot']}
    bare_checks = []
    boundary = [bare / 'probe.exe']
    for path in [*hidden, project, sdk, work / 'native-toolchain-v1.json']:
        boundary += ['--read', path]
    for label, argv in [('visibility', boundary), ('help', [bare / 'crt-cli.exe', '--json', '--help']),
                        ('version', [bare / 'crt-cli.exe', '--json', '--version']),
                        ('doctor', [bare / 'crt-cli.exe', '--json', 'doctor', '--bootstrap'])]:
        status = trace.run(list(map(str, argv)), work=bare, sid=bare_sid, env=bare_env, cwd=bare,
                           stdout=output / ('bare-' + label + '.stdout'), stderr=output / ('bare-' + label + '.stderr'), timeout=60)
        stray = [p for p in status['processes'] if not p['image'] or
                 Path(p['image']).resolve() not in {system_console, (bare / 'crt-cli.exe').resolve(), (bare / 'probe.exe').resolve()}]
        bare_checks.append({'case': label, 'passed': status['exit_code'] == 0 and not status['timed_out'] and not stray,
                            **status})
    imports = sorted(x.lower() for x in crt_pe.imported_libraries(cli))
    bare_checks.append({'case': 'imports', 'passed': set(imports) <= {'kernel32.dll', 'api-ms-win-core-synch-l1-2-0.dll'},
                        'imports': imports})
    return bare_checks


if __name__ == '__main__':
    raise SystemExit(main())
