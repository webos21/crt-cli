#!/usr/bin/env python3
"""Consume the actual public download of a Development SDK Preview natively on Windows. Python is a test-host tool.

Every CLI call runs under CRT's `crt_windows_trace`: a restricted token naming a per-run SID, an empty PATH, redirected stdio and a
job object that records every process the call starts (accounting, not denial; no network denial is claimed). The inputs are the
downloaded files exactly as served: the SDK archive, the release manifest and the portable compiler policy. The SDK is installed from
the independently pinned manifest digest, then C and C++ projects are built *without configure* through the installed SDK
(`build --sdk`), tested, run and rebuilt warm. The producer and CLI checkouts are proven unreadable under the run SID by a native probe.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--download-dir', type=Path, required=True)
    parser.add_argument('--manifest-name', required=True)
    parser.add_argument('--manifest-sha256', required=True, help='the independently obtained pin')
    parser.add_argument('--content-sha256', required=True, help='the promised installed-SDK identity')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True, help='fresh private work directory (must not exist)')
    parser.add_argument('--probe', type=Path, required=True, help='SDK-built executable that must fail to read every --hide root')
    parser.add_argument('--crt-tools', type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    for name in ('cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    assert os.environ.get('CRT_WINDOWS_SDK_LIBPATH'), 'set CRT_WINDOWS_SDK_LIBPATH to the Windows SDK um/<arch> import-library directory'
    sys.path.insert(0, str(args.crt_tools.resolve()))
    import crt_windows_external as sandbox
    import crt_windows_trace as trace

    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    work, sid = sandbox.create_work(args.work)
    hidden = sorted({p.resolve(strict=True) for p in args.hide})
    assert hidden and all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden), 'work must be outside hidden inputs'
    for sub in ('tmp', 'scratch', 'inputs'):
        (work / sub).mkdir()
    cli, probe = work / 'crt-cli.exe', work / 'probe.exe'
    shutil.copy2(args.cli, cli); shutil.copy2(args.probe, probe)
    inputs, scratch = work / 'inputs', work / 'scratch'
    for source in sorted(args.download_dir.iterdir()):
        if source.is_file():
            shutil.copy2(source, inputs / source.name)
    before_inputs = {p.name: digest(p) for p in sorted(inputs.iterdir())}
    home = scratch / 'home'; home.mkdir()
    tools = {name: getattr(args, name).absolute() for name in ('cc', 'cxx', 'linker', 'ar')}
    tool_directories = {path.resolve().parent for path in tools.values()}
    environment = {'HOME': str(home), 'USERPROFILE': str(home), 'PATH': '', 'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C',
                   'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'CRT_WINDOWS_SDK_LIBPATH': os.environ['CRT_WINDOWS_SDK_LIBPATH']}
    permitted = {cli.resolve(), probe.resolve(), Path(os.environ['SystemRoot'], 'System32/conhost.exe').resolve()}
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:400]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def call(label, tail, expect_ok=True, check=None, expect_text=None, tools_allowed=False):
        index = len(commands)
        out, err = output / f'call-{index:03}.stdout', output / f'call-{index:03}.stderr'
        result = trace.run([str(cli), '--json', *map(str, tail)], work=work, sid=sid, env=environment, cwd=work, stdout=out, stderr=err, timeout=1800)

        def known(image):
            path = Path(image).resolve()
            return path in permitted or (tools_allowed and (path.parent in tool_directories or path.is_relative_to(scratch.resolve())))
        stray = [p for p in result['processes'] if not p['image'] or not known(p['image'])]
        commands.append({'case': label, 'argv': [str(x) for x in tail], 'exit': result['exit_code'], 'timed_out': result['timed_out'],
                         'images': sorted({str(p['image']) for p in result['processes']})})
        try:
            reply = json.loads(out.read_bytes())
        except ValueError:
            reply = {'ok': result['exit_code'] == 0, 'data': {'message': out.read_text(errors='replace')[:300]}}
        message = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (result['exit_code'] == 0) == expect_ok and bool(reply.get('ok')) == expect_ok and not result['timed_out'] and not stray
        if expect_text:
            passed = passed and expect_text in message
        if check:
            passed = passed and bool(check(reply))
        record(label, passed, message[:300] + (f' stray={stray[:2]}' if stray else ''))
        return reply

    boundary = [str(probe)]
    for path in hidden:
        boundary += ['--read', str(path)]
    probe_result = trace.run(boundary, work=work, sid=sid, env=environment, cwd=work, stdout=output / 'probe.stdout', stderr=output / 'probe.stderr', timeout=60)
    record('hidden producer and CLI trees are unreadable under the run SID', probe_result['exit_code'] == 0, f'{len(hidden)} roots')
    assert probe_result['exit_code'] == 0, 'boundary probe failed'

    tool_flags = ['--cc', str(tools['cc']), '--cxx', str(tools['cxx']), '--linker', str(tools['linker']), '--ar', str(tools['ar'])]
    manifest_path = inputs / args.manifest_name
    manifest = json.loads(manifest_path.read_bytes())
    archive = next(a for a in manifest['assets'] if 'install' in a)
    contract = next(a for a in manifest['assets'] if a['name'] == archive['contract'])

    # 1. The downloaded files are exactly what the pin and the manifest promise.
    record('the manifest equals the independently obtained pin', digest(manifest_path) == args.manifest_sha256, digest(manifest_path))
    for asset in manifest['assets']:
        path = inputs / asset['name']
        record('asset ' + asset['name'] + ' matches its manifest size and SHA-256', path.is_file() and path.stat().st_size == asset['size'] and digest(path) == asset['sha256'])
    record('the manifest names the promised installed identity', archive['install']['content_sha256'] == args.content_sha256)

    # 2. Pinned native installation and verification.
    store = home / '.crt' / 'sdks'
    install = ['sdk', 'install', '--archive', str(inputs / archive['name']), '--release-manifest', str(manifest_path),
               '--release-manifest-sha256', args.manifest_sha256, '--store', str(store)]
    call('install from the pinned manifest', install,
         check=lambda r: r['data']['identity']['content_sha256'] == args.content_sha256 and not r['data']['already_installed'])
    directory = sorted(p.name for p in store.iterdir() if not p.name.startswith('.'))
    record('exactly one installed SDK and empty bookkeeping', len(directory) == 1 and not list((store / '.staging').glob('*')) and not list((store / '.locks').glob('*')), directory)
    sdk_dir = directory[0] if directory else ''
    call('sdk verify', ['sdk', 'verify', sdk_dir, '--store', str(store)], check=lambda r: r['data']['verified'])
    call('reinstall is idempotent', install, check=lambda r: r['data']['already_installed'])
    wrong = list(install); wrong[wrong.index('--release-manifest-sha256') + 1] = '0' * 64; wrong[wrong.index('--store') + 1] = str(scratch / 'wrong-pin-store')
    call('a wrong manifest pin is refused', wrong, expect_ok=False)
    record('the refused install published nothing', not (scratch / 'wrong-pin-store').exists() or not [p for p in (scratch / 'wrong-pin-store').iterdir() if not p.name.startswith('.')])

    # 3. C and C++ first build through the installed SDK, no configure.
    for language in ('c', 'cxx'):
        project = scratch / f'hello-{language}'
        call(f'{language}: new is source-only', ['new', str(project), '--language', language])
        record(f'{language}: new wrote no binding', not (project / '.crt').exists())
        call(f'{language}: first build without configure', ['build', '--project', str(project), '--sdk', args.content_sha256, *tool_flags],
             check=lambda r: r['data']['cache']['executed'] > 0, tools_allowed=True)
        lock = json.loads((project / 'crt.lock').read_bytes())
        selection = lock.get('selection', {})
        record(f'{language}: the lock selects the installed SDK and its stored policy',
               lock.get('lock_format') == 1 and selection.get('sdk_content_sha256') == args.content_sha256 and selection.get('contract_sha256') == contract['sha256'] and
               selection.get('contract_source') == 'store' and selection.get('provenance', {}).get('producer_working_tree_dirty') in (False, None),
               json.dumps(selection)[:300])
        locked = (project / 'crt.lock').read_bytes()
        call(f'{language}: test', ['test', '--project', str(project)], check=lambda r: all(o['exit_code'] == 0 for o in r['data']['outcomes']), tools_allowed=True)
        call(f'{language}: run', ['run', '--project', str(project)], check=lambda r: all(o['exit_code'] == 0 for o in r['data']['outcomes']), tools_allowed=True)
        call(f'{language}: warm build executes nothing', ['build', '--project', str(project)],
             check=lambda r: r['data']['cache']['executed'] == 0 and not r['data']['commands'], tools_allowed=True)
        record(f'{language}: the project lock is unchanged by later builds', (project / 'crt.lock').read_bytes() == locked)

    # 4. Nothing it consumed changed.
    call('sdk verify after the builds', ['sdk', 'verify', sdk_dir, '--store', str(store)], check=lambda r: r['data']['verified'])
    record('every downloaded input is unchanged', {p.name: digest(p) for p in sorted(inputs.iterdir())} == before_inputs)
    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'cli_sha256': digest(cli), 'sdk_directory': sdk_dir, 'work': str(work),
               'manifest_sha256': args.manifest_sha256, 'content_sha256': args.content_sha256,
               'isolation': 'Windows restricted token (per-run SID, empty PATH), private work directory, job-object process accounting; '
                            'no network denial and no read-only root are claimed'}
    (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('passed' if summary['passed'] else 'FAILED', f"{sum(r['passed'] for r in results)}/{len(results)}")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
