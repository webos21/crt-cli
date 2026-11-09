"""Fast host-only fixtures for the D3 replay ledger and denial policy."""
import json
from pathlib import Path
import tempfile
import unittest

from replay_port_macos import deny_inputs, source_inputs


class ReplayInputsTest(unittest.TestCase):
    def test_source_scope_and_lf_normalization(self):
        with tempfile.TemporaryDirectory() as location:
            root = Path(location)
            (root / 'src/main').mkdir(parents=True)
            (root / 'include').mkdir()
            source = root / 'src/main/main.cpp'
            source.write_bytes(b'int main() {\r\n return 0;\r\n}\r\n')
            (root / 'include/api.h').write_text('void api(void);\n')
            (root / 'crt.json').write_text('{}\n')
            initial = source_inputs(root)
            source.write_bytes(source.read_bytes().replace(b'\r\n', b'\n'))
            self.assertEqual(initial, source_inputs(root))
            (root / 'README.md').write_text('not a compile input\n')
            (root / 'out').mkdir()
            (root / 'out/image').write_bytes(b'generated')
            self.assertEqual(initial, source_inputs(root))
            source.write_text('int changed;\n')
            self.assertNotEqual(initial, source_inputs(root))

    def test_hidden_input_paths_are_quoted(self):
        paths = [Path('/tmp/SDK space 한글'), Path('/tmp/policy"quoted.json')]
        policy = deny_inputs('(version 1)\n(allow default)\n', paths)
        self.assertTrue(policy.startswith('(version 1)\n(allow default)\n'))
        self.assertEqual(policy.count('(deny file-read* file-write*'), 2)
        for path in paths:
            self.assertIn(json.dumps(str(path), ensure_ascii=False), policy)


if __name__ == '__main__':
    unittest.main()
