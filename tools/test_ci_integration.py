"""Integration pin gates: an empty or wrong pin must fail, never skip."""
import copy
import hashlib
import io
import json
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from ci_integration import (IntegrationError, PORT_SUITES, judge_port_result, load_pins, main,
                            prepare_port_inputs, sha256_file, verify_download, verify_oracle)
from replay_port_linux import suite_outcome

GOOD_ZLIB = {'url': 'https://example.invalid/zlib.tar.gz', 'name': 'zlib.tar.gz', 'sha256': 'a' * 64, 'size': 10}
GOOD_TOOLS = {'url': 'https://example.invalid/tools.tar.xz', 'name': 'tools.tar.xz', 'sha256': 'b' * 64, 'size': 20,
              'manifest_sha256': 'c' * 64, 'version': 'v0'}


def write(directory, zlib=None, tools=None, schema='crt-cli.ci-integration-pins'):
    path = Path(directory) / 'pins.json'
    path.write_text(json.dumps({'schema': schema, 'format': 0, 'zlib_source': zlib or dict(GOOD_ZLIB),
                                'hosts': {'linux-x64': {'host_tools': tools or dict(GOOD_TOOLS)}}}))
    return path


class PinTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_complete_pins_load(self):
        zlib, tools = load_pins(write(self.dir), 'linux-x64')
        self.assertEqual((zlib['size'], tools['version']), (10, 'v0'))

    def test_every_empty_host_tool_pin_fails(self):
        for name in GOOD_TOOLS:
            tools = dict(GOOD_TOOLS, **{name: None})
            with self.subTest(field=name), self.assertRaisesRegex(IntegrationError, 'unpinned'):
                load_pins(write(self.dir, tools=tools), 'linux-x64')

    def test_empty_zlib_pin_fails(self):
        with self.assertRaisesRegex(IntegrationError, 'unpinned size'):
            load_pins(write(self.dir, zlib=dict(GOOD_ZLIB, size=None)), 'linux-x64')

    def test_http_malformed_digest_and_bad_size_fail(self):
        for tools, message in [(dict(GOOD_TOOLS, url='http://example.invalid/t'), 'https'),
                               (dict(GOOD_TOOLS, sha256='B' * 64), 'lowercase SHA-256'),
                               (dict(GOOD_TOOLS, manifest_sha256='abc'), 'lowercase SHA-256'),
                               (dict(GOOD_TOOLS, size='20'), 'positive integer'),
                               (dict(GOOD_TOOLS, size=0), 'positive integer')]:
            with self.subTest(message=message), self.assertRaisesRegex(IntegrationError, message):
                load_pins(write(self.dir, tools=tools), 'linux-x64')

    def test_unknown_host_schema_and_missing_file_fail(self):
        with self.assertRaisesRegex(IntegrationError, 'no integration pins for host'):
            load_pins(write(self.dir), 'win-x64')
        with self.assertRaisesRegex(IntegrationError, 'unsupported'):
            load_pins(write(self.dir, schema='other'), 'linux-x64')
        with self.assertRaises(FileNotFoundError):
            load_pins(self.dir / 'absent.json', 'linux-x64')

    def test_download_must_match_size_and_digest(self):
        blob = self.dir / 'blob'
        blob.write_bytes(b'hello')
        entry = {'size': 5, 'sha256': sha256_file(blob)}
        verify_download('blob', blob, entry)
        for bad, message in [(dict(entry, size=6), 'size'), (dict(entry, sha256='0' * 64), 'SHA-256')]:
            with self.subTest(message=message), self.assertRaisesRegex(IntegrationError, message):
                verify_download('blob', blob, bad)
        with self.assertRaisesRegex(IntegrationError, 'missing'):
            verify_download('blob', self.dir / 'nope', entry)

    def test_committed_pins_name_published_preview_bundles(self):
        root = Path(__file__).resolve().parent.parent / '.github/ci/integration-pins.json'
        pins = json.loads(root.read_text())
        recipe = json.loads((Path(__file__).resolve().parent.parent / 'src/test/data/zlib.recipe.json').read_text())
        self.assertEqual(pins['zlib_source']['sha256'], recipe['source']['sha256'])
        for host, arch in (('linux-x64', 'x86_64'), ('linux-arm64', 'aarch64')):
            with self.subTest(host=host):
                zlib, tools = load_pins(root, host)
                self.assertTrue(tools['url'].startswith('https://github.com/webos21/crt/releases/download/' + tools['version'] + '/'))
                self.assertIn('linux-' + arch, tools['version'])
                self.assertEqual(tools['name'], 'crt-' + tools['version'] + '.tar.xz')


