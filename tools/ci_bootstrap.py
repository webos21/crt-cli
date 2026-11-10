#!/usr/bin/env python3
"""GitHub CI driver for crt-cli; a build-host orchestrator, never a CLI runtime dependency.

  pinned published CLI (SHA-256) -> pinned public Dev SDK acquired by that CLI (github-release redirects)
  -> stage 1: the bootstrap CLI builds the checked-out sources
  -> the deterministic native suites run on stage 1 (bootstrap test results never count for the sources)
  -> stage 2: stage 1 rebuilds a clean export (cold, then warm) -> import audit -> SDK-free run

Every product build, test and run below is a native crt-cli argv; this script only downloads, hashes, copies and audits.
Inputs come from .github/ci/pins.json. Byte equality of stage 1 and stage 2 is reported, not required.
"""
import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import ssl
import subprocess
import sys
import tarfile
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = '.exe' if os.name == 'nt' else ''


class CiError(Exception):
    pass


def log(message):
    print(message, file=sys.stderr, flush=True)


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def host_key():
    machine = platform.machine().lower()
    if sys.platform.startswith('linux') and machine in ('x86_64', 'amd64'):
        return 'linux-x64'
    if sys.platform == 'darwin' and machine in ('arm64', 'aarch64'):
        return 'macos-arm64'
    if sys.platform == 'win32' and machine in ('amd64', 'x86_64'):
        return 'win-x64'
    raise CiError('unsupported CI host: %s/%s' % (sys.platform, machine))


