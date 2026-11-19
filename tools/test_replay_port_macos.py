"""Fast host-only fixtures for the D3 replay ledger and denial policy."""
import json
import hashlib
import sys
from pathlib import Path
import tempfile
import unittest

from replay_port_macos import deny_inputs, oracle_input, source_inputs, suite_outcome


class ReplayInputsTest(unittest.TestCase):
    def test_oracle_requires_a_matching_executable_and_explicit_digest_pair(self):
        executable = Path(sys.executable).resolve()
        pin = hashlib.sha256(executable.read_bytes()).hexdigest()
        self.assertEqual(oracle_input(executable, pin)['sha256'], pin)
        self.assertEqual(oracle_input(None, None)['path'], str(executable))
        for path, digest in ((executable, None), (None, pin), (executable, '0' * 64),
                             (executable, 'A' * 64), (Path('relative'), pin)):
            with self.subTest(path=path, digest=digest), self.assertRaises(ValueError):
                oracle_input(path, digest)

    def test_suite_outcomes_reject_skips_wrong_names_and_ambiguous_reports(self):
        name = 'port-build-test'
        good = name + ': 40 checks passed\n'
        self.assertEqual(suite_outcome(name, 0, good, ''), {'passed': True, 'checks': 40, 'skipped': False})
        for code, out, err in ((1, good, ''), (0, good, 'SKIPPED prerequisite'), (0, good + good, ''),
                               (0, 'other: 40 checks passed\n', ''), (0, name + ': 0 checks passed\n', '')):
            with self.subTest(code=code, out=out, err=err):
                self.assertFalse(suite_outcome(name, code, out, err)['passed'])

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
