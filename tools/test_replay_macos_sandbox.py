"""Fast policy-generation checks; real Seatbelt enforcement belongs to native replay."""
from pathlib import Path
import json
import tempfile
import unittest
from unittest.mock import patch

from replay_macos_sandbox import MacSandbox


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
