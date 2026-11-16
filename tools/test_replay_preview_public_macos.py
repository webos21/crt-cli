"""Ensure public-byte replay never mistakes a launcher failure for lock refusal."""
import json
import subprocess
import unittest

from replay_preview_public_macos import evaluate_reply


class ResultTests(unittest.TestCase):
    def outcome(self, code, stdout):
        return evaluate_reply(subprocess.CompletedProcess([], code, stdout, ''),
                              expect_ok=False, expect_text='identity mismatch')[1]

    def test_structured_refusal_requires_positive_exit_and_matching_diagnostic(self):
        reply = {'schema': 'crt-cli.bootstrap-result', 'format': 1, 'ok': False,
                 'data': {'message': 'identity mismatch'}}
        encoded = json.dumps(reply)
        self.assertTrue(self.outcome(1, encoded))
        for code in (0, -9):
            self.assertFalse(self.outcome(code, encoded))
        reply['data']['message'] = 'unrelated failure'
        self.assertFalse(self.outcome(1, json.dumps(reply)))

    def test_launcher_and_malformed_results_never_pass(self):
        for stdout in ('', 'sandbox-exec: identity mismatch', '[]', 'null',
                       '{"ok": false, "data": {"message": "identity mismatch"}}',
                       '{"schema":"crt-cli.bootstrap-result","format":1,"ok":0,"data":{"message":"identity mismatch"}}'):
            with self.subTest(stdout=stdout):
                self.assertFalse(self.outcome(1, stdout))

    def test_success_requires_valid_result_and_success_exit(self):
        reply = json.dumps({'schema': 'crt-cli.bootstrap-result', 'format': 1, 'ok': True, 'data': {}})
        for code, expected in ((0, True), (1, False), (-9, False)):
            self.assertEqual(expected, evaluate_reply(subprocess.CompletedProcess([], code, reply, ''))[1])


if __name__ == '__main__':
    unittest.main()
