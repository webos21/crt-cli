"""CI report gates: parallel execution must not hide missing checks or failed sections."""
import json
import unittest

from ci_bootstrap import CiError, host_key, native_phase_report


class NativePhaseReportTest(unittest.TestCase):
    def setUp(self):
        names = ['startup', 'process', 'selection', 'projects/c', 'projects/cxx'] + [
            'variants/' + language + '/' + kind for language in ('c', 'cxx')
            for kind in ('static-library', 'shared-library', 'executable')]
        self.phases = [dict(name=name, checks=1, seconds=0.25, completed=True) for name in names]

    def report(self, newline='\n'):
        return newline.join(['Hello from CRT'] + ['native-phase: ' + json.dumps(p) for p in self.phases])

    def test_complete_report_accepts_both_line_endings(self):
        for newline in ('\n', '\r\n'):
            self.assertEqual(len(native_phase_report(self.report(newline), 11)), 11)

    def test_missing_section_is_not_a_smaller_pass(self):
        self.phases.pop()
        with self.assertRaisesRegex(CiError, 'coverage'):
            native_phase_report(self.report(), 10)

    def test_duplicate_section_cannot_replace_another(self):
        self.phases[-1] = self.phases[0].copy()
        with self.assertRaisesRegex(CiError, 'coverage'):
            native_phase_report(self.report(), 11)

    def test_incomplete_section_fails(self):
        self.phases[0]['completed'] = False
        with self.assertRaisesRegex(CiError, 'did not complete'):
            native_phase_report(self.report(), 11)

    def test_count_must_match_the_native_total(self):
        with self.assertRaisesRegex(CiError, 'counts differ'):
            native_phase_report(self.report(), 12)

    def test_invalid_telemetry_fails(self):
        for field, value in [('checks', -1), ('checks', True), ('seconds', -1), ('seconds', float('nan'))]:
            saved = self.phases[0][field]
            self.phases[0][field] = value
            with self.assertRaises(CiError):
                native_phase_report(self.report(), 11)
            self.phases[0][field] = saved

    def test_malformed_report_is_a_ci_failure(self):
        with self.assertRaisesRegex(CiError, 'malformed'):
            native_phase_report('native-phase: {broken', 11)


if __name__ == '__main__':
    unittest.main()


class HostKeyTest(unittest.TestCase):
    def test_supported_hosts(self):
        for system, machine, key in [('linux', 'x86_64', 'linux-x64'), ('linux', 'aarch64', 'linux-arm64'), ('linux', 'arm64', 'linux-arm64'),
                                     ('darwin', 'arm64', 'macos-arm64'), ('win32', 'AMD64', 'win-x64')]:
            self.assertEqual(host_key(system, machine), key)

    def test_unsupported_hosts_are_refused(self):
        for system, machine in [('linux', 'riscv64'), ('darwin', 'x86_64'), ('win32', 'ARM64')]:
            with self.assertRaisesRegex(CiError, 'unsupported CI host'):
                host_key(system, machine)
