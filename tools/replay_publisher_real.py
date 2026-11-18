#!/usr/bin/env python3
"""Publisher-signature cases with the REAL key, the REAL published SDK bytes and the owner's REAL signature (any host).

The test-key cases of replay_publisher_acquire_linux.py prove the mechanism; this driver proves that the pinned production key accepts the
published signature on the published bytes and refuses a one-bit-changed copy of it. It needs the network: `sdk acquire` downloads the pinned
SDK from GitHub (github-release redirects) with the system roots as explicit transport trust. Python is a test-host tool; the consumer is
the native crt-cli with a cleared environment and a private HOME. --release-dir holds the release's manifest, contract, archive and `.sig`.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
import publisher_signature  # noqa: E402
from ci_bootstrap import write_system_roots  # noqa: E402


def sha(data):
    return hashlib.sha256(data).hexdigest()


def child_env(home):
    env = {'HOME': str(home), 'PATH': '/nonexistent'}
    if os.name == 'nt':
        env.update(PATH='', USERPROFILE=str(home), TEMP=str(home), TMP=str(home), SystemRoot=os.environ['SystemRoot'])
    return env


def published(store):
    return sorted(p.name for p in store.iterdir() if not p.name.startswith('.')) if store.is_dir() else []


def leftovers(store):
    return sorted(str(p.relative_to(store)) for area in ('.staging', '.locks') if (store / area).is_dir() for p in (store / area).rglob('*'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True)
    parser.add_argument('--pins', type=Path, default=TOOLS.parent / '.github/ci/pins.json')
    parser.add_argument('--host', required=True, help='key of the host in pins.json, for example win-x64')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.cli, args.release_dir = args.cli.resolve(), args.release_dir.resolve()  # the consumer runs in another directory
    pins = json.loads(args.pins.read_text(encoding='utf-8'))
    host = pins['hosts'][args.host]
    sdk = host['sdk']
    publisher = publisher_signature.pinned_publisher(pins['publisher'], TOOLS.parent)
    key_file, fingerprint = publisher['key_file'].resolve(), publisher['key_spki_sha256']
    out = args.output.resolve(); out.mkdir(parents=True)
    work = Path(tempfile.mkdtemp(prefix='real publisher 한글 ', dir=out))
    manifest = args.release_dir / sdk['manifest']['name']
    archive = args.release_dir / sdk['archive']
    real_sig = args.release_dir / (sdk['archive'] + '.sig')
    assert sha(manifest.read_bytes()) == sdk['manifest']['sha256'] and sha(real_sig.read_bytes()) == sdk['signature']['sha256'], 'inputs differ from the pins'
    flipped = work / 'one-bit-changed.sig'
    data = bytearray(real_sig.read_bytes()); data[100] ^= 1; flipped.write_bytes(bytes(data))
    sdk_dir = sdk['version'] + '-' + sdk['content_sha256'][:12]
    results = []

    def cli(label, home, store, argv, expect_ok, needle=None, check=None):
        home.mkdir(parents=True, exist_ok=True)
        proc = subprocess.run([str(args.cli), '--json', *argv], capture_output=True, env=child_env(home), cwd=work, timeout=900)
        text = proc.stdout.decode('utf-8', 'replace')
        try:
            reply = json.loads(text)
        except ValueError:
            reply = {'ok': False, 'data': {'message': text + proc.stderr.decode('utf-8', 'replace')}}
        message = json.dumps(reply.get('data'), ensure_ascii=False)
        passed = bool(reply.get('ok')) == expect_ok and (proc.returncode == 0) == expect_ok
        if needle:
            passed = passed and needle in message
        if check:
            passed = passed and bool(check(reply))
        results.append({'case': label, 'passed': passed, 'exit': proc.returncode, 'detail': message[:300], 'reply_sha256': sha(text.encode())})
        print(('PASS ' if passed else 'FAIL ') + label, flush=True)
        return reply

    publisher_args = ['--publisher-key', str(key_file), '--publisher-key-spki-sha256', fingerprint]
    release = ['--archive', str(archive), '--release-manifest', str(manifest), '--release-manifest-sha256', sdk['manifest']['sha256']]

    # 1. Real acquisition from GitHub with the real key and the pinned signature digest.
    roots = write_system_roots(work / 'roots.pem')
    base = sdk['release_base']
    source = {'schema': 'crt-cli.sdk-source', 'format': 0, 'ca_file': roots.as_posix(), 'ca_sha256': sha(roots.read_bytes()),
              'manifest': {'url': base + sdk['manifest']['name'], 'sha256': sdk['manifest']['sha256'], 'size': sdk['manifest']['size']},
              'archive': {'name': sdk['archive'], 'url': base + sdk['archive']}, 'contract_url': base + sdk['contract'],
              'sdk': {'version': sdk['version'], 'content_sha256': sdk['content_sha256'], 'contract_sha256': sdk['contract_sha256']},
              'redirect_policy': 'github-release', 'timeout_seconds': 300,
              'publisher': {'key_file': key_file.as_posix(), 'key_spki_sha256': fingerprint,
                            'archive_signature': {'url': base + sdk['archive'] + '.sig', 'sha256': sdk['signature']['sha256'], 'size': sdk['signature']['size']}}}
    source_path = work / 'sdk-source.json'
    source_path.write_text(json.dumps(source, indent=2) + '\n', encoding='utf-8')
    acquire_store = work / 'acquire store'
    reply = cli('real GitHub sdk acquire verifies the real signature with the real key', work / 'home-acquire', acquire_store,
                ['sdk', 'acquire', '--source', str(source_path), '--store', str(acquire_store)], True,
                check=lambda r: r['data']['acquisition']['publisher_authentication'] == 'signature_verified'
                and r['data']['identity']['content_sha256'] == sdk['content_sha256'])
    results[-1]['key_spki_sha256'] = reply.get('data', {}).get('acquisition', {}).get('publisher_key_spki_sha256')

    # 2. A one-bit-changed signature installs nothing.
    bad_store = work / 'bad store'
    cli('sdk install with a one-bit-changed real signature is refused', work / 'home-bad', bad_store,
        ['sdk', 'install', *release, *publisher_args, '--signature', str(flipped), '--store', str(bad_store)], False, 'publisher signature verification failed')
    results.append({'case': 'the refused install published nothing and left no staging or locks', 'passed': not published(bad_store) and not leftovers(bad_store),
                    'detail': str(published(bad_store))})
    print(('PASS ' if results[-1]['passed'] else 'FAIL ') + results[-1]['case'])

    # 3. The real signature installs and is reported as verified.
    good_store = work / 'good store'
    cli('sdk install with the real signature is signature_verified', work / 'home-good', good_store,
        ['sdk', 'install', *release, *publisher_args, '--signature', str(real_sig), '--store', str(good_store)], True,
        check=lambda r: r['data']['identity']['content_sha256'] == sdk['content_sha256'] and r['data'].get('publisher_authentication') == 'signature_verified')

    # 4. Damage the stored contract; a repair with the changed signature changes nothing.
    stored, receipt = good_store / '.contracts' / (sdk_dir + '.json'), good_store / '.receipts' / (sdk_dir + '.json')
    original_contract, original_receipt = stored.read_bytes(), receipt.read_bytes()
    damaged = json.loads(original_contract); damaged['changed_policy'] = 'a valid-looking edit of the stored policy'
    damaged_bytes = json.dumps(damaged).encode('utf-8'); stored.write_bytes(damaged_bytes)
    cli('sdk verify detects the damaged contract', work / 'home-good', good_store, ['sdk', 'verify', sdk_dir, '--store', str(good_store)], False, 'digest mismatch')
    repair = ['sdk', 'repair', sdk_dir, *release, *publisher_args, '--store', str(good_store)]
    cli('sdk repair with a one-bit-changed real signature is refused', work / 'home-good', good_store, [*repair, '--signature', str(flipped)], False, 'publisher signature verification failed')
    results.append({'case': 'the refused repair left the damaged contract and the original receipt untouched', 'passed':
                    stored.read_bytes() == damaged_bytes and receipt.read_bytes() == original_receipt and not leftovers(good_store), 'detail': ''})
    print(('PASS ' if results[-1]['passed'] else 'FAIL ') + results[-1]['case'])

    # 5. The real signature repairs it.
    cli('sdk repair with the real signature restores the contract', work / 'home-good', good_store, [*repair, '--signature', str(real_sig)], True)
    results.append({'case': 'the verified repair restored the original contract and receipt', 'passed':
                    stored.read_bytes() == original_contract and receipt.read_bytes() == original_receipt and not leftovers(good_store), 'detail': ''})
    print(('PASS ' if results[-1]['passed'] else 'FAIL ') + results[-1]['case'])
    cli('sdk verify passes after the verified repair', work / 'home-good', good_store, ['sdk', 'verify', sdk_dir, '--store', str(good_store)], True, check=lambda r: r['data']['verified'])

    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'host': args.host, 'cli_sha256': sha(args.cli.read_bytes()),
               'key_spki_sha256': fingerprint, 'sdk': sdk['version'], 'content_sha256': sdk['content_sha256'], 'signature_sha256': sdk['signature']['sha256'],
               'one_bit_changed_signature_sha256': sha(flipped.read_bytes())}
    (out / 'result.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print('%d/%d cases passed' % (sum(r['passed'] for r in results), len(results)))
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
