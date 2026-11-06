#!/usr/bin/env python3
"""Pinned HTTPS SDK acquisition cases, native Windows entry point.

Same cases as `replay_sdk_acquire_linux.py`. Python and OpenSSL are test-host tools (throw-away PKI, loopback HTTPS
servers); the consumer starts neither. Each CLI call runs under CRT's `crt_windows_trace` (restricted token naming a
per-run SID, empty PATH, redirected stdio, job-object process accounting) with a private writable HOME. The private
PKI lives outside the work directory and is hidden from the SID; the probe proves it. The network is shared for
loopback tests: this is not a network-denial sandbox, and process records are accounting, not launch denial.
"""
import argparse
import copy
import hashlib
import http.server
import json
import os
from pathlib import Path
import shutil
import ssl
import subprocess
import sys
import threading
import time


def sha(data):
    return hashlib.sha256(data).hexdigest()


def compact(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cli', 'release-dir', 'output', 'work', 'probe', 'crt-tools', 'pki', 'cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--allow-dirty-sdk', action='store_true')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    sys.path.insert(0, str(args.crt_tools.resolve()))
    import crt_windows_external as sandbox
    import crt_windows_trace as trace

    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    work, sid = sandbox.create_work(args.work)
    pki = args.pki.resolve(); pki.mkdir(parents=True, exist_ok=False)
    hidden = sorted({p.resolve(strict=True) for p in [*args.hide, pki]})
    assert all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden)
    scratch = work / 'scratch'; scratch.mkdir()
    (work / 'tmp').mkdir()
    cli, probe = work / 'crt-cli.exe', work / 'probe.exe'
    shutil.copy2(args.cli, cli); shutil.copy2(args.probe, probe)
    manifest_path = next(args.release_dir.glob('release-manifest-dev-windows-*.json'))
    manifest_bytes = manifest_path.read_bytes(); manifest = json.loads(manifest_bytes)
    archive = next(asset for asset in manifest['assets'] if 'install' in asset)
    policy = next(asset for asset in manifest['assets'] if asset['name'] == archive['contract'])
    archive_bytes = (args.release_dir / archive['name']).read_bytes()
    contract_bytes = (args.release_dir / policy['name']).read_bytes()
    assert sha(archive_bytes) == archive['sha256'] and sha(contract_bytes) == policy['sha256']
    sdk_dir = archive['install']['version'] + '-' + archive['install']['content_sha256'][:12]
    openssl = shutil.which('openssl'); assert openssl
    # A minimal explicit configuration: some Windows OpenSSL builds carry no default openssl.cnf.
    config = pki / 'openssl.cnf'; config.write_text('[req]\ndistinguished_name=dn\n[dn]\n')
    openssl_env = dict(os.environ, OPENSSL_CONF=str(config))

    def openssl_run(*argv):
        subprocess.run([openssl, *argv], cwd=pki, check=True, capture_output=True, env=openssl_env)

    for ca in ('good', 'other'):
        openssl_run('req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', ca + '.key', '-out', ca + '.pem',
                    '-days', '2', '-subj', '/CN=CRT acquire test ' + ca, '-addext', 'basicConstraints=critical,CA:TRUE',
                    '-addext', 'keyUsage=keyCertSign,cRLSign')
    for leaf, host in (('good-leaf', 'localhost'), ('wronghost', 'other.example')):
        (pki / (leaf + '.ext')).write_text('subjectAltName=DNS:' + host + '\nbasicConstraints=CA:FALSE\nextendedKeyUsage=serverAuth\n')
        openssl_run('req', '-newkey', 'rsa:2048', '-nodes', '-keyout', leaf + '.key', '-out', leaf + '.csr', '-subj', '/CN=' + host)
        openssl_run('x509', '-req', '-in', leaf + '.csr', '-CA', 'good.pem', '-CAkey', 'good.key', '-CAcreateserial',
                    '-out', leaf + '.pem', '-days', '2', '-extfile', leaf + '.ext')
    ca_file = work / 'test-ca.pem'; shutil.copy2(pki / 'good.pem', ca_file)
    other_ca = work / 'other-ca.pem'; shutil.copy2(pki / 'other.pem', other_ca)
    routes = {'/manifest': manifest_bytes, '/contract': contract_bytes, '/archive': archive_bytes}
    hits = []; kill_started = threading.Event(); release_kill = threading.Event(); kill_first = [True]

    class Handler(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'

        def log_message(self, *_):
            pass

        def do_GET(self):
            hits.append(self.path)
            try:
                if self.path == '/redirect':
                    self.send_response(302); self.send_header('Location', '/manifest'); self.send_header('Content-Length', '0'); self.end_headers(); return
                name = '/archive' if self.path in ('/truncated', '/slow', '/kill') else self.path
                body = routes.get(name)
                if body is None:
                    self.send_response(404); self.send_header('Content-Length', '0'); self.end_headers(); return
                self.send_response(200); self.send_header('Content-Length', str(len(body))); self.end_headers()
                if self.path == '/truncated':
                    self.wfile.write(body[:8192]); self.wfile.flush(); self.close_connection = True; return
                if self.path == '/slow':
                    time.sleep(3)
                if self.path == '/kill' and kill_first[0]:
                    self.wfile.write(body[:8192]); self.wfile.flush(); kill_started.set(); release_kill.wait(45); return
                self.wfile.write(body)
            except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError, ssl.SSLError):
                pass

    servers = []
    for leaf in ('good-leaf', 'wronghost'):
        server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler); server.daemon_threads = True
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER); context.minimum_version = ssl.TLSVersion.TLSv1_2
        context.load_cert_chain(pki / (leaf + '.pem'), pki / (leaf + '.key'))
        server.socket = context.wrap_socket(server.socket, server_side=True)
        threading.Thread(target=server.serve_forever, daemon=True).start(); servers.append(server)
    origin = 'https://localhost:' + str(servers[0].server_port)
    source = {'schema': 'crt-cli.sdk-source', 'format': 0, 'ca_file': str(ca_file), 'ca_sha256': sha(ca_file.read_bytes()),
              'manifest': {'url': origin + '/manifest', 'size': len(manifest_bytes), 'sha256': sha(manifest_bytes)},
              'archive': {'name': archive['name'], 'url': origin + '/archive'}, 'contract_url': origin + '/contract',
              'sdk': {'version': archive['install']['version'], 'content_sha256': archive['install']['content_sha256'],
                      'contract_sha256': policy['sha256']}, 'timeout_seconds': 180}  # the 146 MB archive crosses a single-threaded Python TLS server; the timeout is total, not per read
    results, commands = [], []
    tools = {name: str(getattr(args, name).absolute()) for name in ('cc', 'cxx', 'linker', 'ar')}
    tool_flags = [part for role, path in tools.items() for part in ('--' + role, path)]
    if args.allow_dirty_sdk:
        tool_flags += ['--allow-dirty-sdk', 'true']
    decoys = []
    for name in ('clang.exe', 'clang++.exe', 'ld.lld.exe', 'llvm-ar.exe', 'python.exe', 'sh.exe', 'cmake.exe', 'ninja.exe'):
        shutil.copy2(probe, work / name); decoys.append((work / name).resolve())
    assert os.environ.get('CRT_WINDOWS_SDK_LIBPATH'), 'set CRT_WINDOWS_SDK_LIBPATH'
    permitted = {cli.resolve(), probe.resolve(), Path(os.environ['SystemRoot'], 'System32/conhost.exe').resolve()}
    tool_directories = {Path(p).resolve().parent for p in tools.values()}

    def environment(user_home):
        return {'HOME': str(user_home), 'USERPROFILE': str(user_home), 'PATH': '', 'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C',
                'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'CRT_WINDOWS_SDK_LIBPATH': os.environ['CRT_WINDOWS_SDK_LIBPATH']}

    def record(label, passed, detail=''):
        results.append({'case': label, 'passed': bool(passed), 'detail': str(detail)[:600]})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)

    def home(name):
        path = scratch / name; path.mkdir(); return path

    def descriptor(name, value):
        path = scratch / (name + '.json'); path.write_bytes(compact(value)); return path

    def known(image):
        path = Path(image).resolve()
        return path in permitted or path.parent in tool_directories or path.is_relative_to(scratch.resolve())

    def launch(tail, user_home, index, timeout=300):
        label = f'call-{index:03}'
        out, err = output / (label + '.stdout'), output / (label + '.stderr')
        result = trace.run([str(cli), '--json', *map(str, tail)], work=work, sid=sid, env=environment(user_home), cwd=work,
                           stdout=out, stderr=err, timeout=timeout)
        stray = [p for p in result['processes'] if not p['image'] or not known(p['image'])]
        commands[index].update(exit=result['exit_code'], timed_out=result['timed_out'],
                               images=sorted({str(p['image']) for p in result['processes']}))
        return result, out.read_bytes(), stray

    def call(label, tail, user_home, expect=True, message=None):
        index = len(commands); commands.append({'case': label, 'argv': [str(x) for x in tail]})
        result, stdout, stray = launch(tail, user_home, index)
        try:
            reply = json.loads(stdout)
        except ValueError:
            reply = {}
        detail = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (result['exit_code'] == 0) == expect and reply.get('ok') is expect and not result['timed_out'] and not stray
        if message:
            passed = passed and message in detail
        record(label, passed, detail + (f' stray={stray[:2]}' if stray else ''))
        return reply

    def bookkeeping(user_home):
        store = user_home / '.crt/sdks'
        return [path.name for area in ('.staging', '.locks') if (store / area).exists() for path in (store / area).iterdir()]

    def failed_source(label, value, message=None):
        user_home = home('negative-' + str(len(results)))
        spec = descriptor('source-' + str(len(results)), value)
        before = len(hits)
        call(label, ['sdk', 'acquire', '--source', str(spec)], user_home, expect=False, message=message)
        store = user_home / '.crt/sdks'
        published = [path.name for path in store.iterdir() if not path.name.startswith('.')] if store.exists() else []
        record(label + ': no SDK or staging published', not published and not bookkeeping(user_home))
        return len(hits) - before

    summary = {}
    try:
        boundary = [str(probe)]
        for path in hidden:
            boundary += ['--read', str(path)]
        probe_result = trace.run(boundary, work=work, sid=sid, env=environment(scratch), cwd=work, stdout=output / 'probe.stdout',
                                 stderr=output / 'probe.stderr', timeout=60)
        record('hidden roots and the private PKI are unreadable under the run SID', probe_result['exit_code'] == 0, f'{len(hidden)} roots')
        assert probe_result['exit_code'] == 0
        for label, mutate, message in (
            ('plain HTTP source', lambda d: d['manifest'].update(url='http://localhost/manifest'), 'HTTPS'),
            ('credential URL', lambda d: d['manifest'].update(url='https://user:secret@localhost/manifest'), 'without credentials'),
            ('changed CA pin', lambda d: d.update(ca_sha256='0' * 64), 'ca_sha256'),
            ('untrusted CA', lambda d: d.update(ca_file=str(other_ca), ca_sha256=sha(other_ca.read_bytes())), 'tls'),
            ('wrong-host certificate', lambda d: d['manifest'].update(url='https://localhost:' + str(servers[1].server_port) + '/manifest'), 'tls'),
            ('wrong metadata digest', lambda d: d['manifest'].update(sha256='0' * 64), 'digest_mismatch'),
            ('metadata size cap', lambda d: d['manifest'].update(size=len(manifest_bytes) - 1), None),
            ('metadata size mismatch', lambda d: d['manifest'].update(size=len(manifest_bytes) + 1), 'size_mismatch'),
            ('redirect refused', lambda d: d['manifest'].update(url=origin + '/redirect'), None),
            ('archive path traversal', lambda d: d['archive'].update(name='../escape.zip'), 'plain file'),
            ('unknown source field', lambda d: d.update(verify_peer=False), 'unknown SDK source field'),
            ('interrupted archive', lambda d: d['archive'].update(url=origin + '/truncated'), 'interrupted'),
            ('archive timeout', lambda d: (d['archive'].update(url=origin + '/slow'), d.update(timeout_seconds=1)), 'timeout'),
        ):
            value = copy.deepcopy(source); mutate(value); failed_source(label, value, message)
        for label, route, original in (('modified archive bytes', '/bad-archive', archive_bytes), ('modified contract bytes', '/bad-contract', contract_bytes)):
            body = bytearray(original); body[len(body) // 2] ^= 1; routes[route] = bytes(body)
            value = copy.deepcopy(source)
            if 'archive' in label:
                value['archive']['url'] = origin + route
            else:
                value['contract_url'] = origin + route
            failed_source(label, value, 'digest_mismatch')
        for label, mutate in (
            ('unsupported install metadata format', lambda m: m['assets'][0]['install'].update(format=2)),
            ('foreign target manifest', lambda m: m['assets'][0]['install'].update(target={'os': 'linux', 'arch': 'not-this-host'})),
            ('different SDK identity manifest', lambda m: m['assets'][0]['install'].update(content_sha256='0' * 64)),
            ('different contract pin manifest', lambda m: m['assets'][1].update(sha256='0' * 64)),
            ('duplicate archive asset', lambda m: m['assets'].append(copy.deepcopy(m['assets'][0]))),
        ):
            changed = copy.deepcopy(manifest); mutate(changed); body = compact(changed); routes['/changed-manifest'] = body
            value = copy.deepcopy(source); value['manifest'] = {'url': origin + '/changed-manifest', 'sha256': sha(body), 'size': len(body)}
            count = failed_source(label, value)
            record(label + ': asset downloads never start', count == 1, count)

        # Cold first build must actually download all inputs and compile a project.
        user_home = home('workflow HOME 한글'); spec = descriptor('workflow-source', source); project = scratch / 'hello'
        call('new performs no SDK acquisition', ['new', str(project)], user_home)
        unresolved = (project / 'crt.lock').read_bytes()
        record('new leaves the SDK store absent', not (user_home / '.crt/sdks').exists())
        argv = ['build', '--project', str(project), '--sdk-source', str(spec), *tool_flags]
        missing = list(argv); missing[missing.index('--cc') + 1] = str(scratch / 'no-compiler.exe')
        before = len(hits); call('missing compiler is diagnosed before networking', missing, user_home, expect=False)
        record('missing compiler preserves unresolved state without GETs', len(hits) == before and (project / 'crt.lock').read_bytes() == unresolved)
        call('cold HTTPS acquisition and first build', argv, user_home)
        record('cold first build fetched metadata, contract and archive once', hits[before:] == ['/manifest', '/contract', '/archive'], hits[before:])
        lock, local = (project / 'crt.lock').read_bytes(), (project / '.crt/local.json').read_bytes()
        selection = json.loads(lock)['selection']
        record('first build pins installed SDK and portable contract', selection['sdk_content_sha256'] == source['sdk']['content_sha256'] and
               selection['contract_sha256'] == source['sdk']['contract_sha256'] and selection['source'] == selection['contract_source'] == 'store')
        call('test after HTTPS first build', ['test', '--project', str(project)], user_home)
        call('run after HTTPS first build', ['run', '--project', str(project)], user_home)
        before = len(hits); call('bound build refuses source replacement', argv, user_home, expect=False, message='use configure explicitly')
        record('bound refusal preserves both pins without GETs', len(hits) == before and (project / 'crt.lock').read_bytes() == lock and
               (project / '.crt/local.json').read_bytes() == local)
        unavailable = copy.deepcopy(source); unavailable['manifest']['url'] = 'https://localhost:1/manifest'
        unavailable['archive']['url'] = 'https://localhost:1/archive'; unavailable['contract_url'] = 'https://localhost:1/contract'
        cached = descriptor('offline-source', unavailable)
        before = len(hits); reply = call('verified installed SDK reuse needs no server', ['sdk', 'acquire', '--source', str(cached)], user_home)
        record('reuse records zero downloads', reply.get('data', {}).get('acquisition', {}).get('downloads') == [] and len(hits) == before)
        second = scratch / 'second'; call('new second project', ['new', str(second)], user_home)
        config = {'format': 0, 'sdk_source': str(cached), **tools}
        if args.allow_dirty_sdk:
            config['allow_dirty_sdk'] = True
        (second / '.sdk.json').write_bytes(compact(config))
        call('plain build acquires from .sdk.json using installed identity', ['build', '--project', str(second)], user_home)
        record('second project reuses the installed SDK without GETs', len(hits) == before and
               json.loads((second / 'crt.lock').read_bytes())['selection']['sdk_directory'] == sdk_dir)
        wrong_pin = copy.deepcopy(unavailable); wrong_pin['sdk']['contract_sha256'] = '0' * 64
        before = len(hits); call('installed receipt cannot satisfy a different contract pin',
                                 ['sdk', 'acquire', '--source', str(descriptor('wrong-reuse-pin', wrong_pin))], user_home, expect=False, message='pinned SDK source')
        record('bad reuse pin does not redownload or replace policy', len(hits) == before and (project / 'crt.lock').read_bytes() == lock)
        store = user_home / '.crt/sdks'; receipt = store / '.receipts' / (sdk_dir + '.json'); receipt_bytes = receipt.read_bytes(); receipt.unlink()
        call('reuse never silently migrates a missing receipt', ['sdk', 'acquire', '--source', str(cached)], user_home, expect=False, message='receipt')
        receipt.write_bytes(receipt_bytes)
        call('reused SDK verifies', ['sdk', 'verify', sdk_dir], user_home)

        # A live fetch is kept; a killed owner is recovered by the existing store.
        interrupted_home = home('killed-fetch'); interrupted = copy.deepcopy(source); interrupted['archive']['url'] = origin + '/kill'
        interrupted_spec = descriptor('kill-source', interrupted)
        index = len(commands); commands.append({'case': 'process killed during HTTPS archive', 'argv': ['sdk', 'acquire', '--source', str(interrupted_spec)]})
        holder = {}
        thread = threading.Thread(target=lambda: holder.update(run=launch(['sdk', 'acquire', '--source', str(interrupted_spec)], interrupted_home, index)))
        thread.start()
        assert kill_started.wait(60), 'download did not reach the controlled interruption point'
        staged = interrupted_home / '.crt/sdks/.staging'; transactions = list(staged.iterdir()); assert len(transactions) == 1
        call('recover preserves a live HTTPS transaction', ['sdk', 'recover'], interrupted_home)
        record('live HTTPS staging remains owned', transactions[0].exists())
        pid = json.loads((transactions[0] / 'txn.json').read_bytes())['pid']
        subprocess.run(['taskkill', '/F', '/T', '/PID', str(pid)], capture_output=True)
        thread.join(timeout=60); release_kill.set(); kill_first[0] = False
        result = holder['run'][0]
        record('killed HTTPS transaction leaves no published SDK', result['exit_code'] != 0 and transactions[0].exists() and
               not (interrupted_home / '.crt/sdks' / sdk_dir).exists(), result['exit_code'])
        call('recover removes the dead HTTPS transaction', ['sdk', 'recover'], interrupted_home)
        record('dead HTTPS staging is empty after recovery', not bookkeeping(interrupted_home))
        call('same pinned source succeeds after interrupted-fetch recovery', ['sdk', 'acquire', '--source', str(interrupted_spec)], interrupted_home)
        record('successful acquisition leaves no bookkeeping', not bookkeeping(interrupted_home))
    except Exception as error:
        record('replay completed without a host-harness exception', False, repr(error))
    finally:
        release_kill.set()
        for server in servers:
            server.shutdown(); server.server_close()
        started = {str(Path(image).resolve()) for entry in commands for image in entry.get('images', []) if image}
        record('no decoy interpreter or tool was started', not started & {str(d) for d in decoys})
        summary = {'passed': bool(results) and all(case['passed'] for case in results), 'cases': results, 'work': str(work),
                   'cli_sha256': sha(cli.read_bytes()), 'source_fixture': source, 'archive_sha256': sha(archive_bytes),
                   'manifest_sha256': sha(manifest_bytes), 'http_requests': hits, 'allow_dirty_sdk': args.allow_dirty_sdk,
                   'isolation': 'Windows restricted token (per-run SID, empty PATH), private work directory with decoy executables, '
                                'private PKI outside the work directory, job-object process accounting; shared loopback network, '
                                'no network denial and no read-only root are claimed'}
        (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(f"{sum(case['passed'] for case in results)}/{len(results)} passed")
    return 0 if summary.get('passed') else 1


if __name__ == '__main__':
    raise SystemExit(main())
