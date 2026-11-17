"""CI report gates: parallel execution must not hide missing checks or failed sections."""
import hashlib
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from ci_bootstrap import Ci, CiError, ROOT as ROOT_FOR_TESTS, host_key, native_phase_report


class NativePhaseReportTest(unittest.TestCase):
    def setUp(self):
        names = ['startup', 'process', 'selection', 'projects/c', 'projects/cxx'] + [
            'variants/' + language + '/' + kind for language in ('c', 'cxx')
            for kind in ('static-library', 'shared-library', 'executable')]
        self.phases = [dict(name=name, checks=1, seconds=0.25, completed=True) for name in names]

    def report(self, newline='\n'):
        return newline.join(['Hello from CRT'] + ['native-phase: ' + json.dumps(p) for p in self.phases])

    def test_complete_report_accepts_both_line_endings(self):
        for newline in ('\n', '\r\n'):
            self.assertEqual(len(native_phase_report(self.report(newline), 11)), 11)

    def test_missing_section_is_not_a_smaller_pass(self):
        self.phases.pop()
        with self.assertRaisesRegex(CiError, 'coverage'):
            native_phase_report(self.report(), 10)

    def test_duplicate_section_cannot_replace_another(self):
        self.phases[-1] = self.phases[0].copy()
        with self.assertRaisesRegex(CiError, 'coverage'):
            native_phase_report(self.report(), 11)

    def test_incomplete_section_fails(self):
        self.phases[0]['completed'] = False
        with self.assertRaisesRegex(CiError, 'did not complete'):
            native_phase_report(self.report(), 11)

    def test_count_must_match_the_native_total(self):
        with self.assertRaisesRegex(CiError, 'counts differ'):
            native_phase_report(self.report(), 12)

    def test_invalid_telemetry_fails(self):
        for field, value in [('checks', -1), ('checks', True), ('seconds', -1), ('seconds', float('nan'))]:
            saved = self.phases[0][field]
            self.phases[0][field] = value
            with self.assertRaises(CiError):
                native_phase_report(self.report(), 11)
            self.phases[0][field] = saved

    def test_malformed_report_is_a_ci_failure(self):
        with self.assertRaisesRegex(CiError, 'malformed'):
            native_phase_report('native-phase: {broken', 11)


if __name__ == '__main__':
    unittest.main()


class HostKeyTest(unittest.TestCase):
    def test_supported_hosts(self):
        for system, machine, key in [('linux', 'x86_64', 'linux-x64'), ('linux', 'aarch64', 'linux-arm64'), ('linux', 'arm64', 'linux-arm64'),
                                     ('darwin', 'arm64', 'macos-arm64'), ('win32', 'AMD64', 'win-x64')]:
            self.assertEqual(host_key(system, machine), key)

    def test_unsupported_hosts_are_refused(self):
        for system, machine in [('linux', 'riscv64'), ('darwin', 'x86_64'), ('win32', 'ARM64')]:
            with self.assertRaisesRegex(CiError, 'unsupported CI host'):
                host_key(system, machine)


