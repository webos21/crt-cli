"""Release naming, checksum and commit rules: a mistake here would publish the wrong bytes under the wrong name."""
import hashlib
import tempfile
import unittest
from pathlib import Path

from ci_release import (ReleaseError, locate_executable, locate_sdk, names, parse_sums, require_artifact_match, require_commit,
                        sums_line)


class NamesTest(unittest.TestCase):
    def test_reproduces_the_published_linux_aarch64_release(self):
        n = names('linux-arm64', '20261111', 1)
        self.assertEqual(n['tag'], 'v0.1.0-preview.20261111.linux-arm64.1')
        self.assertEqual(n['archive'], 'crt-cli-0.1.0-dev-linux-aarch64-preview.20261111.1.tar.xz')
        self.assertEqual(n['sums'], 'SHA256SUMS-linux-aarch64')
        self.assertEqual(n['title'], 'crt-cli Preview 20261111 (Linux aarch64)')

    def test_reproduces_the_other_published_releases(self):
        self.assertEqual(names('linux-x64', '20261110', 1)['tag'], 'v0.1.0-preview.20261110.linux-x86_64.1')
        self.assertEqual(names('linux-x64', '20261110', 1)['archive'], 'crt-cli-0.1.0-dev-linux-x86_64-preview.20261110.1.tar.xz')
        self.assertEqual(names('macos-arm64', '20261110', 1)['tag'], 'v0.1.0-preview.20261110.macos-arm64.1')
        self.assertEqual(names('macos-arm64', '20261110', 1)['archive'], 'crt-cli-0.1.0-dev-macos-aarch64-preview.20261110.1.tar.xz')
        self.assertEqual(names('win-x64', '20261110', 1)['tag'], 'v0.1.0-preview.20261110.windows-x86_64.1')
        self.assertEqual(names('win-x64', '20261110', 1)['sums'], 'SHA256SUMS-windows-x86_64')

    def test_bad_inputs_are_refused(self):
        for args in [('freebsd', '20261111', 1), ('linux-x64', '2026-11-11', 1), ('linux-x64', '20261111', 0), ('linux-x64', '20261111', '1')]:
            with self.subTest(args=args), self.assertRaises(ReleaseError):
                names(*args)


class CommitTest(unittest.TestCase):
    def test_only_a_full_lowercase_sha_is_a_commit(self):
        self.assertEqual(require_commit('6a008856d4a92fafbea1221b0f6c88a4559ccc29'), '6a008856d4a92fafbea1221b0f6c88a4559ccc29')
        for bad in ['main', 'v0.1.0', '6a00885', '6A008856D4A92FAFBEA1221B0F6C88A4559CCC29', '', None, '6a008856d4a92fafbea1221b0f6c88a4559ccc2']:
            with self.subTest(bad=bad), self.assertRaises(ReleaseError):
                require_commit(bad)


class ChecksumTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.archive = self.dir / 'a.tar.xz'
        self.archive.write_bytes(b'payload')
        self.digest = hashlib.sha256(b'payload').hexdigest()

    def tearDown(self):
        self.tmp.cleanup()

    def test_matching_archive_passes(self):
        self.assertEqual(require_artifact_match(self.archive, sums_line('a.tar.xz', self.digest), 'a.tar.xz'), self.digest)

    def test_changed_bytes_are_refused(self):
        self.archive.write_bytes(b'payloaX')
        with self.assertRaisesRegex(ReleaseError, 'differs'):
            require_artifact_match(self.archive, sums_line('a.tar.xz', self.digest), 'a.tar.xz')

    def test_wrong_name_extra_entry_malformed_and_empty_are_refused(self):
        with self.assertRaisesRegex(ReleaseError, 'exactly'):
            require_artifact_match(self.archive, sums_line('b.tar.xz', self.digest), 'a.tar.xz')
        two = sums_line('a.tar.xz', self.digest) + sums_line('b.tar.xz', self.digest)
        with self.assertRaisesRegex(ReleaseError, 'exactly'):
            require_artifact_match(self.archive, two, 'a.tar.xz')
        with self.assertRaisesRegex(ReleaseError, 'malformed'):
            parse_sums('not a checksum line\n')
        with self.assertRaisesRegex(ReleaseError, 'empty'):
            parse_sums('')
        with self.assertRaisesRegex(ReleaseError, 'duplicate'):
            parse_sums(sums_line('a.tar.xz', self.digest) * 2)
        with self.assertRaisesRegex(ReleaseError, 'lowercase'):
            sums_line('a.tar.xz', self.digest.upper())


class LocateTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_executable_is_found_by_digest_not_by_name(self):
        (self.dir / 'crt-cli-host').write_bytes(b'binary')
        (self.dir / 'other').write_bytes(b'something else')
        (self.dir / 'SHA256SUMS-host').write_text('x')
        (self.dir / 'ci-summary.json').write_text('{}')
        self.assertEqual(locate_executable(self.dir, hashlib.sha256(b'binary').hexdigest()).name, 'crt-cli-host')

    def test_zero_or_two_matching_executables_are_refused(self):
        (self.dir / 'a').write_bytes(b'same')
        with self.assertRaisesRegex(ReleaseError, 'found 0'):
            locate_executable(self.dir, '0' * 64)
        (self.dir / 'b').write_bytes(b'same')
        with self.assertRaisesRegex(ReleaseError, 'found 2'):
            locate_executable(self.dir, hashlib.sha256(b'same').hexdigest())

    def test_sdk_is_the_directory_the_summary_names(self):
        sdk = {'version': 'v0.0.1-x', 'content_sha256': 'cd2ab657615c0fa52a1804f301a57dfa21b1067e3e5ed5a9f9a07d86ae050cc6'}
        (self.dir / 'v0.0.1-x-cd2ab657615c/share/licenses').mkdir(parents=True)
        (self.dir / 'v0.0.1-x-cd2ab657615c/manifest.json').write_text('{}')
        self.assertEqual(locate_sdk(self.dir, sdk).name, 'v0.0.1-x-cd2ab657615c')
        with self.assertRaisesRegex(ReleaseError, 'not where'):
            locate_sdk(self.dir, dict(sdk, content_sha256='f' * 64))


if __name__ == '__main__':
    unittest.main()
