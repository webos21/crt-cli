#!/usr/bin/env python3
"""Native replay (Linux and macOS) of the restricted HTTPS redirect policy (fetch and sdk acquire).

Two loopback TLS origins stand in for github.com (the named origin) and release storage (the redirect target); the
real promoted SDK bytes from --release-dir are served behind redirects. No product code is stubbed.
"""
import argparse, copy, hashlib, http.server, json, os, shutil, ssl, subprocess, tempfile, threading
from pathlib import Path

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=False)
    work = Path(tempfile.mkdtemp(prefix='redirect replay 한글 ', dir=out))
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    pki = work / 'pki'; pki.mkdir()
    openssl = shutil.which('openssl'); assert openssl
    def run_ssl(*argv):
        subprocess.run([openssl, *argv], cwd=pki, check=True, capture_output=True)
    run_ssl('req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', 'ca.key', '-out', 'ca.pem', '-days', '2',
            '-subj', '/CN=CRT redirect test', '-addext', 'basicConstraints=critical,CA:TRUE', '-addext', 'keyUsage=keyCertSign,cRLSign')
    for leaf, host in (('good', 'localhost'), ('wrong', 'other.example')):
        (pki / (leaf + '.ext')).write_text('subjectAltName=DNS:' + host + '\nbasicConstraints=CA:FALSE\nextendedKeyUsage=serverAuth\n')
        run_ssl('req', '-newkey', 'rsa:2048', '-nodes', '-keyout', leaf + '.key', '-out', leaf + '.csr', '-subj', '/CN=' + host)
        run_ssl('x509', '-req', '-in', leaf + '.csr', '-CA', 'ca.pem', '-CAkey', 'ca.key', '-CAcreateserial', '-out', leaf + '.pem', '-days', '2', '-extfile', leaf + '.ext')
    ca = work / 'ca.pem'; shutil.copy2(pki / 'ca.pem', ca)

    manifest_path = next(args.release_dir.glob('release-manifest-dev-*.json'))
    manifest_bytes = manifest_path.read_bytes(); manifest = json.loads(manifest_bytes)
    archive = next(a for a in manifest['assets'] if 'install' in a)
    policy = next(a for a in manifest['assets'] if a['name'] == archive['contract'])
    blobs = {'/blob-small': b'redirect payload\n' * 100,
             '/blob-manifest': manifest_bytes,
             '/blob-contract': (args.release_dir / policy['name']).read_bytes(),
             '/blob-archive': (args.release_dir / archive['name']).read_bytes()}
    assert sha(blobs['/blob-archive']) == archive['sha256']
    secret = 'sig-SECRET-TOKEN-0123'
    hits = {'A': [], 'B': [], 'C': []}; headers_seen = {'A': [], 'B': [], 'C': []}
    ports = {}

    def make_handler(name):
        class Handler(http.server.BaseHTTPRequestHandler):
            protocol_version = 'HTTP/1.1'
            def log_message(self, *_): pass
            def redirect(self, status, *locations, cookie=False):
                self.send_response(status)
                for location in locations: self.send_header('Location', location)
                if cookie: self.send_header('Set-Cookie', 'session=abc; Path=/')
                self.send_header('Content-Length', '0'); self.end_headers()
            def do_GET(self):
                hits[name].append(self.path); headers_seen[name].append({k.lower(): v for k, v in self.headers.items()})
                path = self.path.split('?')[0]
                try:
                    if name == 'A':
                        b = 'https://localhost:%d' % ports['B']
                        if path.startswith('/asset/'):  # github.com/<owner>/<repo>/releases/download/<tag>/<asset> shape
                            self.redirect(302, b + path.replace('/asset', '') + '?sig=' + secret, cookie=True); return
                        routes = {
                            '/rel': lambda: self.redirect(302, '/same/blob-small'),
                            '/301': lambda: self.redirect(301, b + '/blob-small'), '/303': lambda: self.redirect(303, b + '/blob-small'),
                            '/307': lambda: self.redirect(307, b + '/blob-small'), '/308': lambda: self.redirect(308, b + '/blob-small'),
                            '/300': lambda: self.redirect(300, b + '/blob-small'), '/304': lambda: self.redirect(304, b + '/blob-small'),
                            '/to-c': lambda: self.redirect(302, 'https://localhost:%d/blob-small' % ports['C']),
                            '/to-wrong-cert': lambda: self.redirect(302, 'https://127.0.0.1:%d/blob-small' % ports['W']),
                            '/to-plain': lambda: self.redirect(302, 'http://localhost:%d/blob-small' % ports['P']),
                            '/to-cred': lambda: self.redirect(302, 'https://user:pw@localhost:%d/blob-small' % ports['B']),
                            '/to-fragment': lambda: self.redirect(302, b + '/blob-small#frag'),
                            '/to-evil-suffix': lambda: self.redirect(302, 'https://evil-localhost:%d/blob-small' % ports['B']),
                            '/to-ipv6': lambda: self.redirect(302, 'https://[::1]:%d/blob-small' % ports['B']),
                            '/no-location': lambda: self.redirect(302),
                            '/two-locations': lambda: self.redirect(302, b + '/blob-small', b + '/blob-small'),
                            '/loop': lambda: self.redirect(302, '/loop'),
                            '/hop1': lambda: self.redirect(302, '/hop2'), '/hop2': lambda: self.redirect(302, '/hop3'),
                            '/hop3': lambda: self.redirect(302, '/hop4'), '/hop4': lambda: self.redirect(302, b + '/blob-small'),
                            '/to-missing': lambda: self.redirect(302, b + '/missing'),
                            '/to-big': lambda: self.redirect(302, b + '/blob-big'),
                            '/to-truncated': lambda: self.redirect(302, b + '/truncated'),
                            '/to-redirect-b': lambda: self.redirect(302, b + '/bounce'),
                        }
                        if path.startswith('/same/'): path = path[5:]
                        elif path in routes: routes[path](); return
                    if path == '/bounce':
                        self.redirect(302, 'https://localhost:%d/blob-small' % ports['C']); return
                    body = blobs.get(path)
                    if path == '/blob-big': body = b'x' * 200000
                    if path == '/truncated':
                        self.send_response(200); self.send_header('Content-Length', '100000'); self.end_headers()
                        self.wfile.write(b'y' * 1000); self.wfile.flush(); self.close_connection = True; return
                    if body is None:
                        self.send_response(404); self.send_header('Content-Length', '0'); self.end_headers(); return
                    self.send_response(200); self.send_header('Content-Length', str(len(body))); self.end_headers(); self.wfile.write(body)
                except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
                    pass
        return Handler

    servers = []
    def serve(name, leaf, tls=True):
        class Quiet(http.server.ThreadingHTTPServer):
            def handle_error(self, *_): pass  # clients that abort a transfer are part of the cases
        server = Quiet(('127.0.0.1', 0), make_handler(name)); server.daemon_threads = True
        if tls:
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER); context.minimum_version = ssl.TLSVersion.TLSv1_2
            context.load_cert_chain(pki / (leaf + '.pem'), pki / (leaf + '.key'))
            server.socket = context.wrap_socket(server.socket, server_side=True)
        threading.Thread(target=server.serve_forever, daemon=True).start(); servers.append(server)
        return server.server_port
    for name in ('A', 'B', 'C'): ports[name] = serve(name, 'good')
    ports['W'] = serve('C', 'wrong'); ports['P'] = serve('C', 'good', tls=False)
    A = 'https://localhost:%d' % ports['A']
    home = work / 'home'; home.mkdir()
    env = {'HOME': str(home), 'PATH': '/nonexistent', 'TMPDIR': str(work)}
    results = []; commands = []

    def call(argv):
        p = subprocess.run([str(cli), '--json', *map(str, argv)], capture_output=True, text=True, env=env, stdin=subprocess.DEVNULL, timeout=600)
        commands.append({'argv': [str(a) for a in argv], 'exit': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr})
        try: reply = json.loads(p.stdout)
        except ValueError: reply = {}
        return p, reply

    def record(label, passed, detail=''):
        results.append({'case': label, 'passed': bool(passed), 'detail': detail})
        print(('PASS ' if passed else 'FAIL ') + label + (' -- ' + detail if detail and not passed else ''))

    small = blobs['/blob-small']
    def fetch(label, path, policy=('--redirect-hosts', 'localhost:%d' % ports['B']), expect_ok=True, code=None, extra=(), body=small):
        output = work / ('fetched-' + str(len(results)))
        argv = ['fetch', '--url', A + path, '--ca-file', ca, '--output', output, '--sha256', sha(body), '--size', len(body), '--timeout', 10, *policy, *extra]
        p, reply = call(argv)
        data = reply.get('data', {})
        ok = (p.returncode == 0) == expect_ok
        if expect_ok: ok = ok and output.read_bytes() == body and data.get('published') is True
        else: ok = ok and not output.exists() and not list(work.glob(output.name + '.part-*')) and (code is None or data.get('code') == code)
        leaked = secret in p.stdout + p.stderr or 'user:pw' in p.stdout + p.stderr
        record(label, ok and not leaked, p.stdout[:400] if not ok else ('secret leaked' if leaked else ''))
        return data

    # Default behavior and the exact policy.
    fetch('no policy: redirect refused (http_status 302)', '/asset/blob-small', policy=(), expect_ok=False, code='http_status')
    data = fetch('cross-origin redirect to allow-listed authority', '/asset/blob-small')
    record('redirect recorded by authority only', data.get('trust', {}).get('redirects', {}).get('chain') == ['localhost:%d' % ports['B']] and data['trust']['redirects']['followed'] == 1)
    record('no Authorization, Cookie or Referer reaches either origin',
           all(not ({'authorization', 'cookie', 'proxy-authorization', 'referer'} & set(h)) for name in 'AB' for h in headers_seen[name]))
    record('signed query is sent to the storage origin only', any(secret in p for p in hits['B']) and not any(secret in p for p in hits['A']))
    fetch('relative redirect inside the named origin', '/rel', policy=('--redirect-hosts', 'localhost:%d' % ports['B']))
    for status in ('301', '303', '307', '308'):
        fetch('HTTP ' + status + ' followed', '/' + status)
    for status in ('300', '304'):
        fetch('HTTP ' + status + ' is not a redirect', '/' + status, expect_ok=False, code='http_status')
    fetch('target authority not allow-listed', '/to-c', expect_ok=False, code='redirect')
    fetch('allowing the right host on the wrong port is not enough', '/to-c', policy=('--redirect-hosts', 'localhost'), expect_ok=False, code='redirect')
    fetch('suffix look-alike host', '/to-evil-suffix', expect_ok=False, code='redirect')
    fetch('IPv6 literal target', '/to-ipv6', expect_ok=False, code='redirect')
    fetch('HTTP downgrade', '/to-plain', policy=('--redirect-hosts', 'localhost:%d' % ports['P']), expect_ok=False, code='redirect')
    fetch('credentials in redirect', '/to-cred', expect_ok=False, code='redirect')
    fetch('fragment in redirect', '/to-fragment', expect_ok=False, code='redirect')
    fetch('missing Location', '/no-location', expect_ok=False, code='redirect')
    fetch('two Location headers', '/two-locations', expect_ok=False, code='redirect')
    fetch('redirect loop', '/loop', expect_ok=False, code='redirect')
    fetch('too many hops (default 3)', '/hop1', expect_ok=False, code='redirect')
    fetch('five-hop limit explicit', '/hop1', extra=('--max-redirects', 5))
    fetch('redirect to wrong-certificate host fails TLS', '/to-wrong-cert', policy=('--redirect-hosts', '127.0.0.1:%d' % ports['W']), expect_ok=False, code='tls')
    fetch('redirected 404', '/to-missing', expect_ok=False, code='http_status')
    fetch('redirected oversize body', '/to-big', expect_ok=False, code='too_large')
    fetch('redirected interrupted body', '/to-truncated', expect_ok=False, code='interrupted', body=b'y' * 100000)
    bad = work / 'bad'
    p, reply = call(['fetch', '--url', A + '/asset/blob-small', '--ca-file', ca, '--output', bad, '--sha256', '0' * 64, '--size', len(small), '--redirect-hosts', 'localhost:%d' % ports['B']])
    record('redirected digest mismatch publishes nothing', p.returncode == 1 and reply['data']['code'] == 'digest_mismatch' and not bad.exists())
    p, reply = call(['fetch', '--url', A + '/asset/blob-small', '--ca-file', ca, '--output', bad, '--sha256', sha(small), '--size', len(small) + 1, '--redirect-hosts', 'localhost:%d' % ports['B']])
    record('redirected size mismatch publishes nothing', p.returncode == 1 and reply['data']['code'] == 'size_mismatch' and not bad.exists())
    p, reply = call(['fetch', '--url', A + '/to-redirect-b', '--ca-file', ca, '--output', bad, '--sha256', sha(small), '--size', len(small),
                     '--redirect-hosts', 'localhost:%d' % ports['B']])
    record('second hop leaving the allow-list refused', p.returncode == 1 and reply['data']['code'] == 'redirect' and not bad.exists())
    # Argument validation.
    for label, extra in (('policy and hosts together', ['--redirect-policy', 'github-release', '--redirect-hosts', 'localhost:1']),
                         ('unknown policy', ['--redirect-policy', 'any']), ('upper-case host', ['--redirect-hosts', 'Localhost']),
                         ('default port written out', ['--redirect-hosts', 'example.com:443']), ('wildcard host', ['--redirect-hosts', '*.example.com']),
                         ('empty entry', ['--redirect-hosts', 'a.example,,b.example']), ('hop count without a policy', ['--max-redirects', '2']),
                         ('hop count zero', ['--redirect-hosts', 'a.example', '--max-redirects', '0']), ('hop count six', ['--redirect-hosts', 'a.example', '--max-redirects', '6'])):
        p, _ = call(['fetch', '--url', A + '/asset/blob-small', '--ca-file', ca, '--output', bad, '--sha256', '0' * 64, '--size', 1, *extra])
        record('argument refused: ' + label, p.returncode == 2 and not bad.exists() and not any('/asset/' in h for h in hits['A'][-1:]) )
    before = len(hits['A'])
    p, _ = call(['fetch', '--url', A + '/asset/blob-small', '--ca-file', ca, '--output', bad, '--sha256', '0' * 64, '--size', 1, '--redirect-policy', 'any'])
    record('invalid arguments make no request', len(hits['A']) == before)

    # sdk acquire from the real promoted bytes: every asset is a redirect.
    source = {'schema': 'crt-cli.sdk-source', 'format': 0, 'ca_file': str(ca), 'ca_sha256': sha(ca.read_bytes()),
              'manifest': {'url': A + '/asset/blob-manifest', 'size': len(manifest_bytes), 'sha256': sha(manifest_bytes)},
              'archive': {'name': archive['name'], 'url': A + '/asset/blob-archive'}, 'contract_url': A + '/asset/blob-contract',
              'sdk': {'version': archive['install']['version'], 'content_sha256': archive['install']['content_sha256'], 'contract_sha256': policy['sha256']},
              'timeout_seconds': 60, 'redirect_hosts': ['localhost:%d' % ports['B']]}
    def acquire(label, value, store, expect_ok, message=None):
        spec = work / ('source-%d.json' % len(results)); spec.write_text(json.dumps(value))
        p, reply = call(['sdk', 'acquire', '--source', spec, '--store', store])
        text = p.stdout + p.stderr
        ok = (p.returncode == 0) == expect_ok and (message is None or message in text) and secret not in text
        record(label, ok, text[:500] if not ok else '')
        return reply
    store = work / 'store'; store.mkdir()
    reply = acquire('sdk acquire without a redirect policy is refused and installs nothing', {k: v for k, v in source.items() if k != 'redirect_hosts'}, work / 'store-none', False, 'http_status')
    record('refused acquisition leaves no store content', not (work / 'store-none').exists() or not any((work / 'store-none').glob('*-*')))
    reply = acquire('sdk acquire follows redirects for manifest, contract and archive', source, store, True)
    downloads = reply.get('data', {}).get('acquisition', {}).get('downloads', [])
    record('acquisition provenance records authorities only', len(downloads) == 3 and all(d.get('redirect_chain') == ['localhost:%d' % ports['B']] for d in downloads))
    sdk_dir = archive['install']['version'] + '-' + archive['install']['content_sha256'][:12]
    p, reply = call(['sdk', 'verify', sdk_dir, '--store', store])
    record('acquired SDK verifies', p.returncode == 0, p.stdout[:300])
    reply = acquire('re-acquire is already installed', source, store, True)
    record('re-acquire downloads nothing', reply.get('data', {}).get('already_installed') is True)
    bad_source = copy.deepcopy(source); bad_source['redirect_hosts'] = ['localhost:%d' % ports['C']]
    acquire('wrong allow-list refuses acquisition', bad_source, work / 'store-wrong', False, 'redirect')
    both = copy.deepcopy(source); both['redirect_policy'] = 'github-release'
    acquire('policy and hosts together refused', both, work / 'store-both', False, 'mutually exclusive')
    hops = copy.deepcopy(source); hops['max_redirects'] = 6
    acquire('max_redirects above five refused', hops, work / 'store-hops', False)
    nopol = {k: v for k, v in source.items() if k != 'redirect_hosts'}; nopol['max_redirects'] = 2
    acquire('max_redirects without allow-list refused', nopol, work / 'store-nopol', False)
    gh = copy.deepcopy(source); gh.pop('redirect_hosts'); gh['redirect_policy'] = 'github-release'
    acquire('github-release policy does not allow a loopback target', gh, work / 'store-gh', False, 'redirect')
    unknown = copy.deepcopy(source); unknown['redirect_policy'] = 'everything'; unknown.pop('redirect_hosts')
    acquire('unknown policy refused', unknown, work / 'store-unknown', False)

    passed = all(r['passed'] for r in results)
    record_path = out / 'redirect-replay.json'
    record_path.write_text(json.dumps({'passed': passed, 'cli_sha256': sha(cli.read_bytes()), 'cases': results, 'commands': commands,
                                       'origin_hits': {k: len(v) for k, v in hits.items()}}, indent=2, ensure_ascii=False) + '\n')
    print('%d/%d redirect cases passed' % (sum(r['passed'] for r in results), len(results)))
    for s in servers: s.shutdown()
    return 0 if passed else 1

if __name__ == '__main__':
    raise SystemExit(main())
