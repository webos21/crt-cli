"""Host-neutral fixtures; native provider acceptance is the separate macOS replay."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from replay_c_shared_macos import image_paths, link_command, replace_provider


class MacCProviderFixtures(unittest.TestCase):
    def test_image_enumeration_preserves_full_paths_and_deduplicates(self):
        with tempfile.TemporaryDirectory(prefix='images space ') as directory:
            image = Path(directory).resolve() / 'libc.dylib'
            self.assertEqual(image_paths('noise\nIMAGE:' + str(image) + '\nIMAGE:' + str(image) + '\n'), [str(image)])

    def test_image_enumeration_rejects_missing_or_relative_paths(self):
        for text in ('IMAGE:\n', 'IMAGE:relative/libc.dylib\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                image_paths(text)

    def test_link_receipt_is_exact_and_never_shell_split(self):
        executable = Path('/consumer space/app')
        command = ['/declared/clang', '/consumer space/main.o', '-o', str(executable)]
        receipt = {'commands': [['/declared/clang', '-c', 'main.c', '-o', str(executable)], command]}
        self.assertEqual(link_command(receipt, executable), command)
        self.assertIsNot(link_command(receipt, executable), command)

    def test_missing_duplicate_or_other_target_link_is_refused(self):
        executable = Path('/consumer/app')
        command = ['clang', '-o', str(executable)]
        for commands in ([], [command, command], [['clang', '-c', '-o', str(executable)]], [['clang', '-o', '/other/app']]):
            with self.subTest(commands=commands), self.assertRaises(ValueError):
                link_command({'commands': commands}, executable)

    def test_explicit_provider_matches_alias_or_canonical_input_exactly_once(self):
        alias, canonical, replacement = Path('/SDK/lib/libz.dylib'), Path('/SDK/lib/libz.1.3.1.dylib'), Path('/explicit/libz.1.dylib')
        with patch.object(Path, 'resolve', return_value=canonical):
            for selected in (str(alias), str(canonical)):
                command = ['clang', selected, '-o', '/app']
                self.assertEqual(replace_provider(command, alias, replacement), ['clang', str(replacement), '-o', '/app'])
                self.assertIn(selected, command)
            for command in (['clang', '/host/libz.dylib'], ['clang', str(alias), str(canonical)]):
                with self.assertRaises(ValueError):
                    replace_provider(command, alias, replacement)


if __name__ == '__main__':
    unittest.main()
