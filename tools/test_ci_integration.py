"""Integration pin gates: an empty or wrong pin must fail, never skip."""
import json
import tempfile
import unittest
from pathlib import Path

from ci_integration import IntegrationError, load_pins, sha256_file, verify_download

GOOD_ZLIB = {'url': 'https://example.invalid/zlib.tar.gz', 'name': 'zlib.tar.gz', 'sha256': 'a' * 64, 'size': 10}
GOOD_TOOLS = {'url': 'https://example.invalid/tools.tar.xz', 'name': 'tools.tar.xz', 'sha256': 'b' * 64, 'size': 20,
              'manifest_sha256': 'c' * 64, 'version': 'v0'}


def write(directory, zlib=None, tools=None, schema='crt-cli.ci-integration-pins'):
    path = Path(directory) / 'pins.json'
    path.write_text(json.dumps({'schema': schema, 'format': 0, 'zlib_source': zlib or dict(GOOD_ZLIB),
                                'hosts': {'linux-x64': {'host_tools': tools or dict(GOOD_TOOLS)}}}))
    return path


class PinTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_complete_pins_load(self):
        zlib, tools = load_pins(write(self.dir), 'linux-x64')
        self.assertEqual((zlib['size'], tools['version']), (10, 'v0'))

    def test_every_empty_host_tool_pin_fails(self):
        for name in GOOD_TOOLS:
            tools = dict(GOOD_TOOLS, **{name: None})
            with self.subTest(field=name), self.assertRaisesRegex(IntegrationError, 'unpinned'):
                load_pins(write(self.dir, tools=tools), 'linux-x64')

    def test_empty_zlib_pin_fails(self):
        with self.assertRaisesRegex(IntegrationError, 'unpinned size'):
            load_pins(write(self.dir, zlib=dict(GOOD_ZLIB, size=None)), 'linux-x64')

    def test_http_malformed_digest_and_bad_size_fail(self):
        for tools, message in [(dict(GOOD_TOOLS, url='http://example.invalid/t'), 'https'),
                               (dict(GOOD_TOOLS, sha256='B' * 64), 'lowercase SHA-256'),
                               (dict(GOOD_TOOLS, manifest_sha256='abc'), 'lowercase SHA-256'),
                               (dict(GOOD_TOOLS, size='20'), 'positive integer'),
                               (dict(GOOD_TOOLS, size=0), 'positive integer')]:
            with self.subTest(message=message), self.assertRaisesRegex(IntegrationError, message):
                load_pins(write(self.dir, tools=tools), 'linux-x64')

    def test_unknown_host_schema_and_missing_file_fail(self):
        with self.assertRaisesRegex(IntegrationError, 'no integration pins for host'):
            load_pins(write(self.dir), 'win-x64')
        with self.assertRaisesRegex(IntegrationError, 'unsupported'):
            load_pins(write(self.dir, schema='other'), 'linux-x64')
        with self.assertRaises(FileNotFoundError):
            load_pins(self.dir / 'absent.json', 'linux-x64')

    def test_download_must_match_size_and_digest(self):
        blob = self.dir / 'blob'
        blob.write_bytes(b'hello')
        entry = {'size': 5, 'sha256': sha256_file(blob)}
        verify_download('blob', blob, entry)
        for bad, message in [(dict(entry, size=6), 'size'), (dict(entry, sha256='0' * 64), 'SHA-256')]:
            with self.subTest(message=message), self.assertRaisesRegex(IntegrationError, message):
                verify_download('blob', blob, bad)
        with self.assertRaisesRegex(IntegrationError, 'missing'):
            verify_download('blob', self.dir / 'nope', entry)

    def test_committed_pins_fail_until_host_tools_are_pinned(self):
        root = Path(__file__).resolve().parent.parent / '.github/ci/integration-pins.json'
        pins = json.loads(root.read_text())
        # The zlib source is pinned to bytes the recipe also pins; the host tools stay unpinned until CRT publishes them.
        recipe = json.loads((Path(__file__).resolve().parent.parent / 'src/test/data/zlib.recipe.json').read_text())
        self.assertEqual(pins['zlib_source']['sha256'], recipe['source']['sha256'])
        for host in pins['hosts']:
            if any(v in (None, '') for v in pins['hosts'][host]['host_tools'].values()):
                with self.assertRaisesRegex(IntegrationError, 'unpinned'):
                    load_pins(root, host)


if __name__ == '__main__':
    unittest.main()
