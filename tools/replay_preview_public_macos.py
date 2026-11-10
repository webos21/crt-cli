#!/usr/bin/env python3
"""Consume the actual public download of a Development SDK Preview natively on macOS. Python/sandbox-exec are test-host tools.

Every CLI call runs under a Seatbelt profile: no network, writes only to one scratch directory (HOME with the SDK store,
the projects) and a private temporary directory, the producer/CLI checkouts and any other --hide path unreadable, a
cleared environment (empty PATH). The inputs are the downloaded files exactly as served: the SDK archive, the release
manifest and the portable compiler policy. The SDK is installed from the independently pinned manifest digest, then C and
C++ projects are built *without configure* through the installed SDK (`build --sdk`), tested, run and rebuilt warm.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


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
    parser.add_argument('--work-parent', type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    for name in ('cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    sandbox = '/usr/bin/sandbox-exec'
    assert sys.platform == 'darwin' and Path(sandbox).exists(), 'macOS sandbox-exec is required'
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='public-preview-', dir=args.work_parent.resolve()))
    hidden = [p.resolve() for p in args.hide]
    assert all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden), 'work must be outside hidden inputs'
    for sub in ('tmp', 'scratch', 'inputs'):
        (work / sub).mkdir()
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    inputs = work / 'inputs'
    for source in sorted(args.download_dir.iterdir()):
        if source.is_file():
            shutil.copy2(source, inputs / source.name)
    before_inputs = {p.name: digest(p) for p in sorted(inputs.iterdir())}
    scratch = work / 'scratch'
    home = scratch / 'home'; home.mkdir()
    results = []
    counter = [0]
    environment = {'HOME': str(home), 'PATH': '/nonexistent', 'TMPDIR': str(work / 'tmp'), 'LC_ALL': 'C'}

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:400]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def command(tail):
        counter[0] += 1
        text = '(version 1)\n(allow default)\n(deny network*)\n'
        for path in hidden:
            text += '(deny file-read* file-write* (subpath ' + json.dumps(str(path)) + '))\n'
        text += '(deny file-write* (require-not (require-any (subpath "/dev") (subpath ' + json.dumps(str(work / 'tmp')) + ') (subpath ' + json.dumps(str(scratch)) + '))))\n'
        profile = work / f'policy-{counter[0]}.sb'; profile.write_text(text)
        return [sandbox, '-f', str(profile), str(cli), '--json', *tail]

    def call(label, tail, expect_ok=True, check=None, expect_text=None):
        proc = subprocess.run(command(tail), capture_output=True, text=True, timeout=1800, env=environment, cwd=work)
        try:
            reply = json.loads(proc.stdout)
        except Exception:
            reply = {'ok': proc.returncode == 0, 'data': {'message': proc.stdout + proc.stderr}}
        message = json.dumps(reply.get('data', {}))
        passed = (proc.returncode == 0) == expect_ok and bool(reply.get('ok')) == expect_ok
        if expect_text:
            passed = passed and expect_text in message
        if check:
            passed = passed and bool(check(reply))
        (output / f'call-{counter[0]:03}.stdout').write_text(proc.stdout)
        (output / f'call-{counter[0]:03}.stderr').write_text(proc.stderr)
        record(label, passed, message[:300])
        return reply

    tools = ['--cc', str(args.cc), '--cxx', str(args.cxx), '--linker', str(args.linker), '--ar', str(args.ar)]
    manifest_path = inputs / args.manifest_name
    manifest = json.loads(manifest_path.read_text())
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
    reply = call('install from the pinned manifest', ['sdk', 'install', '--archive', str(inputs / archive['name']), '--release-manifest', str(manifest_path),
                 '--release-manifest-sha256', args.manifest_sha256, '--store', str(store)],
                 check=lambda r: r['data']['identity']['content_sha256'] == args.content_sha256 and not r['data']['already_installed'])
    directory = sorted(p.name for p in store.iterdir() if not p.name.startswith('.'))
    record('exactly one installed SDK and empty bookkeeping', len(directory) == 1 and not list((store / '.staging').glob('*')) and not list((store / '.locks').glob('*')), directory)
    sdk_dir = directory[0] if directory else ''
    call('sdk verify', ['sdk', 'verify', sdk_dir, '--store', str(store)], check=lambda r: r['data']['verified'])
    call('reinstall is idempotent', ['sdk', 'install', '--archive', str(inputs / archive['name']), '--release-manifest', str(manifest_path),
         '--release-manifest-sha256', args.manifest_sha256, '--store', str(store)], check=lambda r: r['data']['already_installed'])
    call('a wrong manifest pin is refused', ['sdk', 'install', '--archive', str(inputs / archive['name']), '--release-manifest', str(manifest_path),
         '--release-manifest-sha256', '0' * 64, '--store', str(scratch / 'wrong-pin-store')], expect_ok=False)
    record('the refused install published nothing', not (scratch / 'wrong-pin-store').exists() or not [p for p in (scratch / 'wrong-pin-store').iterdir() if not p.name.startswith('.')])

    # 3. C and C++ first build through the installed SDK, no configure.
    locks = {}
    for language, suffix in (('c', 'c'), ('cxx', 'cxx')):
        project = scratch / f'hello-{language}'
        call(f'{language}: new is source-only', ['new', str(project), '--language', language])
        record(f'{language}: new wrote no binding', not (project / '.crt').exists())
        call(f'{language}: first build without configure', ['build', '--project', str(project), '--sdk', args.content_sha256, *tools],
             check=lambda r: r['data']['cache']['executed'] > 0)
        lock = json.loads((project / 'crt.lock').read_text())
        selection = lock.get('selection', {})
        record(f'{language}: the lock selects the installed SDK and its stored policy',
               lock.get('lock_format') == 1 and selection.get('sdk_content_sha256') == args.content_sha256 and selection.get('contract_sha256') == contract['sha256'] and
               selection.get('contract_source') == 'store' and selection.get('producer_working_tree_dirty', selection.get('provenance', {}).get('producer_working_tree_dirty')) in (False, None),
               json.dumps(selection)[:300])
        locks[language] = (project / 'crt.lock').read_bytes()
        call(f'{language}: test', ['test', '--project', str(project)], check=lambda r: all(o['exit_code'] == 0 for o in r['data']['outcomes']))
        call(f'{language}: run', ['run', '--project', str(project)], check=lambda r: all(o['exit_code'] == 0 for o in r['data']['outcomes']))
        call(f'{language}: warm build executes nothing', ['build', '--project', str(project)],
             check=lambda r: r['data']['cache']['executed'] == 0 and not r['data']['commands'])
        record(f'{language}: the project lock is unchanged by later builds', (project / 'crt.lock').read_bytes() == locks[language])

    # 4. Nothing it consumed changed.
    call('sdk verify after the builds', ['sdk', 'verify', sdk_dir, '--store', str(store)], check=lambda r: r['data']['verified'])
    record('every downloaded input is unchanged', {p.name: digest(p) for p in sorted(inputs.iterdir())} == before_inputs)
    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'cli_sha256': digest(cli), 'sdk_directory': sdk_dir, 'work': str(work),
               'manifest_sha256': args.manifest_sha256, 'content_sha256': args.content_sha256,
               'isolation': 'Seatbelt: no network, writes only to the scratch and private temporary directories, hidden checkouts, cleared environment'}
    (output / 'result.json').write_text(json.dumps(summary, indent=2) + '\n')
    print('passed' if summary['passed'] else 'FAILED', f"{sum(r['passed'] for r in results)}/{len(results)}")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