class PortDriverTest(unittest.TestCase):
    """A test-only archive exercises the real driver; it is not a runnable CRT bundle."""
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.inputs = self.dir / 'local inputs 한글'
        self.inputs.mkdir()
        source = self.inputs / 'zlib.tar.gz'
        source.write_bytes(b'test-only source, never executed')
        self.zlib = dict(GOOD_ZLIB, size=source.stat().st_size, sha256=sha256_file(source))
        self.manifest = {'schema': 'crt.experimental.host-tools', 'format': 1, 'promotion': 'local-only',
                         'host': {'os': 'linux', 'arch': 'x86_64'}, 'version': 'v0',
                         'assembler': {'working_tree_dirty': False}}
        self.tools = self.bundle()

    def tearDown(self):
        self.tmp.cleanup()

    def bundle(self, extra=(), manifest=None):
        data = (json.dumps(self.manifest if manifest is None else manifest) + '\n').encode()
        archive = self.inputs / 'tools.tar.xz'
        with tarfile.open(archive, 'w:xz') as bundle:
            item = tarfile.TarInfo('bundle/manifest.json')
            item.size = len(data)
            bundle.addfile(item, io.BytesIO(data))
            for item, payload in extra:
                bundle.addfile(item, io.BytesIO(payload) if payload is not None else None)
        return dict(GOOD_TOOLS, size=archive.stat().st_size, sha256=sha256_file(archive),
                    manifest_sha256=hashlib.sha256(data).hexdigest())

    def prepare(self, **kwargs):
        return prepare_port_inputs(self.dir / 'fresh work 한글', 'linux-x64', self.zlib, self.tools,
                                   input_dir=self.inputs, **kwargs)

    def test_local_bundle_prepares_without_execution_and_preserves_inputs(self):
        before = {p.name: p.read_bytes() for p in self.inputs.iterdir()}
        result = self.prepare()
        self.assertEqual(result['native_validation'], 'not_run')
        self.assertNotIn('passed', result)
        self.assertTrue((Path(result['host_tools_root']) / 'manifest.json').is_file())
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.inputs.iterdir()})
        self.assertEqual(json.loads((self.dir / 'fresh work 한글/port-inputs.json').read_text()), result)
        with self.assertRaises(FileExistsError):
            self.prepare()

    def test_prepare_only_cli_uses_the_same_driver_and_has_no_native_result(self):
        pins = write(self.dir, zlib=self.zlib, tools=self.tools)
        work = self.dir / 'cli-work'
        with patch('sys.argv', ['ci_integration.py', 'ports', '--host', 'linux-x64', '--pins', str(pins),
                                '--work', str(work), '--input-dir', str(self.inputs), '--prepare-only']):
            self.assertEqual(main(), 0)
        self.assertEqual(json.loads((work / 'port-inputs.json').read_text())['native_validation'], 'not_run')
        self.assertFalse((work / 'replay/result.json').exists())

    def test_native_execution_rejects_cross_host_before_work_or_launch(self):
        pins = write(self.dir, zlib=self.zlib, tools=self.tools)
        data = json.loads(pins.read_text())
        data['hosts']['linux-arm64'] = data['hosts']['linux-x64']
        pins.write_text(json.dumps(data))
        work = self.dir / 'cross-host'
        with patch('ci_integration.host_key', return_value='linux-x64'), patch('sys.argv',
                ['ci_integration.py', 'ports', '--host', 'linux-arm64', '--pins', str(pins), '--work', str(work)]):
            self.assertEqual(main(), 1)
        self.assertFalse(work.exists())

    def test_online_interface_requires_a_published_preview_bundle(self):
        def download(url, destination):
            Path(destination).write_bytes((self.inputs / Path(destination).name).read_bytes())
        with patch('ci_integration.download', side_effect=download):
            with self.assertRaisesRegex(IntegrationError, 'promotion preview'):
                prepare_port_inputs(self.dir / 'online-local-only', 'linux-x64', self.zlib, self.tools)
        self.assertFalse((self.dir / 'online-local-only/host-tools').exists())

    def test_online_interface_verifies_downloaded_bytes(self):
        def download(url, destination):
            Path(destination).write_bytes((self.inputs / Path(destination).name).read_bytes())
        self.tools = self.bundle(manifest=dict(self.manifest, promotion='preview'))
        with patch('ci_integration.download', side_effect=download) as get:
            prepare_port_inputs(self.dir / 'online', 'linux-x64', self.zlib, self.tools)
        self.assertEqual(get.call_count, 2)
        with patch('ci_integration.download', side_effect=lambda url, path: Path(path).write_bytes(b'bad')):
            with self.assertRaisesRegex(IntegrationError, 'size'):
                prepare_port_inputs(self.dir / 'bad-online', 'linux-x64', self.zlib, self.tools)

    def test_missing_or_changed_local_input_never_creates_a_prepared_record(self):
        source = self.inputs / self.zlib['name']
        original = source.read_bytes()
        for index, payload in enumerate((None, b'x', b'x' * len(original))):
            if payload is None:
                source.unlink()
            else:
                source.write_bytes(payload)
            work = self.dir / ('failed-%d' % index)
            with self.subTest(payload=payload), self.assertRaisesRegex(IntegrationError, 'missing|size|SHA-256'):
                prepare_port_inputs(work, 'linux-x64', self.zlib, self.tools, input_dir=self.inputs)
            self.assertFalse((work / 'port-inputs.json').exists())

    def test_manifest_pin_and_host_version_provenance_are_required(self):
        variants = [dict(self.tools, manifest_sha256='0' * 64)]
        for field, value in [('host', {'os': 'linux', 'arch': 'aarch64'}), ('version', 'other'),
                             ('assembler', {'working_tree_dirty': True}), ('assembler', {})]:
            variants.append(self.bundle(manifest=dict(self.manifest, **{field: value})))
            # Consume each separately because the archive path is shared by this fixture.
            with self.subTest(field=field, value=value), self.assertRaisesRegex(IntegrationError, 'host/version|provenance'):
                prepare_port_inputs(self.dir / ('manifest-%d' % len(variants)), 'linux-x64', self.zlib,
                                    variants[-1], input_dir=self.inputs)
        self.tools = self.bundle()
        with self.assertRaisesRegex(IntegrationError, 'manifest SHA-256'):
            prepare_port_inputs(self.dir / 'wrong-manifest-pin', 'linux-x64', self.zlib,
                                dict(self.tools, manifest_sha256='0' * 64), input_dir=self.inputs)

    def test_archive_traversal_absolute_paths_links_and_case_collisions_fail(self):
        examples = []
        for name in ('../escape', '/absolute', 'C:/drive', 'bundle/../escape', 'bundle\\escape'):
            member = tarfile.TarInfo(name)
            member.size = 1
            examples.append([(member, b'x')])
        link = tarfile.TarInfo('bundle/escape')
        link.type, link.linkname = tarfile.SYMTYPE, '../../outside'
        examples.append([(link, None)])
        hardlink = tarfile.TarInfo('bundle/hard')
        hardlink.type, hardlink.linkname = tarfile.LNKTYPE, 'bundle/manifest.json'
        examples.append([(hardlink, None)])
        for names in [('bundle/A', 'bundle/a'), ('bundle/a', 'bundle/a')]:
            examples.append([(tarfile.TarInfo(name), b'') for name in names])
        for index, extra in enumerate(examples):
            pin = self.bundle(extra)
            work = self.dir / ('unsafe-%d' % index)
            with self.subTest(index=index), self.assertRaises((IntegrationError, tarfile.TarError)):
                prepare_port_inputs(work, 'linux-x64', self.zlib, pin, input_dir=self.inputs)
            self.assertFalse((work / 'host-tools').exists())
            self.assertFalse((work / 'port-inputs.json').exists())
        self.assertFalse((self.dir / 'outside').exists())

    def test_extraction_size_and_member_limits_fail_before_publication(self):
        for index, limits in enumerate(({'MAX_HOST_TOOL_BYTES': 1}, {'MAX_HOST_TOOL_MEMBERS': 0})):
            with patch.multiple('ci_integration', **limits), self.assertRaisesRegex(IntegrationError, 'limit'):
                prepare_port_inputs(self.dir / ('limit-%d' % index), 'linux-x64', self.zlib,
                                    self.tools, input_dir=self.inputs)
            self.assertFalse((self.dir / ('limit-%d' % index) / 'host-tools').exists())

    def test_pin_names_cannot_escape_or_overwrite_each_other(self):
        for name in ('../escape', '/absolute', 'a/b', 'a\\b', 'C:drive'):
            with self.subTest(name=name), self.assertRaisesRegex(IntegrationError, 'plain file name'):
                prepare_port_inputs(self.dir / 'no-work', 'linux-x64', dict(self.zlib, name=name), self.tools)
        with self.assertRaisesRegex(IntegrationError, 'distinct'):
            prepare_port_inputs(self.dir / 'same-name', 'linux-x64', self.zlib, dict(self.tools, name=self.zlib['name']))

    def test_oracle_requires_an_absolute_pinned_executable(self):
        path = self.inputs / 'test-only-python'
        path.write_bytes(b'fixture, never executed')
        path.chmod(0o755)
        verify_oracle(str(path), sha256_file(path))
        for name, pin in [(None, None), (str(path), None), (str(path), '0' * 64),
                          ('relative', sha256_file(path)), (str(self.inputs / 'missing'), '0' * 64)]:
            with self.subTest(name=name), self.assertRaises(IntegrationError):
                verify_oracle(name, pin)


class ResultGateTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.path = Path(self.tmp.name) / 'result.json'
        self.inputs = {'host_tools_sha256': 'a' * 64, 'source_sha256': 'b' * 64, 'contract_sha256': 'c' * 64}
        self.good = {'passed': True, 'allow_dirty_sdk': False,
                     'cases': [{'case': 'fixture only', 'passed': True}],
                     'suites': {name: {'passed': True, 'checks': 1, 'skipped': False} for name in PORT_SUITES},
                     'host_tools_sha256': 'a' * 64, 'archive_sha256': 'b' * 64, 'contract_sha256': 'c' * 64}

    def tearDown(self):
        self.tmp.cleanup()

    def judge(self, result, code=0):
        self.path.write_text(json.dumps(result))
        return judge_port_result(self.path, code, self.inputs)

    def test_complete_five_suite_report_is_required(self):
        self.assertEqual(self.judge(self.good), self.good)
        for removed in PORT_SUITES:
            result = copy.deepcopy(self.good)
            del result['suites'][removed]
            with self.subTest(removed=removed), self.assertRaisesRegex(IntegrationError, 'five required'):
                self.judge(result)
        result = copy.deepcopy(self.good)
        result['suites']['unexpected'] = {'passed': True, 'checks': 1}
        with self.assertRaisesRegex(IntegrationError, 'five required'):
            self.judge(result)

    def test_skip_zero_bool_count_or_failed_suite_cannot_pass(self):
        for field, value in [('passed', False), ('checks', 0), ('checks', -1), ('checks', True),
                             ('checks', None), ('skipped', True)]:
            result = copy.deepcopy(self.good)
            result['suites'][PORT_SUITES[0]][field] = value
            with self.subTest(field=field, value=value), self.assertRaisesRegex(IntegrationError, 'did not execute'):
                self.judge(result)

    def test_exit_failure_dirty_sdk_incomplete_cases_and_wrong_inputs_fail(self):
        with self.assertRaisesRegex(IntegrationError, 'exit 1'):
            self.judge(self.good, code=1)
        for field, value in [('passed', False), ('allow_dirty_sdk', True), ('cases', []),
                             ('cases', [{'passed': False}]), ('cases', [None]),
                             ('archive_sha256', '0' * 64), ('host_tools_sha256', '0' * 64),
                             ('contract_sha256', '0' * 64)]:
            with self.subTest(field=field), self.assertRaises(IntegrationError):
                self.judge(dict(self.good, **{field: value}))
        with self.assertRaises(FileNotFoundError):
            judge_port_result(self.path.parent / 'missing.json', 0, self.inputs)
        self.path.write_text('not JSON')
        with self.assertRaises(ValueError):
            judge_port_result(self.path, 0, self.inputs)

    def test_linux_adapter_rejects_exit_zero_skips_and_ambiguous_reports(self):
        name = PORT_SUITES[0]
        good = name + ': 35 checks passed\n'
        self.assertTrue(suite_outcome(name, 0, good, '')['passed'])
        for code, out, err in [(1, good, ''), (0, '', ''), (0, good, 'skipped prerequisite'),
                               (0, name + ': skipped\n', ''), (0, good + good, ''),
                               (0, name + ': 0 checks passed\n', ''), (0, 'other: 35 checks passed\n', '')]:
            with self.subTest(code=code, out=out, err=err):
                self.assertFalse(suite_outcome(name, code, out, err)['passed'])


if __name__ == '__main__':
    unittest.main()
