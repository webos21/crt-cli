#!/usr/bin/env python3
"""Bounded native Linux D3 closure replay using public SDK/host-tool inputs.

Python/bwrap/strace and the separately labeled packaged oracle are test-host
tools. Native jobs have no network, source-tree access or writable SDK/bundle.
"""
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


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inventory(root):
    return [[path.relative_to(root).as_posix(), 'symlink' if path.is_symlink() else 'file',
             os.readlink(path) if path.is_symlink() else digest(path)]
            for path in sorted(root.rglob('*')) if path.is_symlink() or path.is_file()]


def source_inputs(root):
    paths = sorted([*root.joinpath('src').rglob('*'), *root.joinpath('include').rglob('*'), root / 'crt.json'])
    return [[path.relative_to(root).as_posix(), hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()]
            for path in paths if path.is_file()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'sdk-root', 'contract', 'host-tools', 'archive', 'seed', 'output', 'work-parent',
                 'cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--contract-sha256', required=True)
    parser.add_argument('--host-tools-sha256', required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--allow-dirty-sdk', action='store_true')
    suites = ('port-recipe-test', 'port-build-test', 'port-store-test', 'port-project-test',
              'port-command-test', 'port-oracle-test', 'native-test')
    parser.add_argument('--cases', nargs='+', choices=suites)
    args = parser.parse_args()
    assert sys.platform.startswith('linux')
    assert digest(args.contract) == args.contract_sha256
    assert digest(args.host_tools / 'manifest.json') == args.host_tools_sha256
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='d3-linux-', dir=args.work_parent.resolve()))
    assert not work.is_relative_to('/tmp')
    sdk, bundle, provider = work / 'sdk', work / 'host-tools', work / 'provider.json'
    scratch = work / 'scratch'; scratch.mkdir()
    project = scratch / 'cli source 한글'; project.mkdir()
    for name in ('src', 'include'):
        shutil.copytree(args.source / name, project / name)
    shutil.copy2(args.source / 'crt.json', project / 'crt.json')
    shutil.copytree(args.sdk_root, sdk, symlinks=True)
    shutil.copytree(args.host_tools, bundle, symlinks=True)
    archive, seed = work / args.archive.name, work / 'seed-cli'
    shutil.copy2(args.contract, provider); shutil.copy2(args.archive, archive); shutil.copy2(args.seed, seed)
    sdk_before, bundle_before = inventory(sdk), inventory(bundle)
    source_rows = source_inputs(project)
    tools = {role: getattr(args, role).absolute() for role in ('cc', 'cxx', 'linker', 'ar')}
    hidden = [args.source.resolve(), args.sdk_root.resolve(), args.host_tools.resolve(), *[path.resolve() for path in args.hide]]
    assert all(not work.is_relative_to(path) and not path.is_relative_to(work) for path in hidden)
    inputs = {'source': source_rows, 'source_sha256': hashlib.sha256(json.dumps(source_rows, separators=(',', ':')).encode()).hexdigest(),
              'sdk': sdk_before, 'host_tools': bundle_before, 'contract_sha256': digest(provider), 'archive_sha256': digest(archive),
              'seed_sha256': digest(seed), 'external_tools': {role: {'path': str(path), 'sha256': digest(path)} for role, path in tools.items()},
              'driver_sha256': digest(Path(__file__)), 'allow_dirty_sdk': args.allow_dirty_sdk}
    (output / 'inputs.json').write_text(json.dumps(inputs, ensure_ascii=False, indent=2) + '\n')
    home, temporary, traces = scratch / 'home', scratch / 'tmp', scratch / 'traces'
    for path in (home, temporary, traces): path.mkdir()
    results, commands, outcomes = [], [], {}

    def record(label, passed, detail=''):
        results.append({'case': label, 'passed': bool(passed), 'detail': str(detail)[-1600:]})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)

    def run(label, argv, environment=None, oracle=False, hidden_extra=(), timeout=3600):
        prefix = ['bwrap', '--unshare-user', '--unshare-ipc', '--unshare-uts', '--unshare-net', '--new-session', '--die-with-parent',
                  '--ro-bind', '/', '/', '--proc', '/proc', '--dev', '/dev', '--tmpfs', '/tmp', '--clearenv',
                  '--setenv', 'HOME', str(home), '--setenv', 'PATH', '/nonexistent', '--setenv', 'TMPDIR', str(temporary)]
        for key, value in (environment or {}).items(): prefix += ['--setenv', key, value]
        prefix += ['--bind', str(scratch), str(scratch)]
        for path in [*hidden, *hidden_extra]:
            prefix += ['--tmpfs', str(path)] if path.is_dir() else ['--ro-bind', '/dev/null', str(path)]
        prefix += ['--chdir', str(scratch), shutil.which('strace'), '-f', '-qq', '-s', '4096',
                   '-e', 'trace=execve', '-o', str(traces / (label + '.log')), '--', *map(str, argv)]
        proc = subprocess.run(prefix, capture_output=True, text=True, stdin=subprocess.DEVNULL, timeout=timeout)
        (output / (label + '.stdout')).write_text(proc.stdout)
        (output / (label + '.stderr')).write_text(proc.stderr)
        commands.append({'case': label, 'argv': prefix, 'exit': proc.returncode, 'oracle_host_tools_allowed': oracle})
        return proc

    complete = False
    try:
        argv = [seed, '--json', 'configure', '--project', project, '--sdk-root', sdk, '--sdk-contract', provider,
                '--contract-sha256', args.contract_sha256]
        for role, path in tools.items(): argv += ['--' + role, path]
        if args.allow_dirty_sdk: argv += ['--allow-dirty-sdk', 'true']
        configured = run('configure', argv)
        record('configure against the copied public SDK', configured.returncode == 0, configured.stderr)
        assert configured.returncode == 0
        built = run('build', [seed, '--json', 'build', '--project', project])
        record('fresh SDK-only CLI build', built.returncode == 0, built.stderr)
        assert built.returncode == 0
        cli = project / 'out/native/crt-cli'
        bare = work / 'bare-cli'; shutil.copy2(cli, bare)
        for label, tail in [('version', ['--version']), ('doctor', ['doctor', '--bootstrap'])]:
            proc = run('bare-' + label, [bare, '--json', *tail], hidden_extra=(sdk, provider, project))
            reply = json.loads(proc.stdout)
            record('SDK/source-free ' + label, proc.returncode == 0 and reply.get('ok') is True, proc.stderr)
        dynamic = subprocess.check_output(['readelf', '-d', str(cli)], text=True)
        (output / 'cli-readelf.txt').write_text(dynamic)
        record('CLI has no dynamic library imports or SDK runpath', '(NEEDED)' not in dynamic and '(RPATH)' not in dynamic and '(RUNPATH)' not in dynamic)

        probe = scratch / 'visibility'
        assert run('probe-new', [cli, '--json', 'new', probe, '--language', 'c']).returncode == 0
        paths = list(dict.fromkeys(str(path / 'manifest.json' if path == args.sdk_root.resolve() or path == args.host_tools.resolve() else path / 'TODO.md')
                                   for path in hidden if path.is_dir()))
        body = '#include <stdio.h>\nint main(void) {\n'
        for path in paths: body += 'if (fopen(' + json.dumps(path, ensure_ascii=False) + ', "rb")) return 1;\n'
        body += 'puts("original inputs denied"); return 0; }\n'
        (probe / 'src/main/main.c').write_text(body)
        config = {'format': 0, 'sdk_root': str(sdk), 'sdk_contract': str(provider), 'contract_sha256': args.contract_sha256,
                  **{role: str(path) for role, path in tools.items()}}
        if args.allow_dirty_sdk: config['allow_dirty_sdk'] = True
        (probe / '.sdk.json').write_text(json.dumps(config))
        assert run('probe-build', [cli, '--json', 'build', '--project', probe]).returncode == 0
        proc = run('probe-run', [cli, '--json', 'run', '--project', probe])
        record('native probe cannot read original repositories/SDK/tools', proc.returncode == 0 and 'original inputs denied' in proc.stderr, proc.stderr)

        env = {'CRT_CLI_TEST_PORT_PROJECT': str(project), 'CRT_CLI_TEST_PORT_HOST_TOOLS': str(bundle),
               'CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256': args.host_tools_sha256, 'CRT_CLI_TEST_PORT_ARCHIVE': str(archive),
               'CRT_CLI_TEST_PORT_PYTHON': str(Path(sys.executable).resolve()), 'KEEP': '1'}
        for name in args.cases or suites:
            proc = run('build-' + name, [cli, '--json', 'build', '--project', project, '--target', name])
            record('build ' + name, proc.returncode == 0, proc.stderr)
            assert proc.returncode == 0
            argv = [project / 'out/native' / name]
            if name == 'native-test': argv += [cli, project]
            proc = run(name, argv, environment=env, oracle=name == 'port-oracle-test')
            match = re.search(r'(\d+) checks passed', proc.stdout)
            passed = proc.returncode == 0 and match is not None and 'skipped' not in proc.stdout
            record(name, passed, proc.stdout[-1600:] + proc.stderr[-1000:])
            outcomes[name] = {'passed': passed, 'checks': int(match.group(1)) if match else None}
            assert passed

        tail = ['port', 'build', 'zlib', '--project', project, '--archive', archive, '--host-tools', bundle,
                '--host-tools-sha256', args.host_tools_sha256]
        cold = run('port-cold', [cli, '--json', *tail]); warm = run('port-reuse', [cli, '--json', *tail])
        a, b = json.loads(cold.stdout), json.loads(warm.stdout)
        record('actual port command publishes once and reuses the exact key', cold.returncode == warm.returncode == 0 and
               a['data']['already_published'] is False and b['data']['already_published'] is True and a['data']['build_key'] == b['data']['build_key'])
        proc = run('port-verify', [cli, '--json', 'port', 'verify', a['data']['build_key'], '--project', project])
        record('actual published port item verifies', proc.returncode == 0, proc.stderr)
        proc = run('warm-build', [cli, '--json', 'build', '--project', project]); reply = json.loads(proc.stdout)
        record('unchanged CLI build executes no commands', proc.returncode == 0 and reply['data']['cache']['executed'] == 0)
        complete = True
    except Exception as error:
        record('replay completed without a harness exception', False, repr(error))
    finally:
        for name, root, original in [('SDK', sdk, sdk_before), ('host tools', bundle, bundle_before),
                                     ('original SDK', args.sdk_root, sdk_before), ('original host tools', args.host_tools, bundle_before)]:
            record(name + ' inventory unchanged', inventory(root) == original)
        record('CLI product/test source matches the baseline', source_inputs(project) == source_rows == source_inputs(args.source))
        bad = []
        for trace in traces.glob('*.log'):
            if trace.stem == 'port-oracle-test': continue
            for line in trace.read_text().splitlines():
                if re.search(r'execve\("/(?:usr/)?bin/(?:sh|dash|bash|python[^"/]*|perl|ruby)"', line): bad.append(line)
        record('native jobs execute no host shell/interpreter', not bad, bad[:8])
        result = {'passed': complete and all(case['passed'] for case in results), 'cases': results, 'suites': outcomes, 'work': str(work),
                  'source_sha256': inputs['source_sha256'], 'contract_sha256': digest(provider), 'host_tools_sha256': args.host_tools_sha256,
                  'archive_sha256': digest(archive), 'allow_dirty_sdk': args.allow_dirty_sdk,
                  'isolation': 'bwrap: network denied, read-only root/SDK/bundle, hidden original trees, private HOME/tmp, shared PID namespace; execve trace'}
        if (project / 'out/native/crt-cli').exists(): result['cli_sha256'] = digest(project / 'out/native/crt-cli')
        (output / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n')
        print(f"{sum(case['passed'] for case in results)}/{len(results)} passed", flush=True)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
