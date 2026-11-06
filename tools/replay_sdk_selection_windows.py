#!/usr/bin/env python3
"""Native Windows selection / first-build adapter for the shared installed-store cases.

Runs `replay_sdk_selection_cases` or `replay_first_build_cases` against an SDK installed from the original release
manifest. Every CLI call runs under CRT's `crt_windows_trace` (restricted token naming a per-run SID, empty PATH,
redirected stdio, job-object process accounting). The accounting records processes; it does not deny them. The
working directory (searched first by CreateProcess for a bare name) holds decoy executables and junk libraries named
like the tools a careless implementation would look up: any process started from them is a stray and fails the call.
Python and the declared LLVM tools are test-host tools, not CLI dependencies.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys

from replay_first_build_cases import first_build_cases
from replay_sdk_selection_cases import selection_cases


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True, help='fresh private work directory (must not exist)')
    parser.add_argument('--probe', type=Path, required=True, help='SDK-built executable: the hidden-root probe and the decoy image')
    parser.add_argument('--crt-tools', type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--cases', choices=('selection', 'first-build'), default='selection')
    parser.add_argument('--allow-dirty-sdk', action='store_true')
    for name in ('cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    tools = {name: getattr(args, name).absolute() for name in ('cc', 'cxx', 'linker', 'ar')}
    sys.path.insert(0, str(args.crt_tools.resolve()))
    import crt_windows_external as sandbox
    import crt_windows_trace as trace

    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    work, sid = sandbox.create_work(args.work)
    hidden = sorted({p.resolve(strict=True) for p in args.hide})
    assert hidden and all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden)
    (work / 'tmp').mkdir()
    cli, probe = work / 'crt-cli.exe', work / 'probe.exe'
    shutil.copy2(args.cli, cli); shutil.copy2(args.probe, probe)
    inputs = work / 'inputs'; inputs.mkdir()
    manifest_src = next(args.release_dir.glob('release-manifest-dev-windows-*.json'))
    manifest = json.loads(manifest_src.read_bytes())
    asset = next(entry for entry in manifest['assets'] if 'install' in entry)
    for name in (manifest_src.name, asset['name'], asset['contract']):
        shutil.copy2(args.release_dir / name, inputs / name)
    archive, manifest_path = inputs / asset['name'], inputs / manifest_src.name
    inputs_before = {p.name: digest(p) for p in inputs.iterdir()}
    scratch = work / 'selection space 한글'; scratch.mkdir()
    home = scratch / 'home space 한글'; home.mkdir()
    store = home / '.crt' / 'sdks'
    # Decoys beside the working directory: never to be started.
    decoys = []
    for name in ('clang.exe', 'clang++.exe', 'ld.lld.exe', 'ld.exe', 'lld-link.exe', 'llvm-ar.exe', 'ar.exe', 'link.exe', 'cl.exe',
                 'python.exe', 'sh.exe', 'crt-cc.exe', 'crt-c++.exe'):
        shutil.copy2(probe, work / name); decoys.append(work / name)
    for name in ('stdio.h', 'stdlib.h', 'stddef.h', 'vector'):
        (work / name).write_text('#error unexpected inherited include search path\n')
    # Junk runtime libraries live in a separate directory used as the working directory by one explicit case only: Windows
    # searches the working directory for DLLs before PATH, which is an OS property the CLI does not claim to defend against.
    planted = work / 'planted-cwd'; planted.mkdir()
    for name in ('libc++.dll', 'libc.dll', 'zlib1.dll', 'liblzma.dll', 'vcruntime140.dll'):
        (planted / name).write_bytes(b'not a loadable runtime\n')
    assert os.environ.get('CRT_WINDOWS_SDK_LIBPATH'), 'set CRT_WINDOWS_SDK_LIBPATH to the Windows SDK um/<arch> import-library directory'
    environment = {'HOME': str(home), 'USERPROFILE': str(home), 'PATH': '', 'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C',
                   'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'CRT_WINDOWS_SDK_LIBPATH': os.environ['CRT_WINDOWS_SDK_LIBPATH']}
    permitted = {cli.resolve(), probe.resolve(), Path(os.environ['SystemRoot'], 'System32/conhost.exe').resolve()}
    tool_directories = {path.resolve().parent for path in tools.values()}
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:500]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def launch(argv, tools_allowed=True, timeout=900, cwd=None):
        label = f'call-{len(commands):03d}'
        out, err = output / (label + '.stdout'), output / (label + '.stderr')
        result = trace.run([str(cli), '--json', *map(str, argv)], work=work, sid=sid, env=environment, cwd=cwd or work,
                           stdout=out, stderr=err, timeout=timeout)

        def known(image):
            path = Path(image).resolve()
            return path in permitted or (tools_allowed and (path.parent in tool_directories or path.is_relative_to(scratch.resolve())))
        stray = [p for p in result['processes'] if not p['image'] or not known(p['image'])]
        commands.append({'label': label, 'argv': [str(x) for x in argv], 'exit': result['exit_code'], 'timed_out': result['timed_out'],
                         'images': sorted({str(p['image']) for p in result['processes']})})
        return result, out.read_bytes(), stray

    def readonly(path):
        """The run SID may only read and execute `path`; the orchestrator keeps full control."""
        kernel, advapi = sandbox.api()
        descriptor, dacl = ctypes.c_void_p(), ctypes.c_void_p()
        sddl = 'D:P' + ''.join(f'(A;OICI;FA;;;{i})' for i in (sandbox.caller_sid(kernel, advapi), 'SY', 'BA')) + f'(A;OICI;GRGX;;;{sid})'
        sandbox.checked(advapi.ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, 1, ctypes.byref(descriptor), None))
        try:
            present, defaulted = w.BOOL(), w.BOOL()
            sandbox.checked(advapi.GetSecurityDescriptorDacl(descriptor, ctypes.byref(present), ctypes.byref(dacl), ctypes.byref(defaulted)))
            error = advapi.SetNamedSecurityInfoW(str(path), 1, 4 | 0x80000000, None, None, dacl, None)
            if error:
                raise ctypes.WinError(error)
        finally:
            kernel.LocalFree(descriptor)

    def call(label, tail, expect_ok=True, expect_text=None, readonly_paths=(), tools=True, cwd=None):
        for path in readonly_paths:
            readonly(path)
        result, stdout, stray = launch(tail, tools_allowed=tools, cwd=cwd)
        try:
            reply = json.loads(stdout)
        except ValueError:
            reply = {}
        detail = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (result['exit_code'] == 0) == expect_ok and bool(reply.get('ok')) == expect_ok and not result['timed_out'] and not stray
        if expect_text:
            passed = passed and expect_text in detail
        record(label, passed, detail + (f' stray={stray[:2]}' if stray else ''))
        return reply

    boundary = [str(probe)]
    for path in hidden:
        boundary += ['--read', str(path)]
    probe_result = trace.run(boundary, work=work, sid=sid, env=environment, cwd=work, stdout=output / 'probe.stdout',
                             stderr=output / 'probe.stderr', timeout=60)
    record('hidden roots are unreadable under the run SID', probe_result['exit_code'] == 0, f'{len(hidden)} roots')
    summary = {}
    try:
        assert probe_result['exit_code'] == 0, 'boundary probe failed'
        call('install the original pinned release', ['sdk', 'install', '--archive', str(archive), '--release-manifest', str(manifest_path),
             '--release-manifest-sha256', digest(manifest_path)], tools=False)
        sdk_dir = asset['install']['version'] + '-' + asset['install']['content_sha256'][:12]
        before = {str(path.relative_to(store / sdk_dir)): digest(path) for path in (store / sdk_dir).rglob('*') if path.is_file()}
        if args.cases == 'selection':
            selection_cases(call=call, record=record, store=store, sdk_dir=sdk_dir, scratch=scratch, tools=tools,
                            allow_dirty_sdk=args.allow_dirty_sdk)
        else:
            def publication_failure(project, argv):
                # Replacing a file needs DELETE on it, and the parent's delete-child right would bypass any file ACL. A handle
                # opened without FILE_SHARE_DELETE (Python's default) blocks the replacement itself, as a locked lock file would.
                with (project / 'crt.lock').open('rb'):
                    call('locked lock publication is refused', argv, expect_ok=False)
            first_build_cases(call=call, record=record, store=store, sdk_dir=sdk_dir, scratch=scratch, tools=tools,
                              allow_dirty_sdk=args.allow_dirty_sdk, publication_failure=publication_failure)
        if args.cases == 'selection':
            shared = scratch / 'shared-project'
            call('shared-runtime run in a clean working directory', ['run', '--project', str(shared)])
            call('a junk libc.dll planted in the working directory shadows the PATH runtime (documented Windows limitation)',
                 ['run', '--project', str(shared)], expect_ok=False, cwd=planted)
        after = {str(path.relative_to(store / sdk_dir)): digest(path) for path in (store / sdk_dir).rglob('*') if path.is_file()}
        record('original SDK file bytes are unchanged', before == after)
        call('final original SDK verification', ['sdk', 'verify', sdk_dir], tools=False)
    except Exception as error:
        record('selection replay completed without a harness exception', False, repr(error))
    finally:
        record('no decoy executable started (images checked per call)', all(
            str(Path(image).resolve()) not in {str(d.resolve()) for d in decoys}
            for entry in commands for image in entry['images'] if image))
        record('release inputs are unchanged', inputs_before == {p.name: digest(p) for p in inputs.iterdir()})
        summary = {'passed': bool(results) and all(case['passed'] for case in results), 'cases': results, 'work': str(work),
                   'cli_sha256': digest(cli), 'archive_sha256': digest(archive), 'manifest_sha256': digest(manifest_path),
                   'allow_dirty_sdk': args.allow_dirty_sdk, 'suite': args.cases,
                   'isolation': 'Windows restricted token (per-run SID, empty PATH), private work directory with decoy executables, '
                                'job-object process accounting; no network denial and no read-only root are claimed'}
        (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f"{sum(case['passed'] for case in results)}/{len(results)} passed")
    return 0 if summary.get('passed') else 1


if __name__ == '__main__':
    raise SystemExit(main())
