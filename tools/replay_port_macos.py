#!/usr/bin/env python3
"""D3 native macOS replay; Python, Seatbelt and the oracle are test-host tools only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from replay_macos_sandbox import MacSandbox


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inventory(root):
    rows = []
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            rows.append([str(path.relative_to(root)), 'symlink', os.readlink(path)])
        elif path.is_file():
            rows.append([str(path.relative_to(root)), 'file', digest(path)])
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'sdk-root', 'contract', 'host-tools', 'archive', 'seed', 'output', 'work-parent',
                 'cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--contract-sha256', required=True)
    parser.add_argument('--host-tools-sha256', required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--resume-from', type=Path, help='reuse a retained workspace with identical product and SDK inputs')
    suites = ('port-recipe-test', 'archive-test', 'sdk-store-test', 'port-build-test', 'port-store-test',
              'port-project-test', 'port-command-test', 'port-oracle-test', 'native-test')
    parser.add_argument('--cases', nargs='+', choices=suites, help='run only these suites; omitted means all nine')
    args = parser.parse_args()
    assert sys.platform == 'darwin'
    assert digest(args.contract) == args.contract_sha256
    assert digest(args.host_tools / 'manifest.json') == args.host_tools_sha256
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    # SDKROOT/archiver strings in zlib's Makefile still require a single shell word. Jobs and projects exercise spaces.
    previous = json.loads(args.resume_from.read_bytes()) if args.resume_from else None
    work = Path(previous['work']) if previous else Path(tempfile.mkdtemp(prefix='d3-macos-', dir=args.work_parent.resolve()))
    assert work.parent == args.work_parent.resolve() and work.name.startswith('d3-macos-')
    assert not any(c.isspace() for c in str(work))
    sdk, bundle, contract = work / 'sdk', work / 'host-tools', work / 'provider.json'
    archive, seed, scratch = work / args.archive.name, work / 'seed-cli', work / 'scratch'
    project = scratch / 'cli source 한글'
    if previous:
        old_inputs = json.loads((args.resume_from.parent / 'inputs.json').read_bytes())
        product = lambda rows: [row for row in rows if row[0] == 'crt.json' or row[0].startswith(('include/', 'src/main/'))]
        assert product(inventory(args.source)) == product(old_inputs['cli_source']) == product(inventory(project))
        assert inventory(sdk) == old_inputs['sdk'] == inventory(args.sdk_root)
        assert inventory(bundle) == old_inputs['host_tools'] == inventory(args.host_tools)
        assert digest(contract) == args.contract_sha256 == previous['contract_sha256']
        assert digest(seed) == digest(args.seed) == old_inputs['seed_sha256']
        assert digest(archive) == digest(args.archive) == previous['archive_sha256']
        # Synchronize only test fixtures, never product files or their pinned SDK.
        shutil.copytree(args.source / 'src/test', project / 'src/test', dirs_exist_ok=True)
    else:
        shutil.copytree(args.sdk_root, sdk, symlinks=True)
        shutil.copytree(args.host_tools, bundle, symlinks=True)
        shutil.copy2(args.contract, contract); shutil.copy2(args.archive, archive); shutil.copy2(args.seed, seed)
        scratch.mkdir()
        shutil.copytree(args.source, project, symlinks=True,
                        ignore=shutil.ignore_patterns('.git', '.crt', 'out', '__pycache__', 'crt.lock', '.sdk.json'))
    before = inventory(sdk)
    inputs = {'cli_source': inventory(project), 'sdk': before, 'host_tools': inventory(bundle),
              'contract_sha256': digest(contract), 'archive_sha256': digest(archive), 'seed_sha256': digest(seed)}
    (output / 'inputs.json').write_text(json.dumps(inputs, ensure_ascii=False, indent=2) + '\n')
    tools = {name: getattr(args, name).absolute() for name in ('cc', 'cxx', 'linker', 'ar')}
    home = scratch / 'home'; home.mkdir(exist_ok=True)
    setuid_item = scratch / 'setuid-item'
    (setuid_item / 'lib').mkdir(parents=True, exist_ok=True)
    setuid_file = setuid_item / 'lib/libz.1.3.1.dylib'
    shutil.copy2(sdk / 'lib/libz.1.3.1.dylib', setuid_file)
    setuid_file.chmod(0o4755)
    for name, target in [('libz.1.dylib', 'libz.1.3.1.dylib'), ('libz.dylib', 'libz.1.dylib')]:
        alias = setuid_file.parent / name
        if not alias.is_symlink(): alias.symlink_to(target)
    assert setuid_file.stat().st_mode & 0o4000
    hidden = [args.source.resolve(), args.sdk_root.resolve(), *[p.resolve() for p in args.hide]]
    assert all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden)
    sandbox = MacSandbox(); sandbox.prepare(work, scratch, seed, tools, hidden)
    # These suites create native test executables below TMPDIR, unlike selection's data-only temp use. Keep them inside
    # the same bounded executable scratch subtree; do not grant execution from arbitrary /tmp or from the SDK.
    sandbox.temp.rmdir()
    sandbox.temp = scratch / 'tmp'; sandbox.temp.mkdir(exist_ok=True)
    sandbox.counter = max((int(p.stem.split('-')[-1]) for p in work.glob('policy-*.sb')), default=0)
    # The selected CRT roles and their canonical aliases are the only external build executables. Job copies and
    # resulting consumers are confined to scratch. ar/ranlib can resolve to Xcode's libtool and are pinned separately.
    host_manifest = json.loads((bundle / 'manifest.json').read_bytes())
    sandbox.allowed += [bundle / value for value in host_manifest['executable_files']]
    sandbox.allowed += [(tools['ar'].parent / 'ranlib').resolve()]
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:2000]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def run(label, argv, *, environment=None, oracle=False, timeout=1800):
        base = sandbox.command([], home)
        policy = Path(base[2])
        if oracle:
            # The reference driver needs host Python/interpreters and SDK tools (including SDK make on macOS). Its result is a
            # differential oracle, not proof of the native executor's executable isolation.
            extra = [Path('/bin'), Path('/usr/bin'), Path(sys.executable).resolve().parent,
                     sdk / 'tools', sdk / 'bin', sdk / 'system/bin']
            # A framework Python's bin/python launcher posix_spawns this executable, outside the launcher's bin/.
            framework_python = Path(sys.prefix) / 'Resources/Python.app/Contents/MacOS/Python'
            if framework_python.is_file(): extra.append(framework_python.resolve().parent)
            text = policy.read_text()
            at = text.index('(deny process-exec')
            text = text[:at] + '(deny process-exec (require-not (require-any ' + ' '.join(
                '(subpath ' + json.dumps(str(p)) + ')' for p in [scratch, *extra]) + ' ' + ' '.join(
                '(literal ' + json.dumps(str(p)) + ')' for p in sandbox.allowed) + ')))\n'
            policy.write_text(text)
        options = sandbox.process_options(home)
        options['env'].update(environment or {})
        command = base[:3] + list(map(str, argv))
        proc = subprocess.run(command, capture_output=True, text=True, timeout=timeout, **options)
        (output / (label + '.stdout')).write_text(proc.stdout)
        (output / (label + '.stderr')).write_text(proc.stderr)
        commands.append({'case': label, 'argv': command, 'exit': proc.returncode, 'oracle_host_tools_allowed': oracle})
        return proc

    completed = False
    try:
        argv = [seed, '--json', 'configure', '--project', project, '--sdk-root', sdk, '--sdk-contract', contract,
                '--contract-sha256', args.contract_sha256]
        for name, path in tools.items():
            argv += ['--' + name, path]
        os_sdk = subprocess.check_output(['/usr/bin/xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
        argv += ['--os-sdk', os_sdk]
        configured = run('configure', argv)
        record('configure against the copied public SDK', configured.returncode == 0, configured.stderr)
        assert configured.returncode == 0
        built = run('build', [seed, '--json', 'build', '--project', project], timeout=3600)
        record('SDK-only CLI build' if previous else 'fresh SDK-only CLI build', built.returncode == 0, built.stderr)
        assert built.returncode == 0
        cli = project / 'out/native/crt-cli'
        sandbox.cli = cli
        env = {'CRT_CLI_TEST_PORT_PROJECT': str(project), 'CRT_CLI_TEST_PORT_HOST_TOOLS': str(bundle),
               'CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256': args.host_tools_sha256,
               'CRT_CLI_TEST_PORT_ARCHIVE': str(archive), 'CRT_CLI_TEST_PORT_PYTHON': str(Path(sys.executable).resolve()),
               'KEEP': '1', 'CRT_CLI_TEST_PORT_SETUID_ITEM': str(setuid_item)}
        for name in args.cases or suites:
            built_test = run('build-' + name, [cli, '--json', 'build', '--project', project, '--target', name], timeout=3600)
            record('build ' + name, built_test.returncode == 0, built_test.stderr)
            if built_test.returncode != 0:
                break
            test_argv = [project / 'out/native' / name]
            if name == 'native-test': test_argv += [cli, project]
            proc = run(name, test_argv, environment=env,
                       oracle=name == 'port-oracle-test', timeout=3600)
            matched = re.search(r'(\d+) checks passed', proc.stdout)
            record(name, proc.returncode == 0 and matched is not None and 'skipped' not in proc.stdout,
                   proc.stdout[-2000:] + proc.stderr[-1000:])
            if proc.returncode != 0:
                break
        else:
            completed = True
        warm = run('warm-build', [cli, '--json', 'build', '--project', project], timeout=3600)
        reply = json.loads(warm.stdout)
        record('the rebuilt CLI reuses every unchanged build command', warm.returncode == 0 and
               reply.get('data', {}).get('cache', {}).get('executed') == 0)
        sandbox.verify(record)
        record('SDK inventory unchanged', inventory(sdk) == before)
        record('original public SDK inventory unchanged', inventory(args.sdk_root) == before)
        record('host-tools payload unchanged', inventory(bundle) == inputs['host_tools'])
    except Exception as error:
        record('replay completed without a harness exception', False, repr(error))
    finally:
        summary = {'passed': completed and all(r['passed'] for r in results), 'cases': results, 'work': str(work),
                   'suites': args.cases or list(suites),
                   'contract_sha256': digest(contract), 'host_tools_sha256': args.host_tools_sha256,
                   'archive_sha256': digest(archive), 'sdk_content_sha256': json.loads(contract.read_bytes())['sdk_content_sha256'],
                   'isolation': sandbox.isolation, 'oracle_exception': 'host Python, shell, make and coreutils permitted only in differential test'}
        if previous:
            summary['resumed_from'] = {'result_sha256': digest(args.resume_from), 'path': str(args.resume_from.resolve()),
                                      'product_inputs_verified_unchanged': True}
        cli = project / 'out/native/crt-cli'
        if cli.is_file(): summary['cli_sha256'] = digest(cli)
        (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n')
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n')
    print(f"{sum(r['passed'] for r in results)}/{len(results)} replay gates passed", flush=True)
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
