"""The pin reviewer must report a mismatch, not only print OK."""
import copy
import hashlib
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from check_ci_pins import review_host

def sha(data):
    return hashlib.sha256(data).hexdigest()

BOOT, ARCHIVE, CONTRACT = b'bootstrap-archive', b'sdk-archive', b'sdk-contract'

def world(**changes):
    manifest = {'assets': [{'name': 'sdk.tar.xz', 'size': len(ARCHIVE), 'sha256': sha(ARCHIVE)},
                           {'name': 'sdk-toolchain.json', 'size': len(CONTRACT), 'sha256': sha(CONTRACT)}]}
    manifest.update(changes.pop('manifest', {}))
    manifest_bytes = json.dumps(manifest).encode()
    pins = {'bootstrap': {'release': 'v1', 'asset': 'cli.tar.xz', 'size': len(BOOT), 'sha256': sha(BOOT)},
            'sdk': {'release_base': 'https://example.invalid/sdk/', 'manifest': {'name': 'm.json', 'size': len(manifest_bytes), 'sha256': sha(manifest_bytes)},
                    'archive': 'sdk.tar.xz', 'contract': 'sdk-toolchain.json', 'contract_sha256': sha(CONTRACT), 'content_sha256': 'c' * 64}}
    files = {'https://github.com/webos21/crt-cli/releases/download/v1/cli.tar.xz': BOOT,
             'https://example.invalid/sdk/m.json': manifest_bytes, 'https://example.invalid/sdk/sdk.tar.xz': ARCHIVE,
             'https://example.invalid/sdk/sdk-toolchain.json': CONTRACT}
    files.update(changes.get('files', {}))
    return pins, (lambda url: files[url])

class ReviewTest(unittest.TestCase):
    def test_matching_pins_report_nothing(self):
        pins, get = world()
        self.assertEqual(review_host('h', pins, get), [])

    def test_a_changed_bootstrap_archive_is_reported(self):
        pins, get = world(files={'https://github.com/webos21/crt-cli/releases/download/v1/cli.tar.xz': b'tampered-bootstrap'})
        problems = review_host('h', pins, get)
        self.assertTrue(any('bootstrap archive' in p and 'SHA-256' in p for p in problems), problems)

    def test_a_changed_size_alone_is_reported(self):
        pins, get = world()
        pins['bootstrap']['size'] += 1
        self.assertTrue(any('size' in p for p in review_host('h', pins, get)))

    def test_a_changed_sdk_archive_or_contract_is_reported(self):
        pins, get = world(files={'https://example.invalid/sdk/sdk.tar.xz': b'other-sdk-archive!'})
        self.assertTrue(any('SDK archive' in p for p in review_host('h', pins, get)))
        pins, get = world(files={'https://example.invalid/sdk/sdk-toolchain.json': b'other-contract'})
        self.assertTrue(any('contract' in p for p in review_host('h', pins, get)))

    def test_an_archive_the_manifest_does_not_list_is_reported(self):
        pins, get = world()
        pins['sdk']['archive'] = 'unlisted.tar.xz'
        self.assertTrue(any('does not list the pinned archive' in p for p in review_host('h', pins, get)))

    def test_a_contract_digest_that_differs_from_the_manifest_is_reported(self):
        wrong = {'assets': [{'name': 'sdk.tar.xz', 'size': len(ARCHIVE), 'sha256': sha(ARCHIVE)},
                            {'name': 'sdk-toolchain.json', 'size': len(CONTRACT), 'sha256': '0' * 64}]}
        pins, get = world(manifest=wrong)
        self.assertTrue(any('differs from the one the SDK manifest lists' in p for p in review_host('h', pins, get)))

