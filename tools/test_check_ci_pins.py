"""The pin reviewer must report a mismatch, not only print OK."""
import hashlib
import json
import unittest

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

if __name__ == '__main__':
    unittest.main()
