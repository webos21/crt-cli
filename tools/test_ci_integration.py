"""Integration pin gates: an empty or wrong pin must fail, never skip."""
import copy
import hashlib
import io
import json
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from ci_integration import (IntegrationError, PORT_SUITES, REDIRECT_CASE_COUNT, command_redirect, judge_redirect_result,
                            judge_port_result, load_pins, load_publisher, main,
                            host_key, port_adapter_command, prepare_windows_harness, windows_harness_pins,
                            prepare_port_inputs, sha256_file, spki_sha256, verify_download, verify_oracle,
                            verify_publisher_signature)
from replay_port_linux import suite_outcome

GOOD_ZLIB = {'url': 'https://example.invalid/zlib.tar.gz', 'name': 'zlib.tar.gz', 'sha256': 'a' * 64, 'size': 10}
GOOD_TOOLS = {'url': 'https://example.invalid/tools.tar.xz', 'name': 'tools.tar.xz', 'sha256': 'b' * 64, 'size': 20,
              'manifest_sha256': 'c' * 64, 'version': 'v0'}


def redirect_result(cli_sha256='a' * 64):
    return {'passed': True, 'cli_sha256': cli_sha256,
            'cases': [{'case': 'fixture-%d' % i, 'passed': True} for i in range(REDIRECT_CASE_COUNT)],
            'commands': [{'exit': 0, 'stdout': '{}', 'stderr': ''}]}


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
        for host, os_name, arch in (('linux-x64', 'linux', 'x86_64'), ('linux-arm64', 'linux', 'aarch64'),
                                    ('macos-arm64', 'macos', 'aarch64'), ('win-x64', 'windows', 'x86_64'),
                                    ('win-arm64', 'windows', 'aarch64')):
            with self.subTest(host=host):
                zlib, tools = load_pins(root, host)
                self.assertTrue(tools['url'].startswith('https://github.com/webos21/crt/releases/download/' + tools['version'] + '/'))
                self.assertIn(os_name + '-' + arch, tools['version'])
                self.assertEqual(tools['name'], 'crt-' + tools['version'] + '.tar.xz')


