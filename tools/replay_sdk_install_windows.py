#!/usr/bin/env python3
"""Native Windows D2 replay of `sdk install|list|verify|recover`. Python is a test-host tool only, never a product input.

Counterpart of replay_sdk_install_linux.py / replay_sdk_install_macos.py. Every CLI call runs under the restricted token
of CRT's crt_windows_external/crt_windows_trace (a per-run SID, an empty PATH, redirected stdio, a job object that records
each process the call starts). The work directory, which holds the SDK store and a sentinel tree, is private to that SID; the
CRT/CLI checkouts and the producer trees are proven unreadable by a native probe built with the SDK under test. Windows
offers no network namespace and no read-only root, so network denial is NOT claimed: the CLI is not given a network
option for `sdk install`, and the process accounting shows only the CLI and conhost.exe. Inputs are a real release
(archive + manifest) from `crt-dev-release`; hostile archives are generated here. Evidence is JSON under --output.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import threading
import time
import zipfile


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
    parser.add_argument('--work', type=Path, required=True, help='fresh private work directory (must not exist)')
    parser.add_argument('--probe', type=Path, required=True, help='SDK-built executable that must fail to read every --hide root')
    parser.add_argument('--crt-tools', type=Path, required=True, help='tools/ of the producer checkout (crt_windows_* and the metadata module)')
    parser.add_argument('--hide', type=Path, action='append', default=[])
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    sys.path.insert(0, str(args.crt_tools.resolve()))
    import crt_windows_external as sandbox
    import crt_windows_trace as trace

    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    work, sid = sandbox.create_work(args.work)
    hidden = sorted({p.resolve(strict=True) for p in args.hide})
    assert hidden and all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden), 'work must be outside hidden inputs'
    (work / 'tmp').mkdir(); (work / 'home').mkdir()
    cli, probe = work / 'crt-cli.exe', work / 'probe.exe'
    shutil.copy2(args.cli, cli); shutil.copy2(args.probe, probe)
    inputs = work / 'inputs'; inputs.mkdir()
    manifest_src = next(args.release_dir.glob('release-manifest-dev-windows-*.json'))
    manifest = json.loads(manifest_src.read_text(encoding='utf-8'))
    asset = next(a for a in manifest['assets'] if 'install' in a)
    shutil.copy2(args.release_dir / asset['name'], inputs / asset['name'])
    shutil.copy2(manifest_src, inputs / manifest_src.name)
    archive, manifest_path = inputs / asset['name'], inputs / manifest_src.name
    scratch = work / 'scratch space 한글'; scratch.mkdir()
    (scratch / 'sentinel').mkdir(); (scratch / 'sentinel' / 'keep').write_text('keep')
    input_digests = {p.name: digest(p) for p in inputs.iterdir()}
    permitted = {cli.resolve(), probe.resolve(), Path(os.environ['SystemRoot'], 'System32/conhost.exe').resolve()}
    environment = {'HOME': str(work / 'home'), 'USERPROFILE': str(work / 'home'), 'PATH': '', 'SystemRoot': os.environ['SystemRoot'],
                   'LC_ALL': 'C', 'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp')}
    results, commands = [], []
    counter = [0]

    def readonly(path):
        """Grant this run's SID read access only on a directory created here; the orchestrator keeps full control."""
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

    def launch(argv, timeout=600):
        counter[0] += 1
        label = f'call-{counter[0]:03d}'
        out, err = output / (label + '.stdout'), output / (label + '.stderr')
        result = trace.run([str(cli), '--json', *map(str, argv)], work=work, sid=sid, env=environment, cwd=work,
                           stdout=out, stderr=err, timeout=timeout)
        stray = [p for p in result['processes'] if not p['image'] or Path(p['image']).resolve() not in permitted]
        commands.append({'label': label, 'argv': [str(x) for x in argv], 'exit': result['exit_code'], 'timed_out': result['timed_out'],
                         'images': sorted({str(p['image']) for p in result['processes']})})
        return result, out.read_bytes(), stray

    def reply_of(stdout, result):
        try:
            return json.loads(stdout)
        except Exception:
            return {'ok': result['exit_code'] == 0, 'data': {'message': stdout.decode('utf-8', 'replace')[:300]}}

    def run(label, tail, expect_ok=True, expect_text=None, check=None):
        result, stdout, stray = launch(tail)
        reply = reply_of(stdout, result)
        message = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (result['exit_code'] == 0) == expect_ok and bool(reply.get('ok')) == expect_ok and not result['timed_out'] and not stray
        if expect_text:
            passed = passed and expect_text in message
        if check:
            passed = passed and bool(check(reply))
        results.append({'case': label, 'passed': passed, 'exit': result['exit_code'], 'detail': message[:400] + (f' stray={stray[:2]}' if stray else '')})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)
        return reply

    def save(final=False):
        summary = {'passed': final and all(r['passed'] for r in results), 'completed': final, 'cases': results, 'cli_sha256': digest(cli),
                   'archive': asset['name'], 'archive_sha256': asset['sha256'], 'work': str(work), 'hidden': list(map(str, hidden)),
                   'isolation': 'Windows restricted token (per-run SID, empty PATH), private work directory, job-object process accounting; '
                                'no network denial and no read-only root are claimed'}
        (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        return summary

    def install_tail(store, extra=(), archive_path=None):
        return ['sdk', 'install', '--archive', archive_path or archive, '--release-manifest', manifest_path,
                '--release-manifest-sha256', digest(manifest_path), '--store', store, *extra]

    # 0. The hidden trees are really unreadable under this SID before anything is trusted.
    boundary = [str(probe)]
    for path in hidden:
        boundary += ['--read', str(path)]
    probe_result = trace.run(boundary, work=work, sid=sid, env=environment, cwd=work, stdout=output / 'probe.stdout', stderr=output / 'probe.stderr', timeout=60)
    results.append({'case': 'hidden roots are unreadable under the run SID', 'passed': probe_result['exit_code'] == 0, 'detail': f'{len(hidden)} roots'})
    save()
    assert probe_result['exit_code'] == 0, 'boundary probe failed'
    sentinel_before = tree(scratch / 'sentinel')

    # 1. Installation from the pinned manifest, no PATH, no shell.
    store = scratch / 'store 한글'
    run('install from release manifest', install_tail(store),
        check=lambda r: r['data']['identity']['content_sha256'] == asset['install']['content_sha256'] and not r['data']['already_installed'])
    entries = published(store)
    results.append({'case': 'exactly one SDK directory, bookkeeping empty', 'passed': len(entries) == 1 and not tree(store / '.staging') and not tree(store / '.locks'), 'detail': str(entries)})
    sdk_dir = entries[0] if entries else ''
    run('list', ['sdk', 'list', '--store', store], check=lambda r: [e['directory'] for e in r['data']['sdks']] == entries)
    run('verify', ['sdk', 'verify', sdk_dir, '--store', store], check=lambda r: r['data']['verified'])
    run('reinstall is idempotent', install_tail(store), check=lambda r: r['data']['already_installed'])
    save()

    # 2. The installed bytes really are the promised identity (recomputed on the host, outside the CLI).
    try:
        import crt_sdk_install_metadata as meta
        host_identity = meta.content_sha256(meta.inventory(store / sdk_dir))
    except Exception as error:
        host_identity = f'unavailable: {error}'
    results.append({'case': 'host recomputation of the installed identity', 'passed': host_identity == asset['install']['content_sha256'], 'detail': host_identity})

    # 3. Damage after installation is detected and never repaired silently.
    victim = next(p for p in (store / sdk_dir).rglob('*') if p.is_file() and not p.is_symlink() and p.stat().st_size > 0)
    original = victim.read_bytes()
    victim.write_bytes(original[:-1] + bytes([original[-1] ^ 1]))
    run('verify detects a modified file', ['sdk', 'verify', sdk_dir, '--store', store], expect_ok=False, expect_text='no longer matches')
    run('reinstall refuses to replace a damaged SDK', install_tail(store), expect_ok=False, expect_text='immutable')
    victim.write_bytes(original)
    run('verify passes after restoring the byte', ['sdk', 'verify', sdk_dir, '--store', store])
    save()

    # 4. Tampered inputs never publish anything.
    def fresh_store(name):
        path = scratch / name; shutil.rmtree(path, ignore_errors=True); return path

    def tampered(name, mutate):
        directory = work / name; directory.mkdir()
        shutil.copy2(manifest_path, directory / manifest_path.name)
        target = directory / archive.name; shutil.copy2(archive, target); mutate(target); return directory

    def flip_byte(p):
        data = bytearray(p.read_bytes()); data[1000] ^= 1; p.write_bytes(bytes(data))

    for label, directory, text in (('flipped archive byte', tampered('tampered-archive', flip_byte), 'SHA-256 mismatch'),
                                   ('archive with a trailing byte', tampered('grown-archive', lambda p: p.open('ab').write(b'x')), 'size differs')):
        s = fresh_store('store-' + directory.name)
        run(label + ' is refused', ['sdk', 'install', '--archive', directory / archive.name, '--release-manifest', directory / manifest_path.name,
                                    '--release-manifest-sha256', digest(manifest_path), '--store', s], expect_ok=False, expect_text=text)
        results.append({'case': label + ': nothing published', 'passed': published(s) == [] and not tree(s / '.staging'), 'detail': str(tree(s))})
    s = fresh_store('store-wrong-manifest-digest')
    run('wrong pinned manifest digest is refused', ['sdk', 'install', '--archive', archive, '--release-manifest', manifest_path,
        '--release-manifest-sha256', '0' * 64, '--store', s], expect_ok=False, expect_text='release manifest SHA-256 mismatch')
    results.append({'case': 'wrong manifest digest: store untouched', 'passed': published(s) == [], 'detail': str(tree(s))})
    save()

    # 5. Hostile archives (digest given directly) never write outside the store and never publish.
    hostile = work / 'hostile'; hostile.mkdir()

    def tar_info(name, type_=tarfile.REGTYPE, link='', data=b''):
        i = tarfile.TarInfo(name); i.type = type_; i.linkname = link; i.size = len(data) if type_ == tarfile.REGTYPE else 0; i.mode = 0o644; return i

    def tar_with(name, members):
        path = hostile / name
        with tarfile.open(path, 'w:xz') as t:
            for info, data in members:
                t.addfile(info, data)
        return path

    def zip_with(name, members):
        path = hostile / name
        with zipfile.ZipFile(path, 'w', zipfile.ZIP_STORED) as z:
            for member, data in members:
                z.writestr(member, data)
        return path

    def zip_raw_name(name, raw, data=b'x'):
        info = zipfile.ZipInfo('placeholder'); info.filename = raw  # assigned after construction: no separator normalisation
        return (info, data)

    def two(name, data=b'x'):
        return (tar_info(name, data=data), io.BytesIO(data))

    tar_cases = {
        'path traversal': [two('../escaped')],
        'absolute path': [two('/escaped')],
        'drive-letter path': [two('C:/escaped')],
        'reserved device name': [two('dev/con.txt')],
        'alternate data stream': [two('file.txt:stream')],
        'absolute symlink': [(tar_info('l', tarfile.SYMTYPE, 'C:/Windows'), None)],
        'escaping symlink': [(tar_info('l', tarfile.SYMTYPE, '../../escaped'), None)],
        'write through symlink': [(tar_info('d', tarfile.SYMTYPE, 'x'), None), two('d/f')],
        'hard link': [two('a'), (tar_info('b', tarfile.LNKTYPE, 'a'), None)],
        'device node': [(tar_info('dev', tarfile.CHRTYPE), None)],
        'case collision': [two('A', b'1'), two('a', b'2')],
        'not an SDK': [two('only-file')],
    }
    zip_cases = {
        'backslash traversal': [zip_raw_name('z', '..\\escaped')],
        'zip drive-letter path': [zip_raw_name('z', 'C:/escaped')],
        'zip reserved device name': [zip_raw_name('z', 'dev/NUL.txt')],
    }
    sentinel_names = set()
    for label, members in tar_cases.items():
        path = tar_with(label.replace(' ', '-') + '.tar.xz', members)
        s = fresh_store('store-hostile')
        run(f'hostile archive refused: {label}', ['sdk', 'install', '--archive', path, '--archive-sha256', digest(path), '--store', s], expect_ok=False)
        results.append({'case': f'hostile {label}: nothing published, no staging left', 'passed': published(s) == [] and not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s))})
    for label, members in zip_cases.items():
        path = zip_with(label.replace(' ', '-') + '.zip', members)
        s = fresh_store('store-hostile')
        run(f'hostile archive refused: {label}', ['sdk', 'install', '--archive', path, '--archive-sha256', digest(path), '--store', s], expect_ok=False)
        results.append({'case': f'hostile {label}: nothing published, no staging left', 'passed': published(s) == [] and not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s))})
    # The hostile members are all named exactly "escaped"; an installed SDK legitimately holds names such as escaped_output_table.h.
    escaped = [str(p) for base in (work, Path(os.environ['SystemDrive'] + '/')) for p in base.glob('escaped')] + \
              [str(p) for p in scratch.rglob('escaped')]
    results.append({'case': 'nothing escaped the store; sentinel tree unchanged', 'passed': not escaped and tree(scratch / 'sentinel') == sentinel_before, 'detail': str(escaped)})
    save()

    # 6. Interruption: the transaction's recorded PID is killed during extraction; recover removes the leftovers.
    s = fresh_store('store-interrupted')
    holder = {}

    def interrupted_install():
        holder['result'] = launch(install_tail(s))

    thread = threading.Thread(target=interrupted_install); thread.start()
    deadline = time.time() + 120
    record = None
    while time.time() < deadline and thread.is_alive() and record is None:
        record = next((s / '.staging').glob('*/txn.json'), None) if (s / '.staging').exists() else None
        time.sleep(0.02)
    time.sleep(1.0)
    interrupted, pid = thread.is_alive(), 0
    if interrupted and record is not None:
        try:
            pid = int(json.loads(record.read_text(encoding='utf-8'))['pid'])
            subprocess.run(['taskkill', '/F', '/T', '/PID', str(pid)], capture_output=True)
        except Exception as error:
            results.append({'case': 'could read the transaction owner', 'passed': False, 'detail': str(error)})
    thread.join(timeout=120)
    time.sleep(0.5)
    results.append({'case': 'install was interrupted mid-extraction (owner killed)', 'passed': interrupted and pid > 0 and not thread.is_alive(), 'detail': f'pid={pid} staging={tree(s / ".staging")[:3]}'})
    results.append({'case': 'interruption published nothing', 'passed': published(s) == [], 'detail': str(published(s))})
    left = tree(s / '.staging') + tree(s / '.locks')
    results.append({'case': 'interruption left recoverable leftovers', 'passed': bool(left), 'detail': str(left[:4])})
    run('recover removes the dead transaction', ['sdk', 'recover', '--store', s], check=lambda r: len(r['data']['removed']) >= 1)
    results.append({'case': 'bookkeeping empty after recover', 'passed': not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s))})
    run('install after the interruption', install_tail(s), check=lambda r: not r['data']['already_installed'])
    run('verify after the interruption', ['sdk', 'verify', sdk_dir, '--store', s])
    save()

    # 7. Two concurrent installs of the same SDK: both succeed, one SDK, no leftovers.
    s = fresh_store('store-concurrent')
    concurrent = []
    threads = [threading.Thread(target=lambda: concurrent.append(launch(install_tail(s, ['--lock-wait', '300']), timeout=900))) for _ in range(2)]
    for t in threads: t.start()
    for t in threads: t.join()
    replies = [reply_of(out, res) for res, out, stray in concurrent]
    results.append({'case': 'concurrent installs both succeed', 'passed': len(replies) == 2 and all(r['exit_code'] == 0 for r, _, _ in concurrent) and all(r.get('ok') for r in replies),
                    'detail': str([r.get('data', {}).get('already_installed') for r in replies])})
    results.append({'case': 'concurrent installs: one new, one already installed', 'passed': sorted(r.get('data', {}).get('already_installed') for r in replies) == [False, True], 'detail': ''})
    results.append({'case': 'concurrent installs: one SDK, bookkeeping empty', 'passed': published(s) == [sdk_dir] and not tree(s / '.staging') and not tree(s / '.locks'), 'detail': str(tree(s)[:4])})
    save()

    # 8. A store that the run SID may only read fails cleanly (the orchestrator created the directory and set its ACL).
    s = fresh_store('store-readonly'); s.mkdir(); readonly(s)
    run('read-only store is refused', install_tail(s), expect_ok=False)
    results.append({'case': 'read-only store: nothing written', 'passed': tree(s) == [], 'detail': str(tree(s))})

    # 9. The same SDK as a tar.xz has the same identity (the release asset is a zip on Windows).
    tar_path = work / 'tar' / (archive.name.rsplit('.', 1)[0] + '.tar.xz'); tar_path.parent.mkdir()
    root = store / sdk_dir
    with tarfile.open(tar_path, 'w:xz') as t:
        info = tarfile.TarInfo('dev'); info.type = tarfile.DIRTYPE; info.mode = 0o755; t.addfile(info)
        for p in sorted(root.rglob('*')):
            rel = 'dev/' + p.relative_to(root).as_posix()
            ti = tarfile.TarInfo(rel)
            if p.is_dir():
                ti.type = tarfile.DIRTYPE; ti.mode = 0o755; t.addfile(ti)
            elif p.is_symlink():
                ti.type = tarfile.SYMTYPE; ti.linkname = os.readlink(p).replace('\\', '/'); t.addfile(ti)
            else:
                data = p.read_bytes(); ti.size = len(data); ti.mode = 0o755 if p.suffix.lower() in ('.exe', '.dll', '.cmd') else 0o644; t.addfile(ti, io.BytesIO(data))
    s = fresh_store('store-tar')
    run('tar.xz of the same SDK installs with the same identity', ['sdk', 'install', '--archive', tar_path, '--archive-sha256', digest(tar_path), '--store', s],
        check=lambda r: r['data']['identity']['content_sha256'] == asset['install']['content_sha256'] and r['data']['extracted']['format'] == 'tar.xz')
    run('tar.xz install verifies', ['sdk', 'verify', sdk_dir, '--store', s])

    results.append({'case': 'inputs unchanged by every run', 'passed': {p.name: digest(p) for p in inputs.iterdir()} == input_digests, 'detail': ''})
    summary = save(final=True)
    print('passed' if summary['passed'] else 'FAILED', f"{sum(r['passed'] for r in results)}/{len(results)}")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
