#!/usr/bin/env python3
"""Native Windows consumer replay of the pure C static/shared runtime modes of an installed Dev SDK, through the CLI.

Python is a test-host tool; the consumer is the native crt-cli with a cleared environment, a private HOME and explicit LLVM
tools. The SDK is installed from its release manifest (a pinned digest) into a private store, then C projects are built with
`build --sdk` and no configure step:

  static      a static-runtime executable (control): no libc.dll import, loads no libc.dll;
  shared      a shared-runtime executable: imports libc.dll and the process loads exactly the installed SDK's bin\\libc.dll;
  shared+dll  a shared-runtime executable with a project shared library: both import libc.dll, one libc, one errno;
  refusal     a static-runtime executable that depends on a shared library is refused before any output and leaves the lock.

The program that reports the loaded libc.dll is the producer's own installed-SDK consumer (libc/tests/sdk_c_provider_windows_test.c
and sdk_c_provider_dso.c); they are inputs identified by SHA-256, not build inputs of the CLI. Without the SDK `bin` directory on
PATH a shared executable must fail to start. Link-time imports are read with llvm-readobj and compared with the loaded image.
This is a cleared-environment replay, not the restricted-token adapter; nothing is hidden from the consumer and no network is denied.
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


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True, help='directory with the SDK release manifest, archive and contract')
    parser.add_argument('--manifest-name', required=True)
    parser.add_argument('--manifest-sha256', required=True, help='the independently obtained manifest pin')
    parser.add_argument('--c-test-source', type=Path, required=True, help='sdk_c_provider_windows_test.c')
    parser.add_argument('--c-dso-source', type=Path, required=True, help='sdk_c_provider_dso.c')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True, help='fresh work directory (must not exist)')
    for name in ('cc', 'cxx', 'linker', 'ar', 'readobj'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    output, work = args.output.resolve(), args.work.resolve()
    output.mkdir(parents=True)
    work.mkdir(parents=True)
    home, projects, inputs = work / 'home', work / 'projects', work / 'inputs'
    for path in (home, projects, inputs, work / 'tmp'):
        path.mkdir()
    cli = work / 'crt-cli.exe'
    shutil.copy2(args.cli, cli)
    for source in sorted(args.release_dir.iterdir()):
        if source.is_file():
            shutil.copy2(source, inputs / source.name)
    manifest_path = inputs / args.manifest_name
    assert digest(manifest_path) == args.manifest_sha256, 'the supplied pin is not this manifest'
    manifest = json.loads(manifest_path.read_bytes())
    archive = next(a for a in manifest['assets'] if 'install' in a)
    content = archive['install']['content_sha256']
    sdk_dir = home / '.crt' / 'sdks' / (archive['install']['version'] + '-' + content[:12])
    environment = {'HOME': str(home), 'USERPROFILE': str(home), 'PATH': '', 'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C',
                   'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'CRT_WINDOWS_SDK_LIBPATH': os.environ['CRT_WINDOWS_SDK_LIBPATH']}
    tool_flags = ['--cc', str(args.cc.absolute()), '--cxx', str(args.cxx.absolute()), '--linker', str(args.linker.absolute()), '--ar', str(args.ar.absolute())]
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:500]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def run(label, argv, env, expect_ok=True, timeout=1800):
        index = len(commands)
        done = subprocess.run([str(a) for a in argv], capture_output=True, text=True, encoding='utf-8', errors='replace', env=env, cwd=work,
                              timeout=timeout, stdin=subprocess.DEVNULL)
        (output / f'call-{index:03}.stdout').write_text(done.stdout, encoding='utf-8')
        (output / f'call-{index:03}.stderr').write_text(done.stderr, encoding='utf-8')
        commands.append({'case': label, 'argv': [str(a) for a in argv], 'exit': done.returncode})
        return done

    def cli_json(label, tail, expect_ok=True, expect_text=None):
        done = run(label, [cli, '--json', *tail], environment)
        try:
            reply = json.loads(done.stdout)
        except ValueError:
            reply = {}
        message = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (done.returncode == 0 and reply.get('ok') is True) if expect_ok else (done.returncode > 0 and reply.get('ok') is False)
        if expect_text:
            passed = passed and expect_text in message
        record(label, passed, message[:300])
        return reply, passed

    def imports(binary):
        text = run('imports ' + binary.name, [args.readobj, '--coff-imports', binary], os.environ.copy()).stdout
        return sorted({m.lower() for m in re.findall(r'Name:\s+(\S+\.dll)', text, re.IGNORECASE)})

    def project(name, text):
        root = projects / name
        cli_json('new ' + name, ['new', root, '--language', 'c'])
        (root / 'crt.json').write_text(json.dumps(text, indent=2) + '\n', encoding='utf-8')
        (root / 'src/main').mkdir(parents=True, exist_ok=True)
        (root / 'src/test').mkdir(parents=True, exist_ok=True)
        for stale in list((root / 'src/main').iterdir()) + list((root / 'src/test').iterdir()):
            stale.unlink()
        (root / 'include').mkdir(exist_ok=True)
        return root

    def manifest_text(targets):
        return {'manifest_format': 1, 'package': {'name': 'cshared', 'version': '0.1.0'},
                'sdk': {'profile': 'dev', 'version': '*', 'target': 'native', 'storage': 'shared-immutable'},
                'include_dirs': ['include'], 'targets': targets}

    main_c = args.c_test_source.read_text(encoding='utf-8')
    dll_main_c = '#define CRT_TEST_PROJECT_DLL 1\n' + main_c
    dso_c = args.c_dso_source.read_text(encoding='utf-8')
    record('producer consumer sources are identified inputs', True, f'{args.c_test_source.name} {digest(args.c_test_source)[:16]} {args.c_dso_source.name} {digest(args.c_dso_source)[:16]}')

    # 1. Install the SDK from its manifest pin into the private store.
    cli_json('sdk install from the pinned release manifest', ['sdk', 'install', '--archive', inputs / archive['name'], '--release-manifest', manifest_path,
                                                              '--release-manifest-sha256', args.manifest_sha256])
    record('the SDK landed in the private store', (sdk_dir / 'bin' / 'libc.dll').is_file(), str(sdk_dir))
    libc = (sdk_dir / 'bin' / 'libc.dll').resolve()
    system = Path(os.environ['SystemRoot']) / 'System32'
    with_sdk = {'SystemRoot': os.environ['SystemRoot'], 'PATH': str(sdk_dir / 'bin') + os.pathsep + str(system)}
    bare = {'SystemRoot': os.environ['SystemRoot'], 'PATH': str(system)}

    def built_binary(root, name):
        found = sorted(root.rglob(name))
        return found[0] if found else None

    def build_and_check(name, targets, sources, shared, with_dll):
        root = project(name, manifest_text(targets))
        for relative, text in sources.items():
            (root / relative).write_text(text, encoding='utf-8')
        reply, ok = cli_json(name + ': cold build', ['build', '--project', root, '--sdk', content, *tool_flags])
        cold = reply.get('data', {}).get('cache', {})
        lock_bound = (root / 'crt.lock').read_bytes()
        reply, ok_warm = cli_json(name + ': warm build executes nothing', ['build', '--project', root])
        warm = reply.get('data', {}).get('cache', {})
        record(name + ': the cold build executed commands and the warm build none', cold.get('executed', 0) > 0 and warm.get('executed') == 0, f'cold={cold} warm={warm}')
        exe = built_binary(root, 'app.exe')
        record(name + ': the executable exists', exe is not None)
        if exe is None:
            return
        found = imports(exe)
        record(name + ': direct imports ' + ('include' if shared else 'exclude') + ' libc.dll', shared == ('libc.dll' in found), found)
        if with_dll:
            dll = built_binary(root, 'libcrt_provider.dll') or built_binary(root, 'crt_provider.dll')
            record(name + ': the project DLL imports libc.dll too', dll is not None and 'libc.dll' in imports(dll), imports(dll) if dll else 'missing')
        done = run(name + ': run with the SDK bin on PATH', [exe], with_sdk)
        loaded = [line.split(':', 1)[1] for line in done.stdout.splitlines() if line.startswith('LIBC_DLL:')]
        expected = [str(libc)] if shared else ['none']
        agree = done.returncode == 0 and 'sdk_c_provider: ok' in done.stdout and [str(Path(p).resolve()) if p != 'none' else p for p in loaded] == expected
        record(name + ': the loaded image is ' + ('the installed SDK libc.dll' if shared else 'none (static runtime)'), agree, f'loaded={loaded} expected={expected} exit={done.returncode}')
        if shared:
            gone = run(name + ': run without the SDK bin on PATH', [exe], bare)
            record(name + ': it fails to start when nothing provides libc.dll', gone.returncode != 0 and 'sdk_c_provider: ok' not in gone.stdout, f'exit={gone.returncode}')
        reply, ok_run = cli_json(name + ': crt-cli run', ['run', '--project', root])
        record(name + ': the lock written by the first build is unchanged by the warm build and the run', (root / 'crt.lock').read_bytes() == lock_bound and content in lock_bound.decode('utf-8', 'replace'),
               'lock selects the installed SDK content: %s' % (content in lock_bound.decode('utf-8', 'replace')))

    build_and_check('static', [{'name': 'app', 'kind': 'executable', 'runtime_linkage': 'static', 'sources': ['src/main/main.c']}],
                    {'src/main/main.c': main_c}, shared=False, with_dll=False)
    build_and_check('shared', [{'name': 'app', 'kind': 'executable', 'runtime_linkage': 'shared', 'sources': ['src/main/main.c']}],
                    {'src/main/main.c': main_c}, shared=True, with_dll=False)
    build_and_check('shared-dll', [{'name': 'crt_provider', 'kind': 'shared-library', 'sources': ['src/main/provider.c']},
                                   {'name': 'app', 'kind': 'executable', 'runtime_linkage': 'shared', 'sources': ['src/main/main.c'], 'dependencies': ['crt_provider']}],
                    {'src/main/main.c': dll_main_c, 'src/main/provider.c': dso_c}, shared=True, with_dll=True)

    # 2. A static-runtime executable depending on a shared library is a second runtime instance: refused before any output.
    root = project('mixed', manifest_text([{'name': 'crt_provider', 'kind': 'shared-library', 'sources': ['src/main/provider.c']},
                                           {'name': 'app', 'kind': 'executable', 'runtime_linkage': 'static', 'sources': ['src/main/main.c'], 'dependencies': ['crt_provider']}]))
    (root / 'src/main/main.c').write_text(dll_main_c, encoding='utf-8')
    (root / 'src/main/provider.c').write_text(dso_c, encoding='utf-8')
    lock_before = (root / 'crt.lock').read_bytes()
    cli_json('mixed: a static executable with a shared-library dependency is refused', ['build', '--project', root, '--sdk', content, *tool_flags], expect_ok=False)
    produced = [p for p in root.rglob('*') if p.is_file() and p.suffix.lower() in ('.exe', '.dll', '.o', '.obj')]
    record('mixed: no output artifact and the lock is untouched', not produced and (root / 'crt.lock').read_bytes() == lock_before, [p.name for p in produced][:4])

    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'cli_sha256': digest(cli), 'manifest_sha256': args.manifest_sha256,
               'sdk_content_sha256': content, 'libc_dll_sha256': digest(libc), 'sdk_directory': str(sdk_dir),
               'inputs': {args.c_test_source.name: digest(args.c_test_source), args.c_dso_source.name: digest(args.c_dso_source)},
               'isolation': 'cleared environment, private HOME and explicit LLVM tools; not the restricted-token adapter; nothing hidden, no network denial'}
    (output / 'result.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    (output / 'commands.json').write_text(json.dumps(commands, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f"{sum(r['passed'] for r in results)}/{len(results)} cases passed")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
