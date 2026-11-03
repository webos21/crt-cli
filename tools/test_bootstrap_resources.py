"""Producer-only resource packaging regressions; fixtures do not prove native execution."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('resources', Path(__file__).with_name('create_bootstrap_resources.py'))
resources = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resources)


class Resources(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(); self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name); self.sdk = self.root / 'sdk'; self.sdk.mkdir()
        target = {'os': 'windows' if os.name == 'nt' else 'linux', 'arch': 'x86_64'}
        self.executable = 'crt-cli.exe' if target['os'] == 'windows' else 'crt-cli'
        (self.sdk / 'manifest.json').write_text(json.dumps({'target': target}))
        (self.sdk / 'LICENSE.md').write_text('fixture notice')
        for component in resources.COMPONENTS:
            for prefix in ('share/licenses', 'share/crt/dependencies'):
                directory = self.sdk / prefix / component; directory.mkdir(parents=True)
                (directory / 'NOTICE').write_text(component)
        self.binary = self.root / 'binary'; self.binary.write_bytes(b'fixture'); self.binary.chmod(0o755)
        self.output = self.root / 'candidate'

    def test_inventory_covers_one_executable_and_explicit_trust(self):
        ca = self.root / 'ca'; ca.write_text('explicit test CA')
        pin = resources.assemble(self.binary, self.sdk, self.output, ca)
        manifest = json.loads((self.output / 'manifest.json').read_text())
        self.assertEqual(pin, resources.digest(self.output / 'manifest.json'))
        self.assertEqual(manifest['trust']['ca_file'], 'share/trust/ca.pem')
        self.assertEqual(manifest['executable'], self.executable)
        self.assertEqual((self.output / self.executable).read_bytes(), self.binary.read_bytes())
        self.assertEqual([p.name for p in self.output.rglob('*') if p.is_file() and p.stat().st_mode & 0o111], [self.executable])
        self.assertIn(['share/trust/ca.pem', 'file', resources.digest(ca)], manifest['inventory'])
        before = (self.output / 'manifest.json').read_bytes()
        with self.assertRaisesRegex(ValueError, 'already exists'):
            resources.assemble(self.binary, self.sdk, self.output)
        self.assertEqual(before, (self.output / 'manifest.json').read_bytes())

    def test_missing_notice_and_external_link_publish_nothing(self):
        notice = self.sdk / 'share/licenses/curl/NOTICE'; notice.unlink()
        notice.symlink_to(self.binary)
        with self.assertRaisesRegex(ValueError, 'symlinks'):
            resources.assemble(self.binary, self.sdk, self.output)
        self.assertFalse(self.output.exists()); self.assertEqual([], list(self.root.glob('.bootstrap-*')))
        notice.unlink(); (self.sdk / 'share/licenses/curl').rmdir()
        with self.assertRaises(FileNotFoundError):
            resources.assemble(self.binary, self.sdk, self.output)
        self.assertFalse(self.output.exists()); self.assertEqual([], list(self.root.glob('.bootstrap-*')))

    def test_linked_output_parent_cannot_change_sdk(self):
        alias = self.root / 'sdk-alias'; alias.symlink_to(self.sdk, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'disjoint'):
            resources.assemble(self.binary, self.sdk, alias / 'candidate')
        self.assertFalse((self.sdk / 'candidate').exists())


if __name__ == '__main__':
    unittest.main()
