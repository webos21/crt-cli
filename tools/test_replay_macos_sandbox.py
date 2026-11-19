"""Fast policy-generation checks; real Seatbelt enforcement belongs to native replay."""
from pathlib import Path
import json
import tempfile
import unittest
from unittest.mock import patch

from replay_macos_sandbox import MacSandbox, linker_helpers


class MacSandboxTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.work = Path(self.temporary.name).resolve()
        self.scratch = self.work / 'scratch'; self.scratch.mkdir()
        binaries = self.work / 'tools'; binaries.mkdir()
        (binaries / 'libtool').write_bytes(b'host fixture')
        (binaries / 'ranlib').symlink_to('libtool')
        tools = {role: binaries / name for role, name in [('cc', 'clang'), ('cxx', 'clang++'), ('ar', 'ar'), ('linker', 'ld')]}
        self.sandbox = MacSandbox()
        with patch('replay_macos_sandbox.sys.platform', 'darwin'), patch.object(Path, 'is_file', return_value=True), \
                patch('replay_macos_sandbox.host_compile'):
            self.sandbox.prepare(self.work, self.scratch, self.work / 'crt-cli', tools, [self.work / 'hidden input'])

    def test_policy_keeps_read_write_and_exec_boundaries(self):
        lock = self.scratch / 'crt.lock'
        argv = self.sandbox.command(['new', 'space quote " argument'], self.scratch, readonly_paths=[lock])
        policy = Path(argv[2]).read_text()
        self.assertEqual(['new', 'space quote " argument'], argv[-2:])
        self.assertIn('(deny network*)', policy)
        self.assertIn('(deny file-read* file-write* (subpath ', policy)
        self.assertIn('(deny file-write* (literal ' + json.dumps(str(lock)) + '))', policy)
        self.assertIn('(deny process-exec (require-not ', policy)

    def test_declares_canonical_ranlib_helper(self):
        self.assertIn(self.work / 'tools/libtool', self.sandbox.allowed)

    def test_linker_dispatcher_helper_is_optional_and_exact(self):
        linker = self.work / 'tools/ld'
        helper = linker.with_name('ld64')
        self.assertEqual({}, linker_helpers(linker))
        helper.write_bytes(b'Xcode linker fixture')
        helper.chmod(0o600)
        self.assertEqual({}, linker_helpers(linker))
        helper.chmod(0o700)
        self.assertEqual({'ld64': helper}, linker_helpers(linker))
        classic = linker.with_name('ld-classic')
        classic.write_bytes(b'Xcode legacy linker fixture')
        classic.chmod(0o700)
        self.assertEqual({'ld64': helper, 'ld-classic': classic}, linker_helpers(linker))
        (self.work / 'tools/undeclared-helper').write_bytes(b'not allowed')
        (self.work / 'second').mkdir()
        with patch('replay_macos_sandbox.sys.platform', 'darwin'), patch('replay_macos_sandbox.host_compile'):
            another = MacSandbox()
            another.prepare(self.work / 'second', self.scratch, self.work / 'crt-cli',
                            {'cc': linker, 'ar': linker, 'linker': linker}, [])
        self.assertIn(helper, another.allowed)
        self.assertIn(classic, another.allowed)
        policy = Path(another.command([], self.scratch)[2]).read_text()
        self.assertIn('(literal ' + json.dumps(str(helper)) + ')', policy)
        self.assertNotIn('(subpath ' + json.dumps(str(helper.parent)) + ')', policy)
        self.assertNotIn('undeclared-helper', policy)

    def test_network_opt_in_does_not_remove_other_boundaries(self):
        self.sandbox.network = True
        argv = self.sandbox.command([], self.scratch)
        policy = Path(argv[2]).read_text()
        self.assertNotIn('(deny network*)', policy)
        self.assertIn('(deny file-read* file-write*', policy)
        self.assertIn('(deny process-exec', policy)

    def test_child_environment_is_explicit(self):
        env = self.sandbox.process_options(self.scratch)['env']
        self.assertEqual({'HOME', 'PATH', 'TMPDIR', 'LC_ALL'}, set(env))
        self.assertEqual('/nonexistent', env['PATH'])
        self.sandbox.hostile = self.scratch / 'hostile'
        env = self.sandbox.process_options(self.scratch)['env']
        for key in ('PATH', 'DYLD_LIBRARY_PATH', 'LIBRARY_PATH', 'CPATH', 'CRT_SYSROOT', 'CRT_ROOTFS'):
            self.assertEqual(str(self.sandbox.hostile), env[key])

    def test_hostile_dyld_is_injected_inside_seatbelt(self):
        self.sandbox.hostile = self.scratch / 'hostile'
        argv = self.sandbox.command(['doctor'], self.scratch)
        self.assertEqual(str(self.sandbox.probe), argv[3])
        self.assertEqual('environment', argv[4])
        self.assertIn('DYLD_LIBRARY_PATH', argv[5:])
        self.assertIn('DYLD_FALLBACK_LIBRARY_PATH', argv[5:])
        self.assertEqual([str(self.sandbox.cli), '--json', 'doctor'], argv[argv.index('--')+1:])


if __name__ == '__main__':
    unittest.main()
