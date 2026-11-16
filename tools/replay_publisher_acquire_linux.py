#!/usr/bin/env python3
"""Publisher-signature cases of `sdk acquire` over a private HTTPS server; Linux entry point.

Python/OpenSSL are test-host tools (they sign with a throw-away RSA key and serve loopback TLS); the consumer is the native
crt-cli with a cleared environment and a private HOME. --release-dir holds one published native Dev SDK release (its
release-manifest-dev-linux-*.json, contract and archive). Nothing here proves an acquisition from a public host.
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
import tempfile
import threading


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True)
    work = Path(tempfile.mkdtemp(prefix='publisher acquire 한글 ', dir=out))
    openssl = shutil.which('openssl'); assert openssl

    def ossl(*argv, **kw):
        return subprocess.run([openssl, *argv], cwd=work, check=True, capture_output=True, **kw)

    manifest_path = next(args.release_dir.glob('release-manifest-dev-linux-*.json'))
    manifest_bytes = manifest_path.read_bytes(); manifest = json.loads(manifest_bytes)
    archive = next(a for a in manifest['assets'] if 'install' in a)
    policy = next(a for a in manifest['assets'] if a['name'] == archive['contract'])
    archive_bytes = (args.release_dir / archive['name']).read_bytes()
    contract_bytes = (args.release_dir / policy['name']).read_bytes()
    assert sha(archive_bytes) == archive['sha256'] and sha(contract_bytes) == policy['sha256']
    sdk_dir = archive['install']['version'] + '-' + archive['install']['content_sha256'][:12]

    ossl('req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', 'ca.key', '-out', 'ca.pem', '-days', '2', '-subj', '/CN=publisher test CA',
         '-addext', 'basicConstraints=critical,CA:TRUE', '-addext', 'keyUsage=keyCertSign,cRLSign')
    (work / 'leaf.ext').write_text('subjectAltName=DNS:localhost\nbasicConstraints=CA:FALSE\nextendedKeyUsage=serverAuth\n')
    ossl('req', '-newkey', 'rsa:2048', '-nodes', '-keyout', 'leaf.key', '-out', 'leaf.csr', '-subj', '/CN=localhost')
    ossl('x509', '-req', '-in', 'leaf.csr', '-CA', 'ca.pem', '-CAkey', 'ca.key', '-CAcreateserial', '-out', 'leaf.pem', '-days', '2', '-extfile', 'leaf.ext')
    for name in ('pub1', 'pub2'):
        ossl('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', name + '.key')
        ossl('pkey', '-in', name + '.key', '-pubout', '-out', name + '.pub')

    def fingerprint(name):
        der = subprocess.run([openssl, 'pkey', '-pubin', '-in', str(work / (name + '.pub')), '-outform', 'DER'], capture_output=True, check=True).stdout
        return sha(der)

    def sign(key, data_path, out_name):
        ossl('dgst', '-sha256', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32', '-sigopt', 'rsa_mgf1_md:sha256',
             '-sign', key + '.key', '-out', out_name, str(data_path))
        return (work / out_name).read_bytes()

    archive_file = work / 'archive.bin'; archive_file.write_bytes(archive_bytes)
    good_sig = sign('pub1', archive_file, 'good.sig')
    other_key_sig = sign('pub2', archive_file, 'other.sig')
    tampered = work / 'tampered.bin'; tampered.write_bytes(archive_bytes[:-1] + bytes([archive_bytes[-1] ^ 1]))
    wrong_data_sig = sign('pub1', tampered, 'wrongdata.sig')
    routes = {'/manifest': manifest_bytes, '/contract': contract_bytes, '/archive': archive_bytes, '/good.sig': good_sig,
              '/other.sig': other_key_sig, '/wrongdata.sig': wrong_data_sig, '/huge.sig': b'x' * 2048}
    hits = []

    class Handler(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'
        def log_message(self, *_):
            pass
        def do_GET(self):
            hits.append(self.path)
            body = routes.get(self.path)
            try:
                if body is None:
                    self.send_response(404); self.send_header('Content-Length', '0'); self.end_headers(); return
                self.send_response(200); self.send_header('Content-Length', str(len(body))); self.end_headers(); self.wfile.write(body)
            except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
                pass

    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler); server.daemon_threads = True
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER); context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(work / 'leaf.pem', work / 'leaf.key')
    server.socket = context.wrap_socket(server.socket, server_side=True)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    origin = 'https://localhost:%d' % server.server_port
    ca_file = work / 'ca.pem'
    base = {'schema': 'crt-cli.sdk-source', 'format': 0, 'ca_file': str(ca_file), 'ca_sha256': sha(ca_file.read_bytes()),
            'manifest': {'url': origin + '/manifest', 'size': len(manifest_bytes), 'sha256': sha(manifest_bytes)},
            'archive': {'name': archive['name'], 'url': origin + '/archive'}, 'contract_url': origin + '/contract',
            'sdk': {'version': archive['install']['version'], 'content_sha256': archive['install']['content_sha256'],
                    'contract_sha256': policy['sha256']}, 'timeout_seconds': 20}

    def publisher(key='pub1', sig='/good.sig', sig_bytes=None, fp=None, size=None, digest=None):
        data = good_sig if sig_bytes is None else sig_bytes
        return {'key_file': str(work / (key + '.pub')), 'key_spki_sha256': fp or fingerprint(key),
                'archive_signature': {'url': origin + sig, 'size': size or len(data), 'sha256': digest or sha(data)}}

    results = []
    homes = [0]

    def acquire(label, source, expect_ok, needle=None, forbid=None, reuse_home=None):
        homes[0] += 1
        home = reuse_home or (work / ('home%d' % homes[0]))
        home.mkdir(exist_ok=True)
        spec = work / ('source%d.json' % homes[0]); spec.write_text(json.dumps(source))
        before = len(hits)
        proc = subprocess.run([str(args.cli.resolve()), '--json', 'sdk', 'acquire', '--source', str(spec), '--store', str(home / 'sdks')],
                              capture_output=True, text=True, env={'HOME': str(home), 'PATH': '/nonexistent'}, timeout=300, stdin=subprocess.DEVNULL)
        try:
            reply = json.loads(proc.stdout)
        except ValueError:
            reply = {}
        detail = json.dumps(reply.get('data', {}))
        passed = (proc.returncode == 0 and reply.get('ok') is True) if expect_ok else (proc.returncode > 0 and reply.get('ok') is False)
        if needle:
            passed = passed and needle in detail
        requested = hits[before:]
        if forbid:
            passed = passed and forbid not in requested
        store = home / 'sdks'
        installed = [p.name for p in store.iterdir() if not p.name.startswith('.')] if store.exists() else []
        leftovers = [p.name for area in ('.staging', '.locks') if (store / area).exists() for p in (store / area).iterdir()]
        if not expect_ok:
            passed = passed and not installed and not leftovers
        results.append({'case': label, 'passed': bool(passed), 'detail': detail[:500], 'requests': requested})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)
        return reply, home, requested

    reply, home, _ = acquire('no publisher field keeps not_verified', base, True)
    results[-1]['passed'] = results[-1]['passed'] and reply['data']['acquisition']['publisher_authentication'] == 'not_verified'

    source = copy.deepcopy(base); source['publisher'] = publisher()
    reply, home, requested = acquire('pinned key and signature verify the archive', source, True)
    acq = reply.get('data', {}).get('acquisition', {})
    ok = (acq.get('publisher_authentication') == 'signature_verified' and acq.get('publisher_key_spki_sha256') == fingerprint('pub1') and
          requested == ['/manifest', '/contract', '/good.sig', '/archive'] and (home / 'sdks' / sdk_dir).is_dir())
    results[-1]['passed'] = results[-1]['passed'] and ok
    print(('PASS ' if results[-1]['passed'] else 'FAIL ') + '  (provenance and request order)')
    before = len(hits)
    reply, _, requested = acquire('an installed SDK is reused with an explicit not-rechecked note', source, True, 'not re-checked', reuse_home=home)
    results[-1]['passed'] = results[-1]['passed'] and requested == [] and reply['data']['acquisition']['publisher_authentication'] == 'not_verified'

    def negative(label, publisher_value, needle=None, forbid=None, mutate=None):
        bad = copy.deepcopy(base); bad['publisher'] = publisher_value
        if mutate:
            mutate(bad)
        return acquire(label, bad, False, needle, forbid)

    negative('a key file that did not sign the archive is refused', publisher('pub2', fp=fingerprint('pub1')), 'publisher_signature_invalid')
    negative('a valid signature by a key other than the pinned fingerprint is refused and nothing is published',
             publisher('pub2', sig='/other.sig', sig_bytes=other_key_sig, fp=fingerprint('pub1')), 'pinned publisher key')
    negative('a signature by another key is refused and nothing is published', publisher(sig='/other.sig', sig_bytes=other_key_sig), 'publisher_signature_invalid')
    negative('a signature over different bytes is refused', publisher(sig='/wrongdata.sig', sig_bytes=wrong_data_sig), 'publisher_signature_invalid')
    negative('a signature digest pin mismatch stops before the archive is requested', publisher(digest='0' * 64), 'digest_mismatch', '/archive')
    negative('a missing signature asset stops before the archive is requested', publisher(sig='/missing.sig'), None, '/archive')
    negative('an oversized signature pin is refused before any request', publisher(sig='/huge.sig', sig_bytes=b'x' * 2048), 'exceeds', '/manifest')
    negative('a wrong fingerprint pin is refused', publisher(fp='0' * 64), 'pinned publisher key')
    negative('a relative key file is refused', publisher(), 'absolute', '/manifest', lambda s: s['publisher'].update(key_file='pub1.pub'))
    negative('an unknown publisher field is refused', publisher(), 'unknown SDK source field', '/manifest', lambda s: s['publisher'].update(trust='auto'))
    negative('an http signature URL is refused', publisher(), 'HTTPS', '/manifest', lambda s: s['publisher']['archive_signature'].update(url='http://localhost/x.sig'))
    negative('a missing signature object is refused', publisher(), None, '/manifest', lambda s: s['publisher'].pop('archive_signature'))

    # `sdk install` (local files, release-manifest mode): the same key/signature options, verified before anything is installed.
    inst = work / 'install-inputs'; inst.mkdir()
    (inst / archive['name']).write_bytes(archive_bytes); (inst / policy['name']).write_bytes(contract_bytes)
    (inst / 'release-manifest.json').write_bytes(manifest_bytes)
    (inst / 'other.sig').write_bytes(other_key_sig); (inst / 'good.sig').write_bytes(good_sig); (inst / 'wrongdata.sig').write_bytes(wrong_data_sig)

    def install(label, extra, expect_ok, needle=None, reuse_home=None, exit_code=None):
        homes[0] += 1
        home = reuse_home or (work / ('home%d' % homes[0])); home.mkdir(exist_ok=True)
        argv = [str(args.cli.resolve()), '--json', 'sdk', 'install', '--archive', str(inst / archive['name']), '--release-manifest', str(inst / 'release-manifest.json'),
                '--release-manifest-sha256', sha(manifest_bytes), '--store', str(home / 'sdks'), *extra]
        proc = subprocess.run(argv, capture_output=True, text=True, env={'HOME': str(home), 'PATH': '/nonexistent'}, timeout=300, stdin=subprocess.DEVNULL)
        try:
            reply = json.loads(proc.stdout)
        except ValueError:
            reply = {}
        detail = json.dumps(reply.get('data', {}))
        passed = (proc.returncode == 0 and reply.get('ok') is True) if expect_ok else (proc.returncode > 0 and reply.get('ok') is False)
        if exit_code is not None:
            passed = passed and proc.returncode == exit_code
        if needle:
            passed = passed and needle in detail
        store = home / 'sdks'
        installed = [p.name for p in store.iterdir() if not p.name.startswith('.')] if store.exists() else []
        leftovers = [p.name for area in ('.staging', '.locks') if (store / area).exists() for p in (store / area).iterdir()]
        if not expect_ok:
            passed = passed and not installed and not leftovers
        results.append({'case': label, 'passed': bool(passed), 'detail': detail[:500]})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)
        return reply, home

    good = ['--publisher-key', str(work / 'pub1.pub'), '--signature', str(inst / 'good.sig'), '--publisher-key-spki-sha256', fingerprint('pub1')]
    reply, home = install('sdk install without publisher options keeps the unchanged result shape', [], True)
    results[-1]['passed'] = results[-1]['passed'] and 'publisher_authentication' not in reply['data']
    reply, home = install('sdk install verifies the archive signature with the pinned key and fingerprint', good, True, 'signature_verified')
    results[-1]['passed'] = results[-1]['passed'] and reply['data']['publisher']['public_key_spki_sha256'] == fingerprint('pub1') and (home / 'sdks' / sdk_dir).is_dir()
    install('sdk install with a signature by another key installs nothing', ['--publisher-key', str(work / 'pub1.pub'), '--signature', str(inst / 'other.sig')], False, 'signature verification failed')
    install('sdk install with a signature over different bytes installs nothing', ['--publisher-key', str(work / 'pub1.pub'), '--signature', str(inst / 'wrongdata.sig')], False, 'signature verification failed')
    install('sdk install with a key file that did not sign installs nothing', ['--publisher-key', str(work / 'pub2.pub'), '--signature', str(inst / 'good.sig')], False, 'signature verification failed')
    install('sdk install with a mismatching fingerprint pin installs nothing', ['--publisher-key', str(work / 'pub2.pub'), '--signature', str(inst / 'other.sig'),
            '--publisher-key-spki-sha256', fingerprint('pub1')], False, 'pinned key fingerprint')
    install('sdk install refuses a key without a signature', ['--publisher-key', str(work / 'pub1.pub')], False, 'supplied together', exit_code=2)
    install('sdk install refuses a signature without a key', ['--signature', str(inst / 'good.sig')], False, 'supplied together', exit_code=2)
    install('sdk install refuses a fingerprint pin without a key', ['--publisher-key-spki-sha256', fingerprint('pub1')], False, 'needs --publisher-key', exit_code=2)
    install('sdk install refuses a malformed fingerprint pin', [*good[:4], '--publisher-key-spki-sha256', 'ABC'], False, '64 lowercase', exit_code=2)
    install('sdk install refuses a missing signature file', ['--publisher-key', str(work / 'pub1.pub'), '--signature', str(inst / 'absent.sig')], False)

    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'release_manifest_sha256': sha(manifest_bytes),
               'cli_sha256': sha(args.cli.read_bytes())}
    (out / 'result.json').write_text(json.dumps(summary, indent=2) + '\n')
    print('{}/{} cases passed'.format(sum(r['passed'] for r in results), len(results)))
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