class Ci:
    def __init__(self, args):
        self.args = args
        self.host = args.host or host_key()
        pins = json.loads(Path(args.pins).read_text(encoding='utf-8'))
        if pins.get('schema') != 'crt-cli.ci-pins' or pins.get('format') != 0:
            raise CiError('unsupported pin file')
        self.pins, self.host_pins = pins, pins['hosts'][self.host]
        self.work = Path(args.work).resolve()
        if self.work.exists() and any(self.work.iterdir()):
            raise CiError('work directory must be new or empty: %s' % self.work)
        for name in ('logs', 'home', 'tmp', 'artifact'):
            (self.work / name).mkdir(parents=True, exist_ok=True)
        # A private home keeps the SDK store (home/.crt/sdks) inside this run.
        self.env = dict(os.environ, HOME=str(self.work / 'home'), USERPROFILE=str(self.work / 'home'))
        # The native suite takes its scratch root from TMPDIR, then TEMP, then /tmp, and the CLI forwards only TMPDIR to
        # children; Windows has no /tmp, so every host sets it (forward slashes keep the Windows spelling the suite uses).
        self.env['TMPDIR'] = (self.work / 'tmp').as_posix()
        self.summary = {'host': self.host, 'steps': [], 'source_commit': self.commit()}

    def commit(self):
        done = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, capture_output=True, text=True)
        return done.stdout.strip() if done.returncode == 0 else 'unknown'

    def run(self, label, argv, env=None, timeout=3600, expect=0):
        argv = [str(a) for a in argv]
        log('[%s] %s' % (label, ' '.join(argv)))
        began = time.monotonic()
        try:
            done = subprocess.run(argv, env=env or self.env, capture_output=True, timeout=timeout)
            code, out, err = done.returncode, done.stdout, done.stderr
        except subprocess.TimeoutExpired as expired:
            code, out, err = 124, expired.stdout or b'', expired.stderr or b''
        (self.work / 'logs' / (label + '.stdout')).write_bytes(out)
        (self.work / 'logs' / (label + '.stderr')).write_bytes(err)
        self.summary['steps'].append({'label': label, 'exit': code, 'seconds': round(time.monotonic() - began, 1)})
        out, err = out.decode('utf-8', 'replace'), err.decode('utf-8', 'replace')
        if code != expect:
            log('\n'.join(err.splitlines()[-40:]))
            log('stdout tail: ' + out[-1500:])
            raise CiError('%s exited %d (expected %d); full logs: %s' % (label, code, expect, self.work / 'logs'))
        return out, err

    def cli(self, label, exe, command, project=None, extra=(), timeout=3600):
        argv = [exe, command] + (['--project', project] if project else []) + list(extra) + ['--json']
        out, err = self.run(label, argv, timeout=timeout)
        reply = json.loads(out)
        if not reply.get('ok'):
            raise CiError('%s reported failure: %s' % (label, reply.get('data')))
        return reply['data'], err

    # --- trust inputs -------------------------------------------------------------------------------------------
    def bootstrap(self):
        pin = self.host_pins['bootstrap']
        url = 'https://github.com/webos21/crt-cli/releases/download/%s/%s' % (pin['release'], pin['asset'])
        archive = self.work / pin['asset']
        for attempt in range(1, 4):  # bounded; the SHA-256 pin below, not TLS, authenticates the bytes
            try:
                with urllib.request.urlopen(url, timeout=120, context=ssl.create_default_context()) as src, open(archive, 'wb') as dst:
                    shutil.copyfileobj(src, dst)
                break
            except OSError as error:
                log('download attempt %d failed: %s' % (attempt, error))
                if attempt == 3:
                    raise CiError('cannot download the pinned bootstrap CLI')
                time.sleep(5 * attempt)
        if archive.stat().st_size != pin['size'] or sha256_file(archive) != pin['sha256']:
            raise CiError('bootstrap CLI archive does not match its pin')
        target = self.work / 'bootstrap'
        target.mkdir()
        if not hasattr(tarfile, 'data_filter'):
            raise CiError('this Python lacks tarfile extraction filters')
        with tarfile.open(archive) as bundle:
            bundle.extractall(target, filter='data')
        found = [p for p in target.rglob('crt-cli' + EXE) if p.is_file()]
        if len(found) != 1:
            raise CiError('expected exactly one crt-cli executable in the bootstrap archive, found %d' % len(found))
        exe = found[0]
        exe.chmod(exe.stat().st_mode | 0o111)
        out, _ = self.run('bootstrap-version', [exe, '--version'])
        if pin['version'] not in out:
            raise CiError('bootstrap CLI reports an unexpected version: %s' % out.strip())
        self.summary['bootstrap'] = {'release': pin['release'], 'archive_sha256': pin['sha256'], 'executable_sha256': sha256_file(exe)}
        return exe

    def roots(self):
        """The caller-chosen PEM bundle (the CLI's explicit trust input); it only protects transport, the pins authenticate."""
        pem = self.work / 'roots.pem'
        if sys.platform.startswith('linux'):
            candidates = ['/etc/ssl/certs/ca-certificates.crt', '/etc/pki/tls/certs/ca-bundle.crt']
            found = [c for c in candidates if Path(c).is_file()]
            if not found:
                raise CiError('no system CA bundle found')
            shutil.copyfile(found[0], pem)
        elif sys.platform == 'darwin':
            done = subprocess.run(['security', 'find-certificate', '-a', '-p', '/System/Library/Keychains/SystemRootCertificates.keychain'],
                                  capture_output=True, check=True)
            pem.write_bytes(done.stdout)
        else:
            blocks = [ssl.DER_cert_to_PEM_cert(der) for der, encoding, _ in ssl.enum_certificates('ROOT') if encoding == 'x509_asn']
            pem.write_text(''.join(blocks), encoding='ascii', newline='\n')
        self.summary['ca_sha256'] = sha256_file(pem)
        return pem

    def sdk_source(self, pem):
        sdk, base = self.host_pins['sdk'], self.host_pins['sdk']['release_base']
        source = {
            'schema': 'crt-cli.sdk-source', 'format': 0, 'ca_file': pem.as_posix(), 'ca_sha256': sha256_file(pem),
            'manifest': {'url': base + sdk['manifest']['name'], 'sha256': sdk['manifest']['sha256'], 'size': sdk['manifest']['size']},
            'archive': {'name': sdk['archive'], 'url': base + sdk['archive']},
            'contract_url': base + sdk['contract'],
            'sdk': {'version': sdk['version'], 'content_sha256': sdk['content_sha256'], 'contract_sha256': sdk['contract_sha256']},
            'redirect_policy': 'github-release', 'timeout_seconds': 300}
        path = self.work / 'sdk-source.json'
        path.write_text(json.dumps(source, indent=2) + '\n', encoding='utf-8')
        return path

    # --- host tools ---------------------------------------------------------------------------------------------
    def tools(self):
        if self.host == 'macos-arm64':
            names = {'--cc': 'clang', '--cxx': 'clang++', '--linker': 'ld', '--ar': 'ar'}
            tools = {}
            for flag, name in names.items():
                done = subprocess.run(['xcrun', '-f', name], capture_output=True, text=True)
                if done.returncode != 0:
                    raise CiError('xcrun cannot find %s' % name)
                tools[flag] = done.stdout.strip()
        else:
            if not self.args.llvm_bin:
                raise CiError('--llvm-bin is required on this host')
            base = Path(self.args.llvm_bin)
            tools = {'--cc': base / ('clang' + EXE), '--cxx': base / ('clang++' + EXE), '--linker': base / ('ld.lld' + EXE), '--ar': base / ('llvm-ar' + EXE)}
        flags = []
        self.tool_paths = {}
        for flag, path in tools.items():
            if not Path(path).is_file():
                raise CiError('missing host tool: %s' % path)
            flags += [flag, str(path)]
            self.tool_paths[flag] = str(path)
        if self.host == 'win-x64':
            flags += ['--os-sdk', self.windows_sdk()]
        return flags

    def windows_sdk(self):
        if self.args.os_sdk:
            return self.args.os_sdk
        libs = sorted(Path('C:/Program Files (x86)/Windows Kits/10/Lib').glob('*/um/x64/kernel32.lib'))
        if not libs:
            raise CiError('no Windows SDK um/x64 import libraries found; pass --os-sdk')
        return libs[-1].parent.as_posix()

    # --- stages -------------------------------------------------------------------------------------------------
    def export(self, name):
        """Tracked and not-ignored files only, so no local build or binding state reaches a stage."""
        listing = subprocess.run(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT, capture_output=True, check=True)
        destination = self.work / name / 'src'
        if destination.exists():  # same absolute path as an earlier stage: debug info and __FILE__ embed it
            shutil.rmtree(destination, onerror=lambda func, path, _: (os.chmod(path, 0o700), func(path)))
        for entry in filter(None, listing.stdout.split(b'\0')):
            source = ROOT / entry.decode('utf-8')
            if source.is_file():
                target = destination / entry.decode('utf-8')
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
        return destination

    def build_stage(self, label, builder, tools, explicit_sdk=None, directory=None):
        """Cold build of a clean export. With explicit_sdk=(root, contract) the project is bound by `configure` first, the
        binding the native suite reads; otherwise the CLI's first build selects the installed SDK as a user's would."""
        project = self.export(directory or label)
        if explicit_sdk:
            self.cli(label + '-configure', builder, 'configure', project,
                     ['--sdk-root', str(explicit_sdk[0]), '--sdk-contract', str(explicit_sdk[1]),
                      '--contract-sha256', self.host_pins['sdk']['contract_sha256']] + tools)
            data, _ = self.cli(label + '-build', builder, 'build', project, timeout=3600)
        else:
            data, _ = self.cli(label + '-build', builder, 'build', project, ['--sdk', self.host_pins['sdk']['content_sha256']] + tools, timeout=3600)
        cache = data['cache']
        if cache['executed'] < 1 or cache['reused'] != 0:
            raise CiError('%s must be a cold build, got %s' % (label, cache))
        exe = project / 'out' / 'native' / ('crt-cli' + EXE)
        if not exe.is_file() or sha256_file(exe) != data['executable_sha256']:
            raise CiError('%s executable is missing or differs from the build receipt' % label)
        self.summary[label] = {'cache': cache, 'sha256': data['executable_sha256']}
        return project, exe

    def check_test_classification(self):
        declared = {t['name'] for t in json.loads((ROOT / 'crt.json').read_text(encoding='utf-8'))['tests']}
        known = set(self.pins['pr_tests']) | set(self.pins['input_gated_tests'])
        both = set(self.pins['pr_tests']) & set(self.pins['input_gated_tests'])
        if both:
            raise CiError('pins.json classifies tests as both PR-set and input-gated: %s' % sorted(both))
        if declared != known:
            raise CiError('crt.json tests and pins.json classification differ: only in crt.json %s, only in pins %s'
                          % (sorted(declared - known), sorted(known - declared)))

    def record_versions(self):
        """Evidence for failures that depend on the host (compiler, linker, C library)."""
        found = {}
        probes = {'clang': [self.tool_flag('--cc'), '--version'], 'linker': [self.tool_flag('--linker'), '--version']}
        if self.host == 'linux-x64':
            probes['glibc'] = ['ldd', '--version']
        for name, argv in probes.items():
            try:
                done = subprocess.run([str(a) for a in argv], capture_output=True, text=True, timeout=30)
                found[name] = (done.stdout or done.stderr).splitlines()[0].strip()
            except (OSError, subprocess.SubprocessError, IndexError):
                found[name] = 'unavailable'
        self.summary['host_versions'] = found

    def tool_flag(self, flag):
        return self.tool_paths[flag]

    def run_tests(self, exe, project):
        results, failures = {}, {}
        for name in self.args.tests or self.pins['pr_tests']:
            try:
                data, err = self.cli('test-' + name, exe, 'test', project, ['--target', name], timeout=3700)
                outcome = data['outcomes']
                if len(outcome) != 1 or outcome[0]['exit_code'] != 0 or outcome[0]['timed_out']:
                    raise CiError('%s failed: %s' % (name, outcome))
                if 'skipped' in err:
                    raise CiError('%s skipped work; a skipped suite is not a pass' % name)
                counted = re.findall(r'(\d+) checks passed', err)
                if not counted:
                    raise CiError('%s did not report a passed-check count' % name)
                results[name] = int(counted[-1])
            except CiError as error:
                # Later suites and stages still run, so one CI pass reports everything that is broken.
                failures[name] = str(error)
        self.summary['tests'] = results
        if failures:
            self.summary['test_failures'] = failures
        return failures

    def audit_imports(self, exe):
        allowed = {item.lower() for item in self.host_pins['allowed_imports']}
        if self.host == 'linux-x64':
            dynamic, _ = self.run('audit-dynamic', ['readelf', '-d', exe])
            if re.search(r'\((NEEDED|RPATH|RUNPATH)\)', dynamic):
                raise CiError('unexpected DT_NEEDED/RPATH/RUNPATH:\n' + dynamic)
            program, _ = self.run('audit-interpreter', ['readelf', '-l', exe])
            found = re.findall(r'interpreter: ([^\]]+)\]', program)
            return {'interpreter': found, 'imports': []}
        if self.host == 'macos-arm64':
            listing, _ = self.run('audit-otool', ['otool', '-L', exe])
            imports = {line.split(' (')[0].strip().lower() for line in listing.splitlines()[1:] if line.strip()}
            commands, _ = self.run('audit-otool-commands', ['otool', '-l', exe])
            if 'LC_RPATH' in commands:
                raise CiError('unexpected LC_RPATH')
        else:
            reader = Path(self.args.llvm_bin) / ('llvm-readobj' + EXE)
            listing, _ = self.run('audit-imports', [reader, '--coff-imports', exe])
            imports = {m.lower() for m in re.findall(r'Name:\s*(\S+\.dll)', listing, re.I)}
        if not imports <= allowed:
            raise CiError('unexpected imports: %s' % sorted(imports - allowed))
        return {'imports': sorted(imports)}

    def bare_run(self, label, exe):
        """Run the executable alone: no SDK store, no PATH, a fresh empty home."""
        folder = self.work / ('bare-' + label)
        (folder / 'home').mkdir(parents=True)
        bare = folder / ('crt-cli' + EXE)
        shutil.copy2(exe, bare)
        env = {'PATH': str(folder / 'nowhere'), 'HOME': str(folder / 'home'), 'USERPROFILE': str(folder / 'home'), 'LC_ALL': 'C'}
        for key in ('SystemRoot', 'TEMP', 'TMP'):
            if key in os.environ:
                env[key] = os.environ[key]
        out, _ = self.run(label + '-bare-version', [bare, '--version'], env=env)
        self.run(label + '-bare-help', [bare, '--help', '--json'], env=env)
        reply = json.loads(self.run(label + '-bare-doctor', [bare, 'doctor', '--bootstrap', '--json'], env=env)[0])
        checks = reply['data']['checks']
        if not reply['ok'] or 'fail' in checks.values():
            raise CiError('bare doctor failed: %s' % checks)
        return out.strip(), checks

    def execute(self):
        self.check_test_classification()
        boot = self.bootstrap()
        tools = self.tools()
        sdk_source = self.sdk_source(self.roots())
        data, _ = self.cli('sdk-acquire', boot, 'sdk', extra=['acquire', '--source', str(sdk_source)], timeout=900)
        identity = data['identity']
        if identity['content_sha256'] != self.host_pins['sdk']['content_sha256']:
            raise CiError('acquired SDK identity differs from its pin: %s' % identity)
        self.summary['sdk'] = {'version': identity['version'], 'content_sha256': identity['content_sha256']}
        # The native suite drives explicit --sdk-root/--sdk-contract flows and reads them from the project's local binding,
        # so stage 1 is bound explicitly to the installed SDK and the contract the store kept beside it.
        store = Path(self.env['HOME']) / '.crt' / 'sdks'
        project1, stage1 = self.build_stage('stage1', boot, tools,
                                            explicit_sdk=(store / identity['directory'], store / '.contracts' / (identity['directory'] + '.json')))
        self.record_versions()
        test_failures = self.run_tests(stage1, project1)
        # Stage 2 is rebuilt at stage 1's absolute path (the export is wiped) by a kept copy of the stage-1 executable.
        kept = self.work / 'stage1-kept' / stage1.name
        kept.parent.mkdir()
        shutil.copy2(stage1, kept)
        project2, stage2 = self.build_stage('stage2', kept, tools, directory='stage1')
        warm, _ = self.cli('stage2-warm', kept, 'build', project2)
        if warm['cache']['executed'] != 0 or warm['commands']:
            raise CiError('unchanged rebuild was not fully reused: %s' % warm['cache'])
        same = sha256_file(kept) == sha256_file(stage2)
        self.summary['stage_bytes_identical'] = same
        if not same:
            log('NOTE: stage 1 and stage 2 differ byte-wise; functional equivalence is checked below')
        version1, checks1 = self.bare_run('stage1', kept)
        version2, checks2 = self.bare_run('stage2', stage2)
        if version1 != version2 or checks1 != checks2:
            raise CiError('stage 1 and stage 2 behave differently: %s %s / %s %s' % (version1, checks1, version2, checks2))
        self.summary['audit'] = self.audit_imports(stage2)
        self.audit_imports(kept)
        final = self.work / 'artifact' / ('crt-cli-%s%s' % (self.host, EXE))
        shutil.copy2(stage2, final)
        (self.work / 'artifact' / ('SHA256SUMS-%s' % self.host)).write_text('%s  %s\n' % (sha256_file(final), final.name), encoding='ascii')
        if test_failures:
            raise CiError('suites failed (the remaining stages passed): %s' % '; '.join('%s: %s' % item for item in sorted(test_failures.items())))
        self.summary['result'] = 'pass'

    def report(self):
        (self.work / 'artifact' / 'ci-summary.json').write_text(json.dumps(self.summary, indent=2, sort_keys=True) + '\n', encoding='utf-8')
        target = os.environ.get('GITHUB_STEP_SUMMARY')
        if target:
            rows = ['### crt-cli CI: %s (%s)' % (self.host, self.summary.get('result', 'fail')), '', '| Item | Value |', '| --- | --- |',
                    '| source | `%s` |' % self.summary['source_commit']]
            for key in ('bootstrap', 'sdk', 'host_versions', 'stage1', 'stage2', 'tests', 'test_failures', 'stage_bytes_identical', 'audit', 'ca_sha256'):
                if key in self.summary:
                    rows.append('| %s | `%s` |' % (key, json.dumps(self.summary[key], sort_keys=True)))
            with open(target, 'a', encoding='utf-8') as stream:
                stream.write('\n'.join(rows) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--work', required=True, help='new or empty directory for downloads, stages, logs and artifacts')
    parser.add_argument('--pins', default=str(ROOT / '.github/ci/pins.json'))
    parser.add_argument('--host', choices=['linux-x64', 'macos-arm64', 'win-x64'], help='default: detect')
    parser.add_argument('--llvm-bin', help='directory with clang, clang++, ld.lld and llvm-ar (not used on macOS)')
    parser.add_argument('--os-sdk', help='Windows um/x64 import-library directory (default: newest installed)')
    parser.add_argument('--tests', nargs='+', help='run only these declared tests (default: the pinned PR set)')
    args = parser.parse_args()
    ci = None
    try:
        ci = Ci(args)
        ci.execute()
        log('CI PASS (%s); artifacts: %s' % (ci.host, ci.work / 'artifact'))
        return 0
    except CiError as error:
        log('CI FAIL: %s' % error)
        return 1
    finally:
        if ci is not None:
            ci.report()


if __name__ == '__main__':
    sys.exit(main())