class NativeAdapterTest(unittest.TestCase):
    def test_native_host_detection_and_explicitly_unsupported_hosts(self):
        for system, machine, expected in (('linux', 'x86_64', 'linux-x64'), ('linux', 'aarch64', 'linux-arm64'),
                                           ('darwin', 'arm64', 'macos-arm64'), ('darwin', 'aarch64', 'macos-arm64'),
                                           ('win32', 'AMD64', 'win-x64'), ('win32', 'x86_64', 'win-x64'),
                                           ('win32', 'ARM64', 'win-arm64'), ('win32', 'aarch64', 'win-arm64')):
            self.assertEqual(host_key(system, machine), expected)
        for system, machine in (('darwin', 'x86_64'), ('win32', 'x86'), ('linux', 'riscv64')):
            with self.subTest(system=system, machine=machine), self.assertRaisesRegex(IntegrationError, 'unsupported'):
                host_key(system, machine)

    def test_linux_adapter_keeps_explicit_llvm_tools(self):
        adapter, tools = port_adapter_command('linux-arm64', '/declared/llvm/bin')
        self.assertEqual(adapter.name, 'replay_port_linux.py')
        self.assertEqual(tools, {flag: Path('/declared/llvm/bin').absolute() / name for flag, name in
                                 (('--cc', 'clang'), ('--cxx', 'clang++'), ('--linker', 'ld.lld'), ('--ar', 'llvm-ar'))})
        with self.assertRaisesRegex(IntegrationError, '--llvm-bin'):
            port_adapter_command('linux-x64', None)

    def test_macos_uses_only_declared_xcode_tools_and_its_adapter(self):
        paths = {flag: Path('/Xcode/usr/bin') / name for flag, name in
                 (('--cc', 'clang'), ('--cxx', 'clang++'), ('--linker', 'ld'), ('--ar', 'ar'))}
        with patch('ci_integration.macos_tool_paths', return_value=paths) as select:
            adapter, tools = port_adapter_command('macos-arm64', None)
        select.assert_called_once_with()
        self.assertEqual(adapter.name, 'replay_port_macos.py')
        self.assertEqual(tools, paths)
        from ci_bootstrap import CiError
        with patch('ci_integration.macos_tool_paths', side_effect=CiError('xcrun missing')):
            with self.assertRaisesRegex(IntegrationError, 'xcrun missing'):
                port_adapter_command('macos-arm64', None)
        with self.assertRaisesRegex(IntegrationError, '--llvm-bin'):
            port_adapter_command('win-x64', None)

    def test_windows_tools_and_public_sdk_import_libraries_are_explicit(self):
        for arch in ('x64', 'arm64'):
            sdk = 'C:/Windows SDK/um/' + arch
            with self.subTest(arch=arch), patch('ci_integration.windows_sdk_path', return_value=sdk) as select:
                adapter, tools = port_adapter_command('win-' + arch, 'C:/LLVM/bin')
                select.assert_called_once_with(arch)
                self.assertEqual(adapter.name, 'replay_port_windows_ci.py')
                self.assertEqual(tools['--cc'], Path('C:/LLVM/bin').absolute() / 'clang.exe')
                self.assertEqual(tools['--os-sdk'], sdk)

    def test_windows_redirect_stages_the_pinned_cli_and_runs_the_shared_native_cases(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory) / 'fresh'
            exe = Path(directory) / 'crt-cli.exe'; exe.write_bytes(b'fixture only, never executed')
            release = Path(directory) / 'release'
            def replay(argv, **kwargs):
                self.assertEqual(Path(argv[1]).name, 'replay_redirect_linux.py')
                self.assertEqual(argv[argv.index('--cli') + 1], str(exe))
                output = Path(argv[argv.index('--output') + 1]); output.mkdir()
                (output / 'redirect-replay.json').write_text(json.dumps(redirect_result(sha256_file(exe))))
                return subprocess.CompletedProcess(argv, 0, 'fixture stdout', 'fixture stderr')
            with patch('ci_integration.host_key', return_value='win-x64'), \
                    patch('ci_integration.published_cli', return_value=(exe, {})) as bootstrap, \
                    patch('ci_integration.sdk_release_dir', return_value=release), \
                    patch('ci_integration.subprocess.run', side_effect=replay):
                command_redirect(SimpleNamespace(work=str(work)), 'win-x64')
            bootstrap.assert_called_once_with('win-x64', work.resolve())
            self.assertEqual((work / 'redirect.stdout').read_text(), 'fixture stdout')
            self.assertEqual((work / 'redirect.stderr').read_text(), 'fixture stderr')

    def test_redirect_rejects_cross_host_before_download_or_work_creation(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory) / 'cross-host'
            with patch('ci_integration.host_key', return_value='linux-x64'), patch('ci_integration.download') as get:
                with self.assertRaisesRegex(IntegrationError, 'native execution host'):
                    command_redirect(SimpleNamespace(work=str(work)), 'win-x64')
            get.assert_not_called()
            self.assertFalse(work.exists())

    def test_redirect_timeout_retains_partial_logs_without_success(self):
        from types import SimpleNamespace
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory) / 'timeout'
            error = subprocess.TimeoutExpired(['fixture'], 1500, output=b'partial stdout', stderr=b'partial stderr')
            with patch('ci_integration.host_key', return_value='win-x64'), \
                    patch('ci_integration.published_cli', return_value=(Path(directory) / 'crt-cli.exe', {})), \
                    patch('ci_integration.sdk_release_dir', return_value=Path(directory)), \
                    patch('ci_integration.subprocess.run', side_effect=error):
                with self.assertRaises(subprocess.TimeoutExpired):
                    command_redirect(SimpleNamespace(work=str(work)), 'win-x64')
            self.assertEqual((work / 'redirect.stdout').read_text(), 'partial stdout')
            self.assertEqual((work / 'redirect.stderr').read_text(), 'partial stderr')
            self.assertFalse((work / 'redirect-replay/redirect-replay.json').exists())

    def test_workflow_selects_all_five_hosts_and_keeps_native_prerequisites_separate(self):
        root = Path(__file__).resolve().parent.parent
        text = (root / '.github/workflows/integration.yml').read_text()
        for host in ('linux-x64', 'linux-arm64', 'macos-arm64', 'win-x64', 'win-arm64'):
            self.assertEqual(text.count('host: ' + host + '\n'), 3)
            load_pins(root / '.github/ci/integration-pins.json', host)
        self.assertIn("if: runner.os == 'Linux'", text)
        self.assertIn("if: runner.os == 'macOS'", text)
        self.assertIn('runner: macos-15', text)
        self.assertIn('sandbox-exec', text)
        self.assertIn('hashlib.file_digest', text)
        self.assertNotIn('sha256sum', text)
        self.assertIn('ports/replay/commands.json', text)
        self.assertEqual(text.count('runner: windows-11-arm\n'), 3)
        self.assertEqual(text.count("architecture: ${{ matrix.host == 'win-arm64' && 'arm64' || '' }}"), 3)
        self.assertIn('LLVM-22.1.8-woa64.exe', text)
        self.assertIn("os.environ['RUNNER_TEMP']", text)
        self.assertIn('redirect/redirect.stderr', text)
        self.assertIn('options: [all, redirect]', text)
        self.assertEqual(text.count("if: github.event_name != 'workflow_dispatch' || inputs.scope != 'redirect'"), 2)
        self.assertIn('name: ports-${{ matrix.host }}-${{ github.sha }}', text)
        self.assertIn('SetAccessRuleProtection($true, $false)', text)
        self.assertIn('path: private-inputs', text)
        self.assertIn('real symlinks required', text)
        self.assertNotIn('AllowDevelopmentWithoutDevLicense', text)


