#!/usr/bin/env python3
"""Pinned HTTPS SDK acquisition against a controlled native Linux TLS server.

Python/OpenSSL/bwrap/strace are test-host tools. The consumer has no source or
server-file access, a cleared environment and writable private HOME only.
Networking is shared for loopback tests; this is not a network-denial sandbox.
"""
import argparse
import copy
import hashlib
import http.server
import json
import os
from pathlib import Path
import shutil
import signal
import ssl
import subprocess
import tempfile
import threading
import time


def sha(data):
    return hashlib.sha256(data).hexdigest()


def compact(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cli', 'release-dir', 'output', 'work-parent', 'cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--allow-dirty-sdk', action='store_true')
    parser.add_argument('--first-build-failure-only', action='store_true', help='focused project-binding failure/retry delta')
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='HTTPS SDK space 한글 ', dir=args.work_parent.resolve()))
    hidden = [path.resolve() for path in args.hide]
    assert all(not work.is_relative_to(path) and not path.is_relative_to(work) for path in hidden)
    assert not work.is_relative_to('/tmp')
    pki = work / 'private-pki'; pki.mkdir()
    scratch = work / 'scratch'; scratch.mkdir()
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    manifest_path = next(args.release_dir.glob('release-manifest-dev-linux-*.json'))
    manifest_bytes = manifest_path.read_bytes(); manifest = json.loads(manifest_bytes)
    archive = next(asset for asset in manifest['assets'] if 'install' in asset)
    policy = next(asset for asset in manifest['assets'] if asset['name'] == archive['contract'])
    archive_bytes = (args.release_dir / archive['name']).read_bytes()
    contract_bytes = (args.release_dir / policy['name']).read_bytes()
    assert sha(archive_bytes) == archive['sha256'] and sha(contract_bytes) == policy['sha256']
    sdk_dir = archive['install']['version'] + '-' + archive['install']['content_sha256'][:12]
    openssl = shutil.which('openssl'); assert openssl

    def openssl_run(*argv):
        subprocess.run([openssl, *argv], cwd=pki, check=True, capture_output=True)

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
            except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
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
                      'contract_sha256': policy['sha256']}, 'timeout_seconds': 10}
    results, commands = [], []
    traces = scratch / 'traces'; traces.mkdir()
    masked = sorted({Path(os.path.realpath(path)) for name in ('python3', 'python', 'sh', 'bash', 'perl', 'ruby', 'cmake', 'ninja')
                     if (path := shutil.which(name))})
    tools = {name: str(getattr(args, name).absolute()) for name in ('cc', 'cxx', 'linker', 'ar')}
    tool_flags = [part for role, path in tools.items() for part in ('--' + role, path)]
    if args.allow_dirty_sdk:
        tool_flags += ['--allow-dirty-sdk', 'true']

    def record(label, passed, detail=''):
        results.append({'case': label, 'passed': bool(passed), 'detail': str(detail)[:600]})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)

    def home(name):
        path = scratch / name; path.mkdir(); return path

    def descriptor(name, value):
        path = scratch / (name + '.json'); path.write_bytes(compact(value)); return path

    def command(tail, user_home, index):
        argv = ['bwrap', '--unshare-user', '--unshare-ipc', '--unshare-uts', '--die-with-parent', '--new-session',
                '--ro-bind', '/', '/', '--proc', '/proc', '--dev', '/dev', '--tmpfs', '/tmp', '--clearenv',
                '--setenv', 'HOME', str(user_home), '--setenv', 'PATH', '/nonexistent']
        for path in [*hidden, pki]:
            argv += ['--tmpfs', str(path)]
        for path in masked:
            argv += ['--ro-bind', '/dev/null', str(path)]
        argv += ['--bind', str(scratch), str(scratch), '--chdir', str(scratch), shutil.which('strace'), '-f', '-qq', '-e', 'trace=execve',
                 '-o', str(traces / f'{index:03}.log'), '--', str(cli), '--json', *tail]
        return argv

    def call(label, tail, user_home, expect=True, message=None):
        index = len(commands); argv = command(tail, user_home, index)
        proc = subprocess.run(argv, capture_output=True, timeout=180)
        commands.append({'case': label, 'argv': argv, 'exit': proc.returncode})
        (output / f'call-{index:03}.stdout').write_bytes(proc.stdout); (output / f'call-{index:03}.stderr').write_bytes(proc.stderr)
        try:
            reply = json.loads(proc.stdout)
        except ValueError:
            reply = {}
        detail = json.dumps(reply.get('data', {}))
        passed = (proc.returncode == 0 if expect else proc.returncode > 0) and reply.get('ok') is expect
        if message:
            passed = passed and message in detail
        record(label, passed, detail)
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

    try:
        if args.first_build_failure_only:
            user_home = home('failed first build HOME 한글'); project = scratch / 'failed-project'
            call('new failure-recovery project', ['new', str(project)], user_home)
            original = (project / 'crt.lock').read_bytes()
            bad = copy.deepcopy(source); bad['manifest']['sha256'] = '0' * 64
            spec = descriptor('failed-network-source', bad)
            call('first-build HTTPS verification failure', ['build', '--project', str(project), '--sdk-source', str(spec), *tool_flags],
                 user_home, expect=False, message='digest_mismatch')
            record('HTTPS failure leaves project unresolved and staging empty', (project / 'crt.lock').read_bytes() == original and
                   not (project / '.crt/local.json').exists() and not bookkeeping(user_home))
            spec = descriptor('good-retry-source', source)
            declaration = json.loads((project / 'crt.json').read_bytes()); saved_declaration = copy.deepcopy(declaration)
            declaration['sdk']['version'] = 'not-declared'; (project / 'crt.json').write_bytes(compact(declaration))
            before = len(hits)
            call('project version mismatch is diagnosed before GETs', ['build', '--project', str(project), '--sdk-source', str(spec), *tool_flags],
                 user_home, expect=False, message='project SDK version differs')
            record('version mismatch keeps unresolved lock without network', len(hits) == before and (project / 'crt.lock').read_bytes() == original)
            (project / 'crt.json').write_bytes(compact(saved_declaration))
            main_file = project / 'src/main/main.cpp'; saved_source = main_file.read_bytes(); main_file.write_bytes(b'#error deliberate compile failure\n')
            before = len(hits)
            call('acquired SDK survives a subsequent compile failure', ['build', '--project', str(project), '--sdk-source', str(spec), *tool_flags], user_home, expect=False)
            record('compile failure keeps project unresolved but SDK reusable', (project / 'crt.lock').read_bytes() == original and
                   not (project / '.crt/local.json').exists() and (user_home / '.crt/sdks' / sdk_dir).is_dir() and
                   hits[before:] == ['/manifest', '/contract', '/archive'] and not bookkeeping(user_home))
            main_file.write_bytes(saved_source); before = len(hits)
            call('fixed source first build reuses the acquired SDK', ['build', '--project', str(project), '--sdk-source', str(spec), *tool_flags], user_home)
            record('successful retry pins identity without redownload', len(hits) == before and
                   json.loads((project / 'crt.lock').read_bytes())['selection']['sdk_content_sha256'] == source['sdk']['content_sha256'])
            call('test after acquisition-failure recovery', ['test', '--project', str(project)], user_home)
            call('run after acquisition-failure recovery', ['run', '--project', str(project)], user_home)
        else:
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
                ('archive path traversal', lambda d: d['archive'].update(name='../escape.tar.xz'), 'plain file'),
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
            missing = list(argv); missing[missing.index('--cc') + 1] = str(scratch / 'no-compiler')
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
            record('bound refusal preserves both pins without GETs', len(hits) == before and (project / 'crt.lock').read_bytes() == lock and (project / '.crt/local.json').read_bytes() == local)
            unavailable = copy.deepcopy(source); unavailable['manifest']['url'] = 'https://localhost:1/manifest'; unavailable['archive']['url'] = 'https://localhost:1/archive'; unavailable['contract_url'] = 'https://localhost:1/contract'
            cached = descriptor('offline-source', unavailable)
            before = len(hits); reply = call('verified installed SDK reuse needs no server', ['sdk', 'acquire', '--source', str(cached)], user_home)
            record('reuse records zero downloads', reply.get('data', {}).get('acquisition', {}).get('downloads') == [] and len(hits) == before)
            second = scratch / 'second'; call('new second project', ['new', str(second)], user_home)
            config = {'format': 0, 'sdk_source': str(cached), **tools}
            if args.allow_dirty_sdk:
                config['allow_dirty_sdk'] = True
            (second / '.sdk.json').write_bytes(compact(config))
            call('plain build acquires from .sdk.json using installed identity', ['build', '--project', str(second)], user_home)
            record('second project reuses the installed SDK without GETs', len(hits) == before and json.loads((second / 'crt.lock').read_bytes())['selection']['sdk_directory'] == sdk_dir)
            wrong_pin = copy.deepcopy(unavailable); wrong_pin['sdk']['contract_sha256'] = '0' * 64
            before = len(hits); call('installed receipt cannot satisfy a different contract pin', ['sdk', 'acquire', '--source', str(descriptor('wrong-reuse-pin', wrong_pin))], user_home, expect=False, message='pinned SDK source')
            record('bad reuse pin does not redownload or replace policy', len(hits) == before and (project / 'crt.lock').read_bytes() == lock)
            store = user_home / '.crt/sdks'; receipt = store / '.receipts' / (sdk_dir + '.json'); receipt_bytes = receipt.read_bytes(); receipt.unlink()
            call('reuse never silently migrates a missing receipt', ['sdk', 'acquire', '--source', str(cached)], user_home, expect=False, message='receipt')
            receipt.write_bytes(receipt_bytes)
            call('reused SDK verifies', ['sdk', 'verify', sdk_dir], user_home)

            # A live fetch is kept; a killed owner is recovered by the existing store.
            interrupted_home = home('killed-fetch'); interrupted = copy.deepcopy(source); interrupted['archive']['url'] = origin + '/kill'
            interrupted_spec = descriptor('kill-source', interrupted); index = len(commands)
            argv = command(['sdk', 'acquire', '--source', str(interrupted_spec)], interrupted_home, index)
            commands.append({'case': 'SIGKILL during HTTPS archive', 'argv': argv, 'exit': None})
            proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert kill_started.wait(30), 'download did not reach the controlled interruption point'
            staged = interrupted_home / '.crt/sdks/.staging'; transactions = list(staged.iterdir()); assert len(transactions) == 1
            call('recover preserves a live HTTPS transaction', ['sdk', 'recover'], interrupted_home)
            record('live HTTPS staging remains owned', transactions[0].exists())
            pid = json.loads((transactions[0] / 'txn.json').read_bytes())['pid']; os.kill(pid, signal.SIGKILL)
            stdout, stderr = proc.communicate(timeout=30); release_kill.set(); kill_first[0] = False
            commands[index]['exit'] = proc.returncode
            (output / 'killed-fetch.stdout').write_bytes(stdout); (output / 'killed-fetch.stderr').write_bytes(stderr)
            record('killed HTTPS transaction leaves no published SDK', proc.returncode != 0 and transactions[0].exists() and not (interrupted_home / '.crt/sdks' / sdk_dir).exists())
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
        images = []
        for trace in traces.glob('*.log'):
            for line in trace.read_text().splitlines():
                if 'execve(' in line:
                    images.append(line)
        record('execution traces contain no interpreter or shell', not any('"' + str(path) + '"' in line for path in masked for line in images))
        summary = {'passed': bool(results) and all(case['passed'] for case in results), 'cases': results, 'work': str(work),
                   'cli_sha256': sha(cli.read_bytes()), 'source_fixture': source, 'archive_sha256': sha(archive_bytes),
                   'manifest_sha256': sha(manifest_bytes), 'http_requests': hits, 'allow_dirty_sdk': args.allow_dirty_sdk,
                   'isolation': 'bwrap: read-only root, hidden sources/private PKI, cleared env, masked interpreters, private writable scratch; shared loopback network'}
        (output / 'result.json').write_text(json.dumps(summary, indent=2) + '\n')
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        print(f"{sum(case['passed'] for case in results)}/{len(results)} passed")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
