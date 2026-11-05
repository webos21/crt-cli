#!/usr/bin/env python3
"""Native macOS D2 isolated replay of `sdk install|list|verify|recover`. Python/sandbox-exec are test-host tools only.

Every CLI run happens under a Seatbelt profile: no network, no writes outside one scratch directory (the SDK store plus a
sentinel tree) and a private temporary directory, the source/producer trees denied for reading and writing, and a
cleared environment (nothing in PATH). Seatbelt cannot make the root read-only or give each run a private /tmp the way
bubblewrap does, so the write boundary is the deny rule; escapes are additionally searched for afterwards. macOS has no PID
namespaces, so lock and transaction owners are ordinary PIDs. The inputs are a real release (archive + manifest) from
`crt-dev-release`; hostile archives are generated here. Evidence is JSON under --output.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tarfile
import tempfile
import time
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import replay_sdk_contract_cases as contract_cases


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def tree(path):
    return sorted(str(p.relative_to(path)) for p in Path(path).rglob('*')) if Path(path).exists() else []


def published(store):
    return sorted(p.name for p in Path(store).iterdir() if not p.name.startswith('.')) if Path(store).exists() else []


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True, help='crt-dev-release output holding the dev archive and manifest')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work-parent', type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    for name in ('cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True, help=f'absolute native {name} for the project workflow')
    args = parser.parse_args()
    tools = {name: getattr(args, name) for name in ('cc', 'cxx', 'linker', 'ar')}
    sandbox = '/usr/bin/sandbox-exec'
    assert sys.platform == 'darwin' and Path(sandbox).exists(), 'macOS sandbox-exec is required'
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='d2-native-', dir=args.work_parent.resolve()))
    hidden = [p.resolve() for p in args.hide]
    assert all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden), 'work must be outside hidden inputs'
    (work / 'tmp').mkdir()
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    inputs = work / 'inputs'; inputs.mkdir()
    manifest_src = next(args.release_dir.glob('release-manifest-dev-macos-*.json'))
    manifest = json.loads(manifest_src.read_text())
    asset = next(a for a in manifest['assets'] if 'install' in a)
    contract_asset = next(a for a in manifest['assets'] if a['name'] == asset['contract'])
    shutil.copy2(args.release_dir / asset['name'], inputs / asset['name'])
    shutil.copy2(args.release_dir / contract_asset['name'], inputs / contract_asset['name'])
    shutil.copy2(manifest_src, inputs / manifest_src.name)
    archive, manifest_path = inputs / asset['name'], inputs / manifest_src.name
    scratch = work / 'scratch'; scratch.mkdir()
    (scratch / 'sentinel').mkdir(); (scratch / 'sentinel' / 'keep').write_text('keep')
    # $HOME/.crt/sdks is where `configure` looks for installed SDKs, so the store lives under a writable HOME.
    home = scratch / 'home'; home.mkdir()
    results = []
    sandbox_digest_before = {p.name: digest(p) for p in inputs.iterdir()}

    counter = [0]

    def command(tail, store, readonly=False, binds=()):
        counter[0] += 1
        writable = [work / 'tmp'] + ([] if readonly else [scratch])
        text = '(version 1)\n(allow default)\n(deny network*)\n'
        for path in hidden:
            text += '(deny file-read* file-write* (subpath ' + json.dumps(str(path)) + '))\n'
        text += '(deny file-write* (require-not (require-any (subpath "/dev") ' + ' '.join(
            '(subpath ' + json.dumps(str(path)) + ')' for path in writable) + ')))\n'
        profile = work / f'policy-{counter[0]}.sb'
        profile.write_text(text)
        return [sandbox, '-f', str(profile), str(cli), '--json', *tail]

    environment = {'HOME': str(home), 'PATH': '/nonexistent', 'TMPDIR': str(work / 'tmp'), 'LC_ALL': 'C'}

    def run(label, tail, store, expect_ok=True, expect_text=None, readonly=False, binds=(), check=None):
        proc = subprocess.run(command(tail, store, readonly, binds), capture_output=True, text=True, timeout=600, env=environment, cwd=work)
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
        results.append({'case': label, 'passed': passed, 'exit': proc.returncode, 'detail': message[:400]})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)
        return reply

    def install_tail(store, extra=()):
        return ['sdk', 'install', '--archive', str(archive), '--release-manifest', str(manifest_path),
                '--release-manifest-sha256', digest(manifest_path), '--store', str(store), *extra]

    sentinel_before = tree(scratch / 'sentinel')

    # 1. Installation from the pinned manifest, with no network and nothing on PATH.
    store = home / '.crt' / 'sdks'
    reply = run('install from release manifest', install_tail(store), store,
                check=lambda r: r['data']['identity']['content_sha256'] == asset['install']['content_sha256'] and not r['data']['already_installed'])
    entries = published(store)
    results.append({'case': 'exactly one SDK directory, bookkeeping empty', 'passed': len(entries) == 1 and not tree(store / '.staging') and not tree(store / '.locks'), 'detail': str(entries)})
    sdk_dir = entries[0] if entries else ''
    run('list', ['sdk', 'list', '--store', str(store)], store, check=lambda r: [e['directory'] for e in r['data']['sdks']] == entries)
    run('verify', ['sdk', 'verify', sdk_dir, '--store', str(store)], store, check=lambda r: r['data']['verified'])
    run('reinstall is idempotent', install_tail(store), store, check=lambda r: r['data']['already_installed'])

    # 2. The installed bytes really are the promised identity (recomputed on the host, outside the CLI).
    sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'crt' / 'tools'))
    try:
        import crt_sdk_install_metadata as meta
        host_identity = meta.content_sha256(meta.inventory(store / sdk_dir))
    except Exception as error:  # the producer module is optional for the replay
        host_identity = f'unavailable: {error}'
    results.append({'case': 'host recomputation of the installed identity', 'passed': host_identity == asset['install']['content_sha256'], 'detail': host_identity})

    # 3. Damage after installation is detected and never repaired silently.
    victim = next(p for p in (store / sdk_dir).rglob('*') if p.is_file() and not p.is_symlink() and p.stat().st_size > 0)
    original = victim.read_bytes(); mode = victim.stat().st_mode
    victim.chmod(0o644); victim.write_bytes(original[:-1] + bytes([original[-1] ^ 1]))
    run('verify detects a modified file', ['sdk', 'verify', sdk_dir, '--store', str(store)], store, expect_ok=False, expect_text='no longer matches')
    run('reinstall refuses to replace a damaged SDK', install_tail(store), store, expect_ok=False, expect_text='immutable')
    victim.write_bytes(original); victim.chmod(mode)
    run('verify passes after restoring the byte', ['sdk', 'verify', sdk_dir, '--store', str(store)], store)

    def fresh_store(name):
        path = scratch / name; shutil.rmtree(path, ignore_errors=True); return path

    # 3b. The release's toolchain contract is kept beside the SDK; a project selects both without --sdk-contract.
    def call(label, tail, **kw):
        kw.pop('tools', None)
        return run(label, tail, kw.pop('store', store), **kw)
    contract_cases.contract_cases(call=call, results=results, store=store, sdk_dir=sdk_dir, contract=contract_asset, archive=archive,
                                  manifest_path=manifest_path, inputs=inputs, work=work, scratch=scratch, fresh_store=fresh_store,
                                  digest=digest, tree=tree, published=published, tools=tools, os_kind='macos-xcode-sdk')

    # 4. Tampered inputs never publish anything.
    def tampered(name, mutate):
        directory = work / name; directory.mkdir()
        shutil.copy2(manifest_path, directory / manifest_path.name); shutil.copy2(inputs / contract_asset['name'], directory / contract_asset['name'])
        target = directory / archive.name; shutil.copy2(archive, target); mutate(target); return directory, target
    flip, flipped = tampered('tampered-archive', lambda p: p.write_bytes(bytearray(p.read_bytes())[:1000] + bytes([p.read_bytes()[1000] ^ 1]) + p.read_bytes()[1001:]))
    grown, grown_path = tampered('grown-archive', lambda p: p.open('ab').write(b'x'))
    for label, directory, text in (('flipped archive byte', flip, 'SHA-256 mismatch'), ('archive with a trailing byte', grown, 'size differs')):
        s = fresh_store('store-' + directory.name)
        argv = ['sdk', 'install', '--archive', str(directory / archive.name), '--release-manifest', str(directory / manifest_path.name),
                '--release-manifest-sha256', digest(manifest_path), '--store', str(s)]
        run(label + ' is refused', argv, s, expect_ok=False, expect_text=text, binds=(directory,))
        results.append({'case': label + ': nothing published', 'passed': published(s) == [] and not tree(s / '.staging'), 'detail': str(tree(s))})
    s = fresh_store('store-wrong-manifest-digest')
    run('wrong pinned manifest digest is refused', ['sdk', 'install', '--archive', str(archive), '--release-manifest', str(manifest_path),
        '--release-manifest-sha256', '0' * 64, '--store', str(s)], s, expect_ok=False, expect_text='release manifest SHA-256 mismatch')
    results.append({'case': 'wrong manifest digest: store untouched', 'passed': published(s) == [], 'detail': str(tree(s))})

    # 5. Hostile archives (digest given directly) never write outside the store and never publish.
    hostile = work / 'hostile'; hostile.mkdir()
    def tar_with(name, members):
        path = hostile / name
        with tarfile.open(path, 'w:xz') as t:
            for info, data in members:
                t.addfile(info, data)
        return path
    def info(name, type_=tarfile.REGTYPE, link='', data=b''):
        i = tarfile.TarInfo(name); i.type = type_; i.linkname = link; i.size = len(data) if type_ == tarfile.REGTYPE else 0; i.mode = 0o644; return i
    import io
    cases = {
        'path traversal': [(info('../escaped', data=b'x'), io.BytesIO(b'x'))],
        'absolute path': [(info('/tmp/escaped', data=b'x'), io.BytesIO(b'x'))],
        'absolute symlink': [(info('l', tarfile.SYMTYPE, '/etc'), None)],
        'escaping symlink': [(info('l', tarfile.SYMTYPE, '../../etc'), None)],
        'write through symlink': [(info('d', tarfile.SYMTYPE, 'x'), None), (info('d/f', data=b'x'), io.BytesIO(b'x'))],
        'hard link': [(info('a', data=b'x'), io.BytesIO(b'x')), (info('b', tarfile.LNKTYPE, 'a'), None)],
        'device node': [(info('dev', tarfile.CHRTYPE), None)],
        'case collision': [(info('A', data=b'1'), io.BytesIO(b'1')), (info('a', data=b'2'), io.BytesIO(b'2'))],
        'not an SDK': [(info('only-file', data=b'x'), io.BytesIO(b'x'))],
    }
    for label, members in cases.items():
        path = tar_with(label.replace(' ', '-') + '.tar.xz', members)
        s = fresh_store('store-hostile')
        run(f'hostile archive refused: {label}', ['sdk', 'install', '--archive', str(path), '--archive-sha256', digest(path), '--store', str(s)],
            s, expect_ok=False, binds=(hostile,))
        results.append({'case': f'hostile {label}: nothing published, no staging left', 'passed': published(s) == [] and not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s))})
    escaped = [str(p) for p in scratch.rglob('*') if p.name in {'escaped'}] + [str(p) for p in Path('/tmp').glob('escaped')]
    results.append({'case': 'nothing escaped the store; sentinel tree unchanged', 'passed': not escaped and tree(scratch / 'sentinel') == sentinel_before, 'detail': str(escaped)})

    # 6. Interruption: SIGKILL during extraction leaves no SDK; recover removes the leftovers; the next install succeeds.
    s = fresh_store('store-interrupted')
    proc = subprocess.Popen(command(install_tail(s), s), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True, env=environment, cwd=work)
    deadline = time.time() + 120
    while time.time() < deadline and proc.poll() is None and not any((s / '.staging').glob('*/txn.json')):
        time.sleep(0.02)
    time.sleep(1.0)
    interrupted = proc.poll() is None
    if interrupted:
        # The CLI (and anything else started for this store) is found by its command line and killed outright.
        listing = subprocess.run(['/bin/ps', '-axo', 'pid=,command='], capture_output=True, text=True).stdout
        for line in listing.splitlines():
            pid, _, command_line = line.strip().partition(' ')
            if pid.isdigit() and int(pid) != os.getpid() and str(s) in command_line:
                try:
                    os.kill(int(pid), signal.SIGKILL)
                except OSError:
                    pass
    proc.wait()
    time.sleep(0.5)
    results.append({'case': 'install was interrupted mid-extraction', 'passed': interrupted, 'detail': f'staging={tree(s / ".staging")[:3]}'})
    results.append({'case': 'interruption published nothing', 'passed': published(s) == [], 'detail': str(published(s))})
    left = tree(s / '.staging') + tree(s / '.locks')
    results.append({'case': 'interruption left recoverable leftovers', 'passed': bool(left), 'detail': str(left[:4])})
    run('recover removes the dead transaction', ['sdk', 'recover', '--store', str(s)], s, check=lambda r: len(r['data']['removed']) >= 1)
    results.append({'case': 'bookkeeping empty after recover', 'passed': not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s))})
    run('install after the interruption', install_tail(s), s, check=lambda r: not r['data']['already_installed'])
    run('verify after the interruption', ['sdk', 'verify', sdk_dir, '--store', str(s)], s)

    # 7. Two concurrent installs of the same SDK: both succeed, one SDK, no leftovers.
    s = fresh_store('store-concurrent')
    procs = [subprocess.Popen(command(install_tail(s, ['--lock-wait', '300']), s), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, start_new_session=True, env=environment, cwd=work) for _ in range(2)]
    outs = [p.communicate(timeout=900) for p in procs]
    replies = [json.loads(o[0]) for o in outs]
    results.append({'case': 'concurrent installs both succeed', 'passed': all(p.returncode == 0 for p in procs) and all(r['ok'] for r in replies),
                    'detail': str([r['data'].get('already_installed') for r in replies])})
    results.append({'case': 'concurrent installs: one new, one already installed', 'passed': sorted(r['data']['already_installed'] for r in replies) == [False, True], 'detail': ''})
    results.append({'case': 'concurrent installs: one SDK, bookkeeping empty', 'passed': published(s) == [sdk_dir] and not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s)[:4])})

    # 8. A read-only store fails cleanly.
    s = fresh_store('store-readonly'); s.mkdir()
    run('read-only store is refused', install_tail(s), s, expect_ok=False, readonly=True)

    # 9. The same SDK as a zip has the same identity (zip made with the release tooling's layout).
    zip_path = work / 'zip' / (archive.name.replace('.tar.xz', '.zip')); zip_path.parent.mkdir()
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as z:
        root = store / sdk_dir
        z.writestr('dev/', b'')
        for p in sorted(root.rglob('*')):
            rel = 'dev/' + p.relative_to(root).as_posix()
            if p.is_symlink():
                zi = zipfile.ZipInfo(rel); zi.create_system = 3; zi.external_attr = (0o120777 << 16); z.writestr(zi, os.readlink(p))
            elif p.is_dir():
                z.writestr(rel + '/', b'')
            else:
                zi = zipfile.ZipInfo(rel); zi.create_system = 3; zi.external_attr = ((0o100755 if p.stat().st_mode & 0o100 else 0o100644) << 16); zi.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(zi, p.read_bytes())
    s = fresh_store('store-zip')
    run('zip of the same SDK installs with the same identity', ['sdk', 'install', '--archive', str(zip_path), '--archive-sha256', digest(zip_path), '--store', str(s)],
        s, binds=(zip_path.parent,), check=lambda r: r['data']['identity']['content_sha256'] == asset['install']['content_sha256'] and r['data']['extracted']['format'] == 'zip')
    run('zip install verifies', ['sdk', 'verify', sdk_dir, '--store', str(s)], s)

    results.append({'case': 'inputs unchanged by every run', 'passed': {p.name: digest(p) for p in inputs.iterdir()} == sandbox_digest_before, 'detail': ''})
    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'sdk_directory': sdk_dir, 'cli_sha256': digest(cli),
               'archive': asset['name'], 'archive_sha256': asset['sha256'], 'work': str(work),
               'isolation': 'Seatbelt: no network, writes only to the scratch and private temporary directories, hidden source trees, cleared env'}
    (output / 'result.json').write_text(json.dumps(summary, indent=2) + '\n')
    print('passed' if summary['passed'] else 'FAILED', f"{sum(r['passed'] for r in results)}/{len(results)}")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
