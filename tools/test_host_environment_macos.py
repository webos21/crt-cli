"""Portable policy fixtures; native boundary proof belongs to the macOS replay."""
import importlib.util
from pathlib import Path, PurePosixPath
import unittest

spec = importlib.util.spec_from_file_location('macos_replay', Path(__file__).with_name('replay_host_environment_macos.py'))
replay = importlib.util.module_from_spec(spec)
spec.loader.exec_module(replay)


class SeatbeltPolicy(unittest.TestCase):
    def test_hidden_inputs_and_immutable_payloads_have_distinct_denials(self):
        text = replay.policy_text([PurePosixPath('/source')], [PurePosixPath('/work/cli')], [PurePosixPath('/work/tools')])
        self.assertIn('(deny file-read* file-write* (subpath "/source"))', text)
        self.assertIn('(deny file-write* (subpath "/work/tools"))', text)
        self.assertNotIn('(deny file-read* file-write* (subpath "/work/tools"))', text)
        self.assertIn('(deny network*)', text)

    def test_execution_allowlist_is_literal_sorted_and_deduplicated(self):
        text = replay.policy_text([], [PurePosixPath('/z/cli'), PurePosixPath('/a/mksh'), PurePosixPath('/z/cli')], [])
        self.assertIn('(require-any (literal "/a/mksh") (literal "/z/cli"))', text)
        self.assertEqual(text.count('(literal "/z/cli")'), 1)
        self.assertNotIn('/bin/sh', text)

    def test_unicode_is_not_json_escaped_and_quotes_are_escaped(self):
        text = replay.policy_text([PurePosixPath('/source space 한글')], [PurePosixPath('/work/"quoted"/cli')], [])
        self.assertIn('/source space 한글', text)
        self.assertNotIn('\\ud55c', text)
        self.assertIn('\\"quoted\\"', text)


if __name__ == '__main__':
    unittest.main()
