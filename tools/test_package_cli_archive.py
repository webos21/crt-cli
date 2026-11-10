import hashlib, json, os, tarfile, tempfile, unittest
from pathlib import Path
import package_cli_archive as tool


def make_package(root):
    root.mkdir(parents=True)
    files = {'crt-cli': b'\x7fexe', 'LICENSE.md': b'sdk license', 'share/licenses/crt-cli/LICENSE': b'apache'}
    for component in tool.REQUIRED_COMPONENTS:
        files[f'share/licenses/{component}/LICENSE'] = component.encode()
    inventory = []
    for name, data in sorted(files.items()):
        path = root / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)
        path.chmod(0o755 if name == 'crt-cli' else 0o644)
        inventory.append([name, 'file', hashlib.sha256(data).hexdigest()])
    (root / 'manifest.json').write_text(json.dumps({'schema': 'crt-cli.bootstrap-resources', 'format': 1, 'executable': 'crt-cli',
                                                    'inventory': inventory}))
    return root


class Package(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.package = make_package(self.tmp / 'crt-cli-test')

    def test_pack_is_deterministic_and_verifies(self):
        first = tool.pack(self.package, self.tmp / 'a.tar.xz')
        second = tool.pack(self.package, self.tmp / 'b.tar.xz')
        self.assertEqual(first['sha256'], second['sha256'])
        result = tool.verify(self.tmp / 'a.tar.xz', self.tmp / 'out')
        self.assertEqual(first['files'], result['files'])
        self.assertTrue(os.access(Path(result['root']) / 'crt-cli', os.X_OK))
        self.assertFalse(os.access(Path(result['root']) / 'LICENSE.md', os.X_OK))

    def test_refusals_before_and_after_packing(self):
        with self.assertRaisesRegex(ValueError, 'already exists'):
            (self.tmp / 'x.tar.xz').write_bytes(b''); tool.pack(self.package, self.tmp / 'x.tar.xz')
        (self.package / 'extra.txt').write_text('x')
        with self.assertRaisesRegex(ValueError, 'extra'):
            tool.pack(self.package, self.tmp / 'y.tar.xz')
        (self.package / 'extra.txt').unlink()
        (self.package / 'LICENSE.md').write_text('changed')
        with self.assertRaisesRegex(ValueError, 'changed'):
            tool.pack(self.package, self.tmp / 'z.tar.xz')

    def test_missing_notice_and_second_executable_fail_closed(self):
        (self.package / 'share/licenses/xz/LICENSE').unlink()
        manifest = json.loads((self.package / 'manifest.json').read_text())
        manifest['inventory'] = [e for e in manifest['inventory'] if e[0] != 'share/licenses/xz/LICENSE']
        (self.package / 'manifest.json').write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'xz'):
            tool.inspect(self.package)
        (self.package / 'share/licenses/xz/NOTICE').parent.mkdir(parents=True, exist_ok=True)
        (self.package / 'share/licenses/xz/NOTICE').write_text('n')
        manifest['inventory'].append(['share/licenses/xz/NOTICE', 'file', hashlib.sha256(b'n').hexdigest()])
        (self.package / 'manifest.json').write_text(json.dumps(manifest))
        (self.package / 'share/licenses/xz/NOTICE').chmod(0o755)
        with self.assertRaisesRegex(ValueError, 'only executable'):
            tool.inspect(self.package)

    def test_hostile_archive_members_are_refused(self):
        evil = self.tmp / 'evil.tar.xz'
        with tarfile.open(evil, 'w:xz') as archive:
            info = tarfile.TarInfo('top/link'); info.type = tarfile.SYMTYPE; info.linkname = '/etc/passwd'; archive.addfile(info)
        with self.assertRaisesRegex(ValueError, 'link or special'):
            tool.verify(evil, self.tmp / 'evil-out')
        two = self.tmp / 'two.tar.xz'
        with tarfile.open(two, 'w:xz') as archive:
            for name in ('a/x', 'b/y'):
                info = tarfile.TarInfo(name); info.size = 0; archive.addfile(info)
        with self.assertRaisesRegex(ValueError, 'one top-level'):
            tool.verify(two, self.tmp / 'two-out')


if __name__ == '__main__':
    unittest.main()