@unittest.skipUnless(shutil.which('openssl'), 'openssl is a test-host tool')
class SignatureReviewTest(unittest.TestCase):
    """Throw-away key; the reviewer must verify each signed archive and its signature pin, not just print OK."""
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.dir = Path(cls.tmp.name)
        openssl = shutil.which('openssl')
        for name in ('a', 'b'):
            subprocess.run([openssl, 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', str(cls.dir / (name + '.key'))], check=True, capture_output=True)
            subprocess.run([openssl, 'pkey', '-in', str(cls.dir / (name + '.key')), '-pubout', '-out', str(cls.dir / (name + '.pub'))], check=True, capture_output=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def sign(self, key, data):
        blob, out = self.dir / 'blob', self.dir / 'out.sig'
        blob.write_bytes(data)
        subprocess.run([shutil.which('openssl'), 'dgst', '-sha256', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32',
                        '-sigopt', 'rsa_mgf1_md:sha256', '-sign', str(self.dir / (key + '.key')), '-out', str(out), str(blob)], check=True, capture_output=True)
        return out.read_bytes()

    def signed_world(self, boot_key='a', sdk_key='a'):
        pins, get = world()
        files = {}
        for part, data, url, key in (('bootstrap', BOOT, 'https://github.com/webos21/crt-cli/releases/download/v1/cli.tar.xz', boot_key),
                                     ('sdk', ARCHIVE, 'https://example.invalid/sdk/sdk.tar.xz', sdk_key)):
            signature = self.sign(key, data)
            files[url + '.sig'] = signature
            pins[part]['signature'] = {'name': pins[part].get('asset', pins[part].get('archive')) + '.sig', 'size': len(signature), 'sha256': sha(signature)}
        base = get
        return pins, (lambda url: files[url] if url in files else base(url)), files

    def publisher(self, key='a'):
        return {'key_file': self.dir / (key + '.pub'), 'key_spki_sha256': 'x'}

    def test_signed_archives_report_nothing(self):
        pins, get, _ = self.signed_world()
        self.assertEqual(review_host('h', pins, get, publisher=self.publisher()), [])

    def test_a_signature_by_another_key_is_reported_for_each_archive(self):
        pins, get, _ = self.signed_world(boot_key='b')
        self.assertTrue(any('bootstrap archive' in p and 'did not sign' in p for p in review_host('h', pins, get, publisher=self.publisher())))
        pins, get, _ = self.signed_world(sdk_key='b')
        self.assertTrue(any('SDK archive' in p and 'did not sign' in p for p in review_host('h', pins, get, publisher=self.publisher())))

    def test_a_changed_signature_pin_or_a_missing_signature_pin_is_reported(self):
        pins, get, _ = self.signed_world()
        bad = copy.deepcopy(pins)
        bad['bootstrap']['signature']['sha256'] = '0' * 64
        self.assertTrue(any('bootstrap archive signature' in p and 'SHA-256' in p for p in review_host('h', bad, get, publisher=self.publisher())))
        bad = copy.deepcopy(pins)
        del bad['sdk']['signature']
        self.assertTrue(any('signature pin' in p for p in review_host('h', bad, get, publisher=self.publisher())))

    def test_a_tampered_archive_with_a_matching_pin_is_still_reported(self):
        pins, get, _ = self.signed_world()
        tampered = b'tampered-bootstrap'
        pins['bootstrap'].update(size=len(tampered), sha256=sha(tampered))
        url = 'https://github.com/webos21/crt-cli/releases/download/v1/cli.tar.xz'
        problems = review_host('h', pins, lambda u: tampered if u == url else get(u), publisher=self.publisher())
        self.assertTrue(any('did not sign' in p for p in problems), problems)


class CommittedPinsTest(unittest.TestCase):
    def test_every_host_pins_signatures_that_match_the_asset_names_and_one_publisher(self):
        root = Path(__file__).resolve().parent.parent / '.github/ci'
        pins = json.loads((root / 'pins.json').read_text())
        self.assertEqual(pins['publisher'], json.loads((root / 'integration-pins.json').read_text())['publisher'])
        for host, entry in pins['hosts'].items():
            with self.subTest(host=host):
                self.assertEqual(entry['bootstrap']['signature']['name'], entry['bootstrap']['asset'] + '.sig')
                self.assertEqual(entry['sdk']['signature']['name'], entry['sdk']['archive'] + '.sig')
                for part in ('bootstrap', 'sdk'):
                    self.assertEqual(set(entry[part]['signature']), {'name', 'size', 'sha256'})
                    self.assertEqual(entry[part]['signature']['size'], 256)


if __name__ == '__main__':
    unittest.main()
