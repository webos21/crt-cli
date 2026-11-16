"""The stdlib publisher-signature verifier against OpenSSL-made keys and signatures (OpenSSL is a test-host tool only)."""
import hashlib
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import publisher_signature as ps

OPENSSL = shutil.which('openssl')


@unittest.skipUnless(OPENSSL, 'openssl is a test-host tool')
class PublisherSignatureTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.dir = Path(cls.tmp.name)
        for name, bits in (('k2048', 2048), ('k2048b', 2048), ('k4096', 4096), ('k1024', 1024)):
            cls.run_openssl('genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:%d' % bits, '-out', cls.dir / (name + '.key'))
            cls.run_openssl('pkey', '-in', cls.dir / (name + '.key'), '-pubout', '-out', cls.dir / (name + '.pub'))
        cls.run_openssl('genpkey', '-algorithm', 'EC', '-pkeyopt', 'ec_paramgen_curve:P-256', '-out', cls.dir / 'ec.key')
        cls.run_openssl('pkey', '-in', cls.dir / 'ec.key', '-pubout', '-out', cls.dir / 'ec.pub')
        cls.data = cls.dir / 'asset.bin'
        cls.data.write_bytes(b'published bytes ' * 5000)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    @staticmethod
    def run_openssl(*argv):
        return subprocess.run([OPENSSL, *map(str, argv)], check=True, capture_output=True)

    def sign(self, key, out, *options, data=None):
        self.run_openssl('dgst', '-sha256', '-sign', self.dir / (key + '.key'), '-out', self.dir / out, *options, data or self.data)
        return self.dir / out

    def pss(self, salt=32, mgf='sha256'):
        return ('-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:%d' % salt, '-sigopt', 'rsa_mgf1_md:' + mgf)

    def verify(self, key, signature, data=None):
        return ps.verify_file(data or self.data, signature, self.dir / (key + '.pub'))

    def test_good_signatures_verify_for_2048_and_4096_bit_keys(self):
        for key in ('k2048', 'k2048b', 'k4096'):
            with self.subTest(key=key):
                self.assertTrue(self.verify(key, self.sign(key, key + '.sig', *self.pss())))

    def test_other_key_other_bytes_and_bit_flips_fail(self):
        good = self.sign('k2048', 'good.sig', *self.pss())
        self.assertFalse(self.verify('k2048b', good))
        other = self.dir / 'other.bin'
        other.write_bytes(b'x' + self.data.read_bytes()[1:])
        self.assertFalse(self.verify('k2048', good, other))
        raw = bytearray(good.read_bytes())
        for index in (0, len(raw) // 2, len(raw) - 1):
            mutated = bytearray(raw)
            mutated[index] ^= 0x01
            path = self.dir / 'flip.sig'
            path.write_bytes(bytes(mutated))
            self.assertFalse(self.verify('k2048', path), index)

    def test_only_the_fixed_pss_parameters_are_accepted(self):
        self.assertFalse(self.verify('k2048', self.sign('k2048', 'salt20.sig', *self.pss(salt=20))))
        self.assertFalse(self.verify('k2048', self.sign('k2048', 'salt0.sig', *self.pss(salt=0))))
        self.assertFalse(self.verify('k2048', self.sign('k2048', 'mgf1sha1.sig', *self.pss(mgf='sha1'))))
        self.assertFalse(self.verify('k2048', self.sign('k2048', 'v15.sig')))
        sha512 = self.dir / 'sha512.sig'
        self.run_openssl('dgst', '-sha512', '-sign', self.dir / 'k2048.key', '-out', sha512, *self.pss(), self.data)
        self.assertFalse(self.verify('k2048', sha512))

    def test_wrong_length_empty_and_oversized_signatures_fail(self):
        good = self.sign('k2048', 'len.sig', *self.pss())
        for label, blob in (('short', good.read_bytes()[:-1]), ('long', good.read_bytes() + b'\0'), ('empty', b''), ('ones', b'\xff' * 256)):
            path = self.dir / (label + '.bad')
            path.write_bytes(blob)
            with self.subTest(label=label):
                self.assertFalse(self.verify('k2048', path))

    def test_key_restrictions_and_fingerprint(self):
        for name in ('k1024', 'ec'):
            with self.subTest(key=name), self.assertRaises(ps.SignatureError):
                ps.load_public_key(self.dir / (name + '.pub'))
        for name in ('k2048', 'k4096'):
            der = self.run_openssl('pkey', '-pubin', '-in', self.dir / (name + '.pub'), '-outform', 'DER').stdout
            self.assertEqual(ps.spki_sha256(self.dir / (name + '.pub')), hashlib.sha256(der).hexdigest())
        junk = self.dir / 'junk.pub'
        junk.write_text('-----BEGIN PUBLIC KEY-----\nAAAA\n-----END PUBLIC KEY-----\n')
        with self.assertRaises(ps.SignatureError):
            ps.load_public_key(junk)
        private = self.dir / 'k2048.key'
        with self.assertRaises(ps.SignatureError):
            ps.load_public_key(private)

    def test_committed_publisher_key_has_the_published_fingerprint(self):
        key = Path(__file__).resolve().parent.parent / '.github/ci/crt-release-publisher.pub'
        self.assertEqual(ps.spki_sha256(key), '59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2')
        ps.load_public_key(key)


if __name__ == '__main__':
    unittest.main()