class RedirectResultGateTest(unittest.TestCase):
    def test_complete_native_result_only(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'result.json'
            good = redirect_result()
            path.write_text(json.dumps(good))
            self.assertEqual(judge_redirect_result(path, 0, 'a' * 64), good)
            variants = [dict(good, passed=False), dict(good, cli_sha256='b' * 64), dict(good, cases=[]),
                        dict(good, cases=good['cases'][:-1]), dict(good, cases=good['cases'] + [good['cases'][0]]),
                        dict(good, cases=[good['cases'][0]] * REDIRECT_CASE_COUNT), dict(good, commands=[]),
                        dict(good, commands=[{'exit': 0xC0000005}]), dict(good, commands=[{'exit': True}])]
            for replacement in ({'case': 'bad', 'passed': False}, {'case': 'bad', 'passed': True, 'skipped': True},
                                {'case': '', 'passed': True}, {'case': 'bad', 'passed': 1}):
                variants.append(dict(good, cases=[replacement, *good['cases'][1:]]))
            for index, result in enumerate(variants):
                path.write_text(json.dumps(result))
                with self.subTest(index=index), self.assertRaises(IntegrationError):
                    judge_redirect_result(path, 0, 'a' * 64)
            path.write_text(json.dumps(good))
            with self.assertRaisesRegex(IntegrationError, 'exit 1'):
                judge_redirect_result(path, 1, 'a' * 64)
            with self.assertRaises(FileNotFoundError):
                judge_redirect_result(path.parent / 'absent.json', 0, 'a' * 64)


class WindowsHarnessTest(unittest.TestCase):
    def test_helper_inputs_are_pinned_at_the_immutable_revision(self):
        root = Path(__file__).resolve().parent.parent
        path = root / '.github/ci/integration-pins.json'
        files = windows_harness_pins(path)
        self.assertEqual(len(files), 2)
        with tempfile.TemporaryDirectory() as directory:
            for label, change in [('no revision', {'revision': None}), ('mutable revision', {'revision': 'main'}),
                                   ('no files', {'files': []}), ('duplicate file', {'files': [files[0], files[0]]}),
                                   ('wrong url', {'files': [dict(files[0], url='https://example.invalid/tool.py'), files[1]]}),
                                   ('oversized', {'files': [dict(files[0], size=65537), files[1]]})]:
                document = json.loads(path.read_text())
                document['windows_replay_harness'].update(change)
                altered = Path(directory) / 'pins.json'; altered.write_text(json.dumps(document))
                with self.subTest(label=label), self.assertRaises(IntegrationError):
                    windows_harness_pins(altered)

    def test_changed_download_never_publishes_a_ready_harness_receipt(self):
        root = Path(__file__).resolve().parent.parent
        with tempfile.TemporaryDirectory() as directory, patch('ci_integration.download',
                side_effect=lambda url, path: Path(path).write_bytes(b'changed')):
            work = Path(directory)
            with self.assertRaisesRegex(IntegrationError, 'size'):
                prepare_windows_harness(root / '.github/ci/integration-pins.json', work)
            self.assertFalse((work / 'windows-harness/pins.json').exists())


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

    def test_native_execution_rejects_macos_cross_host_before_download(self):
        pins = write(self.dir, zlib=self.zlib, tools=self.tools)
        data = json.loads(pins.read_text())
        data['hosts']['macos-arm64'] = data['hosts']['linux-x64']
        pins.write_text(json.dumps(data))
        work = self.dir / 'cross-macos'
        with patch('ci_integration.host_key', return_value='linux-x64'), patch('ci_integration.download') as get, patch('sys.argv',
                ['ci_integration.py', 'ports', '--host', 'macos-arm64', '--pins', str(pins), '--work', str(work)]):
            self.assertEqual(main(), 1)
        get.assert_not_called()
        self.assertFalse(work.exists())

    def signed(self, tampered=False):
        """Throw-away key; returns (publisher, tools pin with a signature pin) for the archive currently in self.inputs."""
        openssl = shutil.which('openssl')
        if not openssl:
            self.skipTest('openssl is a test-host tool')
        key, pub = self.dir / 'k.key', self.dir / 'k.pub'
        subprocess.run([openssl, 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', str(key)], check=True, capture_output=True)
        subprocess.run([openssl, 'pkey', '-in', str(key), '-pubout', '-out', str(pub)], check=True, capture_output=True)
        signature = self.inputs / 'tools.tar.xz.sig'
        # Some test-host OpenSSL builds cannot open Unicode paths. Sign the same archive bytes over stdio;
        # Python still writes/verifies the signature and stages the archive in the original Unicode directory.
        signed = subprocess.run([openssl, 'dgst', '-sha256', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32', '-sigopt', 'rsa_mgf1_md:sha256',
                                 '-sign', str(key)], input=(self.inputs / 'tools.tar.xz').read_bytes(), check=True, capture_output=True)
        signature.write_bytes(signed.stdout)
        if tampered:
            data = bytearray(signature.read_bytes()); data[0] ^= 1; signature.write_bytes(bytes(data))
        pin = {'url': 'https://example.invalid/tools.tar.xz.sig', 'name': 'tools.tar.xz.sig', 'size': signature.stat().st_size,
               'sha256': sha256_file(signature)}
        return {'key_file': pub, 'key_spki_sha256': spki_sha256(pub)}, dict(self.tools, signature=pin)

    def test_online_interface_requires_a_published_preview_bundle(self):
        def download(url, destination):
            Path(destination).write_bytes((self.inputs / Path(destination).name).read_bytes())
        publisher, tools = self.signed()
        with patch('ci_integration.download', side_effect=download):
            with self.assertRaisesRegex(IntegrationError, 'promotion preview'):
                prepare_port_inputs(self.dir / 'online-local-only', 'linux-x64', self.zlib, tools, publisher=publisher)
        self.assertFalse((self.dir / 'online-local-only/host-tools').exists())

    def test_online_interface_verifies_downloaded_bytes_and_the_publisher_signature(self):
        def download(url, destination):
            Path(destination).write_bytes((self.inputs / Path(destination).name).read_bytes())
        self.tools = self.bundle(manifest=dict(self.manifest, promotion='preview'))
        publisher, tools = self.signed()
        with patch('ci_integration.download', side_effect=download) as get:
            prepared = prepare_port_inputs(self.dir / 'online', 'linux-x64', self.zlib, tools, publisher=publisher)
        self.assertEqual(get.call_count, 3)
        self.assertEqual((prepared['publisher_authentication'], prepared['publisher_key_spki_sha256']), ('signature_verified', publisher['key_spki_sha256']))
        with patch('ci_integration.download', side_effect=lambda url, path: Path(path).write_bytes(b'bad')):
            with self.assertRaisesRegex(IntegrationError, 'size'):
                prepare_port_inputs(self.dir / 'bad-online', 'linux-x64', self.zlib, tools, publisher=publisher)

    def test_macos_preview_requires_its_own_manifest_host_and_signature(self):
        self.tools = self.bundle(manifest=dict(self.manifest, promotion='preview', host={'os': 'macos', 'arch': 'aarch64'}))
        publisher, tools = self.signed()
        with patch('ci_integration.download', side_effect=lambda url, path: shutil.copyfile(self.inputs / Path(path).name, path)):
            result = prepare_port_inputs(self.dir / 'macos', 'macos-arm64', self.zlib, tools, publisher=publisher)
            with self.assertRaisesRegex(IntegrationError, 'host/version'):
                prepare_port_inputs(self.dir / 'wrong-linux', 'linux-arm64', self.zlib, tools, publisher=publisher)
        self.assertEqual(result['publisher_authentication'], 'signature_verified')
        self.assertEqual(result['native_validation'], 'not_run')

    def test_windows_preview_requires_its_own_manifest_host_and_signature(self):
        self.tools = self.bundle(manifest=dict(self.manifest, promotion='preview', host={'os': 'windows', 'arch': 'x86_64'}))
        publisher, tools = self.signed()
        with patch('ci_integration.download', side_effect=lambda url, path: shutil.copyfile(self.inputs / Path(path).name, path)):
            result = prepare_port_inputs(self.dir / 'windows', 'win-x64', self.zlib, tools, publisher=publisher)
            with self.assertRaisesRegex(IntegrationError, 'host/version'):
                prepare_port_inputs(self.dir / 'wrong-linux', 'linux-x64', self.zlib, tools, publisher=publisher)
        self.assertEqual(result['publisher_authentication'], 'signature_verified')
        self.assertEqual(result['native_validation'], 'not_run')

    def test_online_interface_refuses_unsigned_or_wrongly_signed_bundles_before_extraction(self):
        def download(url, destination):
            Path(destination).write_bytes((self.inputs / Path(destination).name).read_bytes())
        self.tools = self.bundle(manifest=dict(self.manifest, promotion='preview'))
        publisher, tools = self.signed(tampered=True)
        with patch('ci_integration.download', side_effect=download):
            with self.assertRaisesRegex(IntegrationError, 'signature verification failed'):
                prepare_port_inputs(self.dir / 'tampered', 'linux-x64', self.zlib, tools, publisher=publisher)
            with self.assertRaisesRegex(IntegrationError, 'signature: pin is not an object'):
                prepare_port_inputs(self.dir / 'unsigned', 'linux-x64', self.zlib, self.tools, publisher=publisher)
        self.assertFalse((self.dir / 'tampered/host-tools').exists() or (self.dir / 'unsigned/host-tools').exists())


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

    def judge(self, result, code=0, prepared=None):
        self.path.write_text(json.dumps(result))
        return judge_port_result(self.path, code, self.inputs if prepared is None else prepared)

    def test_macos_requires_the_same_five_suite_and_input_gates(self):
        prepared = dict(self.inputs, host='macos-arm64')
        result = dict(self.good, suites=list(PORT_SUITES), suite_results=copy.deepcopy(self.good['suites']))
        self.assertEqual(self.judge(result, prepared=prepared), result)
        for requested in (None, {}, list(PORT_SUITES[:-1]), [*PORT_SUITES, PORT_SUITES[0]], [1, *PORT_SUITES[1:]]):
            with self.subTest(requested=requested), self.assertRaisesRegex(IntegrationError, 'five required'):
                self.judge(dict(result, suites=requested), prepared=prepared)
        for outcome in ({'passed': True, 'checks': 0}, {'passed': True, 'checks': True},
                        {'passed': True, 'checks': 2, 'skipped': True}, {'passed': False, 'checks': 2}):
            bad = copy.deepcopy(result)
            bad['suite_results'][PORT_SUITES[0]] = outcome
            with self.subTest(outcome=outcome), self.assertRaisesRegex(IntegrationError, 'did not execute'):
                self.judge(bad, prepared=prepared)

    def test_windows_requires_native_boundary_symlink_and_inventory_cases(self):
        required = ['original-inputs-hidden', 'real-symlink-prerequisite', 'bare-original-and-build-inputs-hidden',
                    'PE x64 and only declared OS imports', 'original and copied SDK/tool inventories unchanged',
                    'source inputs unchanged']
        result = dict(self.good, host='win-x64', cases=[{'case': name, 'passed': True} for name in required])
        prepared = dict(self.inputs, host='win-x64')
        self.assertEqual(self.judge(result, prepared=prepared), result)
        for missing in required:
            bad = dict(result, cases=[c for c in result['cases'] if c['case'] != missing])
            with self.subTest(missing=missing), self.assertRaisesRegex(IntegrationError, 'Windows replay lacks'):
                self.judge(bad, prepared=prepared)
        for key, value in (('allow_dirty_sdk', True), ('allow_dirty_sdk', None), ('contract_sha256', '0' * 64)):
            with self.subTest(key=key, value=value), self.assertRaises(IntegrationError):
                self.judge(dict(result, **{key: value}), prepared=prepared)

        arm64 = dict(prepared, host='win-arm64')
        with self.assertRaisesRegex(IntegrationError, 'host differs'):
            self.judge(result, prepared=arm64)
        arm64_result = dict(result, host='win-arm64')
        with self.assertRaisesRegex(IntegrationError, 'Windows replay lacks'):
            self.judge(arm64_result, prepared=arm64)
        arm64_result['cases'] = [dict(case, case=case['case'].replace('PE x64', 'PE arm64')) for case in result['cases']]
        self.assertEqual(self.judge(arm64_result, prepared=arm64), arm64_result)

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


class PublisherSignatureTest(unittest.TestCase):
    """Throw-away RSA keys; the committed key is only checked against its pinned fingerprint."""
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.openssl = shutil.which('openssl')
        if not self.openssl:
            self.skipTest('openssl is a test-host tool')
        for name in ('a', 'b'):
            subprocess.run([self.openssl, 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:2048', '-out', str(self.dir / (name + '.key'))],
                           check=True, capture_output=True)
            subprocess.run([self.openssl, 'pkey', '-in', str(self.dir / (name + '.key')), '-pubout', '-out', str(self.dir / (name + '.pub'))],
                           check=True, capture_output=True)
        self.archive = self.dir / 'bundle.tar.xz'
        self.archive.write_bytes(b'published bytes')

    def tearDown(self):
        self.tmp.cleanup()

    def sign(self, key, archive, out):
        subprocess.run([self.openssl, 'dgst', '-sha256', '-sigopt', 'rsa_padding_mode:pss', '-sigopt', 'rsa_pss_saltlen:32',
                        '-sigopt', 'rsa_mgf1_md:sha256', '-sign', str(self.dir / (key + '.key')), '-out', str(out), str(archive)],
                       check=True, capture_output=True)
        return out

    def publisher(self, key):
        return {'key_file': self.dir / (key + '.pub'), 'key_spki_sha256': spki_sha256(self.dir / (key + '.pub'))}

    def test_good_signature_verifies_and_every_other_case_fails(self):
        good = self.sign('a', self.archive, self.dir / 'good.sig')
        verify_publisher_signature('t', self.archive, good, self.publisher('a'))
        with self.assertRaisesRegex(IntegrationError, 'verification failed'):
            verify_publisher_signature('t', self.archive, good, self.publisher('b'))
        other = self.dir / 'other.tar.xz'
        other.write_bytes(b'different bytes')
        with self.assertRaisesRegex(IntegrationError, 'verification failed'):
            verify_publisher_signature('t', other, good, self.publisher('a'))
        garbage = self.dir / 'garbage.sig'
        garbage.write_bytes(b'x' * 256)
        with self.assertRaisesRegex(IntegrationError, 'verification failed'):
            verify_publisher_signature('t', self.archive, garbage, self.publisher('a'))
        huge = self.dir / 'huge.sig'
        huge.write_bytes(b'x' * 2048)
        with self.assertRaisesRegex(IntegrationError, 'larger than'):
            verify_publisher_signature('t', self.archive, huge, self.publisher('a'))

    def test_published_inputs_require_the_pinned_key_and_a_signature_pin(self):
        zlib_file = self.dir / 'zlib.tar.gz'
        zlib_file.write_bytes(b'z')
        with self.assertRaisesRegex(IntegrationError, 'requires the pinned publisher key'), patch('ci_integration.fetch_pinned'):
            prepare_port_inputs(self.dir / 'w1', 'linux-x64', dict(GOOD_ZLIB), dict(GOOD_TOOLS))
        with self.assertRaisesRegex(IntegrationError, 'signature: pin is not an object'), patch('ci_integration.fetch_pinned'):
            prepare_port_inputs(self.dir / 'w2', 'linux-x64', dict(GOOD_ZLIB), dict(GOOD_TOOLS), publisher=self.publisher('a'))

    def test_committed_publisher_pin_matches_its_key_file_and_every_host_pins_a_signature(self):
        root = Path(__file__).resolve().parent.parent / '.github/ci/integration-pins.json'
        publisher = load_publisher(root)
        self.assertEqual(publisher['key_spki_sha256'], '59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2')
        for host in ('linux-x64', 'linux-arm64', 'macos-arm64'):
            with self.subTest(host=host):
                _, tools = load_pins(root, host)
                self.assertEqual(tools['signature']['url'], tools['url'] + '.sig')
                self.assertEqual(tools['signature']['name'], tools['name'] + '.sig')

    def test_publisher_pin_edits_fail(self):
        base = json.loads((Path(__file__).resolve().parent.parent / '.github/ci/integration-pins.json').read_text())
        for label, change, message in [('wrong fingerprint', {'key_spki_sha256': '0' * 64}, 'does not match'),
                                       ('bad fingerprint', {'key_spki_sha256': 'AB'}, 'lowercase'),
                                       ('absolute key', {'key_file': '/etc/passwd'}, 'repository-relative'),
                                       ('escaping key', {'key_file': '../x.pub'}, 'repository-relative'),
                                       ('unknown field', {'extra': 1}, 'unknown fields')]:
            pins = copy.deepcopy(base)
            pins['publisher'].update(change)
            path = self.dir / 'pins.json'
            path.write_text(json.dumps(pins))
            with self.subTest(label=label), self.assertRaisesRegex(IntegrationError, message):
                load_publisher(path)
        pins = copy.deepcopy(base)
        del pins['publisher']
        path = self.dir / 'pins.json'
        path.write_text(json.dumps(pins))
        with self.assertRaisesRegex(IntegrationError, 'missing'):
            load_publisher(path)