@unittest.skipUnless(shutil.which('openssl'), 'openssl is a test-host tool')
class BootstrapAuthenticationTest(unittest.TestCase):
    """The pinned bootstrap CLI must be signed by the pinned key before it is extracted; a pin without a signature fails."""
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        openssl = shutil.which('openssl')
        for name in ('a', 'b'):
            subprocess.run([openssl, 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', str(self.dir / (name + '.key'))], check=True, capture_output=True)
            subprocess.run([openssl, 'pkey', '-in', str(self.dir / (name + '.key')), '-pubout', '-out', str(self.dir / (name + '.pub'))], check=True, capture_output=True)
        self.archive = self.dir / 'cli.tar.xz'
        self.archive.write_bytes(b'bootstrap bytes')
        # The repository-relative key path is resolved against the module's ROOT, which the test points at the throw-away directory.
        patcher = patch('ci_bootstrap.ROOT', self.dir)
        patcher.start()
        self.addCleanup(patcher.stop)

    def tearDown(self):
        self.tmp.cleanup()

    def sign(self, key):
        out = self.dir / (key + '.sig')
        subprocess.run([shutil.which('openssl'), 'dgst', '-sha256', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32', '-sigopt', 'rsa_mgf1_md:sha256',
                        '-sign', str(self.dir / (key + '.key')), '-out', str(out), str(self.archive)], check=True, capture_output=True)
        return out.read_bytes()

    def ci(self, key_file='a.pub'):
        import publisher_signature
        ci = Ci.__new__(Ci)
        ci.work, ci.summary = self.dir, {}
        fingerprint = publisher_signature.spki_sha256(self.dir / key_file)
        ci.pins = {'publisher': {'key_file': key_file, 'key_spki_sha256': fingerprint}}
        return ci

    def authenticate(self, ci, signature_bytes, pin_changes=None):
        pin = {'signature': {'name': 'cli.tar.xz.sig', 'size': len(signature_bytes), 'sha256': hashlib.sha256(signature_bytes).hexdigest()}}
        if pin_changes:
            pin['signature'].update(pin_changes)
        ci.download = lambda url, destination, label: Path(destination).write_bytes(signature_bytes)
        ci.authenticate('bootstrap CLI', self.archive, 'https://example.invalid/cli.tar.xz', pin)

    def test_the_pinned_key_signature_authenticates_and_is_recorded(self):
        ci = self.ci()
        self.authenticate(ci, self.sign('a'))
        self.assertEqual(ci.summary['publisher']['authenticated'], ['bootstrap CLI'])

    def test_another_keys_signature_a_changed_pin_and_a_missing_pin_fail(self):
        ci = self.ci()
        with self.assertRaisesRegex(CiError, 'verification failed'):
            self.authenticate(ci, self.sign('b'))
        with self.assertRaisesRegex(CiError, 'does not match its pin'):
            self.authenticate(ci, self.sign('a'), {'sha256': '0' * 64})
        ci.download = lambda url, destination, label: None
        with self.assertRaisesRegex(CiError, 'signature pin'):
            ci.authenticate('bootstrap CLI', self.archive, 'https://example.invalid/cli.tar.xz', {})
        self.assertNotIn('publisher', ci.summary)

    def test_a_wrong_fingerprint_for_the_key_file_fails(self):
        ci = self.ci()
        ci.pins['publisher']['key_spki_sha256'] = '0' * 64
        with self.assertRaisesRegex(CiError, 'does not match'):
            self.authenticate(ci, self.sign('a'))

class SdkSourcePublisherTest(unittest.TestCase):
    """The SDK source gets the `publisher` object only for a bootstrap CLI that the pin says understands it (an older one rejects unknown fields)."""
    def source(self, flag, signature=True):
        import publisher_signature
        with tempfile.TemporaryDirectory() as folder:
            ci = Ci.__new__(Ci)
            ci.work, ci.summary = Path(folder), {}
            key = ROOT_FOR_TESTS / '.github' / 'ci' / 'crt-release-publisher.pub'
            sdk = {'release_base': 'https://example.invalid/r/', 'version': 'v1', 'archive': 'a.tar.xz', 'contract': 'c.json', 'content_sha256': '1' * 64,
                   'contract_sha256': '2' * 64, 'manifest': {'name': 'm.json', 'sha256': '3' * 64, 'size': 10}}
            if signature:
                sdk['signature'] = {'name': 'a.tar.xz.sig', 'size': 256, 'sha256': '4' * 64}
            boot = {'sdk_source_publisher': True} if flag else {}
            ci.pins = {'publisher': {'key_file': '.github/ci/crt-release-publisher.pub', 'key_spki_sha256': publisher_signature.spki_sha256(key)}}
            ci.host_pins = {'bootstrap': boot, 'sdk': sdk}
            pem = Path(folder) / 'roots.pem'
            pem.write_text('x', encoding='ascii')
            path = ci.sdk_source(pem)
            return json.loads(path.read_text(encoding='utf-8'))

    def test_a_flagged_bootstrap_gets_the_pinned_key_and_signature(self):
        source = self.source(True)
        publisher = source['publisher']
        self.assertTrue(Path(publisher['key_file']).is_absolute())
        self.assertEqual(publisher['archive_signature'], {'url': 'https://example.invalid/r/a.tar.xz.sig', 'sha256': '4' * 64, 'size': 256})
        self.assertEqual(len(publisher['key_spki_sha256']), 64)

    def test_an_unflagged_bootstrap_keeps_the_source_it_always_had(self):
        self.assertNotIn('publisher', self.source(False))

    def test_a_flagged_bootstrap_without_a_signature_pin_fails(self):
        with self.assertRaises(Exception):
            self.source(True, signature=False)
