#!/usr/bin/env python3
"""Daily/manual integration driver for crt-cli; a build-host orchestrator, never a CLI runtime dependency.

Reads .github/ci/integration-pins.json, downloads every pinned input, checks its size and SHA-256, and only then
runs the replays. A missing (null) pin or a digest mismatch is a failure: a skipped suite looks like a pass.
Subcommands:
  pins      check the pin file only (what a scheduled run needs to know before spending runner time)
  redirect  restricted-redirect replay: the pinned published CLI against the pinned public SDK bytes
  ports     the five input-gated port suites through the pinned host tools and zlib source (fails while unpinned)
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
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parent.parent


class IntegrationError(Exception):
    pass


HOST_TOOL_FIELDS = ('url', 'name', 'sha256', 'size', 'manifest_sha256', 'version')
ZLIB_FIELDS = ('url', 'name', 'sha256', 'size')
PORT_SUITES = ('port-build-test', 'port-store-test', 'port-oracle-test', 'port-command-test', 'port-project-test')
HOST_TARGETS = {'linux-x64': {'os': 'linux', 'arch': 'x86_64'},
                'linux-arm64': {'os': 'linux', 'arch': 'aarch64'}}
MAX_HOST_TOOL_BYTES = 512 * 1024 * 1024
MAX_HOST_TOOL_MEMBERS = 32768


def sha256_file(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def _require_fields(label, entry, fields):
    if not isinstance(entry, dict):
        raise IntegrationError('%s: pin is not an object' % label)
    missing = [name for name in fields if entry.get(name) in (None, '')]
    if missing:
        raise IntegrationError('%s: unpinned %s (an empty pin is a failure, not a skip)' % (label, ', '.join(missing)))
    if not str(entry['url']).startswith('https://'):
        raise IntegrationError('%s: url must be https' % label)
    name = entry['name']
    if not isinstance(name, str) or name in ('.', '..') or any(c in name for c in '/\\:'):
        raise IntegrationError('%s: name must be a plain file name' % label)
    for name in ('sha256', 'manifest_sha256'):
        if name in entry and (not isinstance(entry[name], str) or re.fullmatch(r'[0-9a-f]{64}', entry[name]) is None):
            raise IntegrationError('%s: %s is not a lowercase SHA-256' % (label, name))
    if type(entry['size']) is not int or entry['size'] <= 0:
        raise IntegrationError('%s: size must be a positive integer' % label)


def load_pins(path, host):
    pins = json.loads(Path(path).read_text(encoding='utf-8'))
    if pins.get('schema') != 'crt-cli.ci-integration-pins' or pins.get('format') != 0:
        raise IntegrationError('unsupported integration pin file')
    if host not in pins.get('hosts', {}):
        raise IntegrationError('no integration pins for host %s' % host)
    _require_fields('zlib_source', pins.get('zlib_source'), ZLIB_FIELDS)
    _require_fields('%s host_tools' % host, pins['hosts'][host].get('host_tools'), HOST_TOOL_FIELDS)
    return pins['zlib_source'], pins['hosts'][host]['host_tools']


def verify_download(label, path, entry):
    path = Path(path)
    if not path.is_file():
        raise IntegrationError('%s: download is missing' % label)
    if path.stat().st_size != entry['size']:
        raise IntegrationError('%s: size %d differs from the pin %d' % (label, path.stat().st_size, entry['size']))
    actual = sha256_file(path)
    if actual != entry['sha256']:
        raise IntegrationError('%s: SHA-256 %s differs from the pin %s' % (label, actual, entry['sha256']))


def log(message):
    print(message, file=sys.stderr, flush=True)


def host_key(system=None, machine=None):
    system = system if system is not None else sys.platform
    machine = (machine if machine is not None else platform.machine()).lower()
    if system.startswith('linux') and machine in ('x86_64', 'amd64'):
        return 'linux-x64'
    if system.startswith('linux') and machine in ('aarch64', 'arm64'):
        return 'linux-arm64'
    raise IntegrationError('integration runs support Linux/x86_64 and Linux/aarch64 only: %s/%s' % (system, machine))


def download(url, destination, attempts=3):
    """Bounded retries; the SHA-256 pin, not TLS, authenticates the bytes."""
    destination = Path(destination)
    for attempt in range(1, attempts + 1):
        try:
            with urllib.request.urlopen(url, timeout=180, context=ssl.create_default_context()) as src, destination.open('wb') as dst:
                shutil.copyfileobj(src, dst)
            return
        except OSError as error:
            log('download attempt %d of %s failed: %s' % (attempt, url, error))
            if attempt == attempts:
                raise IntegrationError('cannot download %s' % url)
            time.sleep(5 * attempt)


def fetch_pinned(label, url, destination, entry):
    download(url, destination)
    verify_download(label, destination, entry)


def run(label, argv, timeout=3600, **kwargs):
    log('[%s] %s' % (label, ' '.join(map(str, argv))))
    done = subprocess.run([str(a) for a in argv], timeout=timeout, **kwargs)
    if done.returncode != 0:
        raise IntegrationError('%s failed with exit %d' % (label, done.returncode))
    return done


def published_cli(host, work):
    """The same pinned, published Preview that bootstraps ci.yml; its pin lives in .github/ci/pins.json."""
    ci = json.loads((ROOT / '.github/ci/pins.json').read_text(encoding='utf-8'))['hosts'][host]
    pin = ci['bootstrap']
    archive = Path(work) / pin['asset']
    fetch_pinned('bootstrap CLI', 'https://github.com/webos21/crt-cli/releases/download/%s/%s' % (pin['release'], pin['asset']), archive, pin)
    target = Path(work) / 'cli'
    target.mkdir()
    with tarfile.open(archive) as bundle:
        bundle.extractall(target, filter='data')
    found = [p for p in target.rglob('crt-cli') if p.is_file()]
    if len(found) != 1:
        raise IntegrationError('expected one crt-cli in the bootstrap archive, found %d' % len(found))
    found[0].chmod(found[0].stat().st_mode | 0o111)
    return found[0], ci['sdk']


def sdk_release_dir(sdk, work):
    """The three public SDK assets, each checked against the ci pins, laid out as replay_redirect_linux.py expects."""
    release = Path(work) / 'sdk-release'
    release.mkdir()
    base = sdk['release_base']
    manifest = sdk['manifest']
    fetch_pinned('SDK manifest', base + manifest['name'], release / manifest['name'], manifest)
    contract = release / sdk['contract']
    download(base + sdk['contract'], contract)
    if sha256_file(contract) != sdk['contract_sha256']:
        raise IntegrationError('SDK contract does not match its pin')
    archive = release / sdk['archive']
    download(base + sdk['archive'], archive)
    manifest_data = json.loads((release / manifest['name']).read_text(encoding='utf-8'))
    entry = next((a for a in manifest_data['assets'] if a.get('name') == sdk['archive']), None)
    if entry is None:
        raise IntegrationError('the SDK manifest does not list the pinned archive')
    verify_download('SDK archive', archive, {'size': entry['size'], 'sha256': entry['sha256']})
    return release


def command_redirect(args, host):
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=False)
    exe, sdk = published_cli(host, work)
    release = sdk_release_dir(sdk, work)
    output = work / 'redirect-replay'
    run('redirect-replay', [sys.executable, ROOT / 'tools/replay_redirect_linux.py', '--cli', exe, '--release-dir', release, '--output', output])
    result = json.loads((output / 'redirect-replay.json').read_text(encoding='utf-8'))
    if result.get('passed') is not True or not result.get('cases'):
        raise IntegrationError('redirect replay did not report a complete pass')
    log('redirect replay passed: %d cases' % len(result['cases']))


def command_ports(args, host):
    # Preparation can use a local fixture; it never reports native acceptance.
    zlib, tools = load_pins(args.pins, host)
    if not args.work:
        raise IntegrationError('ports requires --work (a fresh directory)')
    work = Path(args.work).resolve()
    if not args.prepare_only:
        if host != host_key():
            raise IntegrationError('port replay host must match the native execution host')
        if work.is_relative_to(ROOT) or work.is_relative_to('/tmp'):
            raise IntegrationError('native work must be outside the source checkout and /tmp')
        if not args.llvm_bin:
            raise IntegrationError('native ports requires --llvm-bin')
        verify_oracle(args.oracle_python, args.oracle_python_sha256)
    prepared = prepare_port_inputs(work, host, zlib, tools, input_dir=args.input_dir)
    if args.prepare_only:
        log('port inputs prepared only; no suite executed: %s' % (work / 'port-inputs.json'))
        return

    exe, sdk = published_cli(host, work)
    release = sdk_release_dir(sdk, work)
    home = work / 'home'
    home.mkdir()
    store = home / '.crt/sdks'
    env = dict(os.environ, HOME=str(home), USERPROFILE=str(home))
    installed = run('install-sdk', [exe, '--json', 'sdk', 'install', '--archive', release / sdk['archive'],
                    '--release-manifest', release / sdk['manifest']['name'],
                    '--release-manifest-sha256', sdk['manifest']['sha256'], '--store', store],
                    env=env, capture_output=True, text=True)
    (work / 'install-sdk.stdout').write_text(installed.stdout, encoding='utf-8')
    (work / 'install-sdk.stderr').write_text(installed.stderr, encoding='utf-8')
    reply = json.loads(installed.stdout)
    identity = reply.get('data', {}).get('identity', {})
    if reply.get('ok') is not True or identity.get('content_sha256') != sdk['content_sha256'] or identity.get('version') != sdk['version']:
        raise IntegrationError('installed SDK does not match the public pin')
    directory = identity['directory']
    if not isinstance(directory, str) or directory != Path(directory).name or directory in ('.', '..'):
        raise IntegrationError('invalid installed SDK directory')
    contract = store / '.contracts' / (directory + '.json')
    if sha256_file(contract) != sdk['contract_sha256']:
        raise IntegrationError('stored SDK contract does not match the pin')
    prepared['contract_sha256'] = sdk['contract_sha256']
    llvm = Path(args.llvm_bin).absolute()
    argv = [sys.executable, ROOT / 'tools/replay_port_linux.py', '--source', ROOT,
            '--sdk-root', store / directory, '--contract', contract, '--contract-sha256', sdk['contract_sha256'],
            '--host-tools', prepared['host_tools_root'], '--host-tools-sha256', tools['manifest_sha256'],
            '--archive', prepared['source_archive'], '--seed', exe, '--output', work / 'replay',
            '--work-parent', work / 'native-work', '--oracle-python', args.oracle_python,
            '--oracle-python-sha256', args.oracle_python_sha256,
            '--cc', llvm / 'clang', '--cxx', llvm / 'clang++', '--linker', llvm / 'ld.lld', '--ar', llvm / 'llvm-ar']
    for path in args.hide:
        argv += ['--hide', path]
    argv += ['--cases', *PORT_SUITES]
    (work / 'replay-command.json').write_text(json.dumps(list(map(str, argv)), indent=2) + '\n', encoding='utf-8')
    # Isolation stays in the Linux adapter; this common driver owns input and result policy.
    done = subprocess.run(list(map(str, argv)), capture_output=True, text=True, timeout=6600)
    (work / 'replay.stdout').write_text(done.stdout, encoding='utf-8')
    (work / 'replay.stderr').write_text(done.stderr, encoding='utf-8')
    judge_port_result(work / 'replay/result.json', done.returncode, prepared)
    log('all five native port suites passed on %s' % host)


def verify_oracle(path, pin):
    if not path or not pin or re.fullmatch(r'[0-9a-f]{64}', pin) is None:
        raise IntegrationError('native ports requires --oracle-python and its lowercase --oracle-python-sha256')
    path = Path(path)
    if not path.is_absolute() or not path.is_file() or not os.access(path, os.X_OK) or sha256_file(path) != pin:
        raise IntegrationError('oracle Python executable does not match its pin')


def extract_host_tools(archive, destination, pin, host):
    """Bounded staging, not a replacement for native HostTools::load validation."""
    destination = Path(destination)
    staging = destination.with_name(destination.name + '.extracting')
    staging.mkdir()
    with tarfile.open(archive) as bundle:
        seen, size, count = set(), 0, 0
        for member in bundle:
            count += 1
            path = PurePosixPath(member.name)
            if (path.is_absolute() or '..' in path.parts or '\\' in member.name or ':' in member.name or
                    not (member.isfile() or member.isdir() or member.issym())):
                raise IntegrationError('unsafe host-tools archive member: %s' % member.name)
            key = str(path).casefold()
            if key in seen:
                raise IntegrationError('duplicate or case-colliding host-tools archive member')
            seen.add(key)
            size += member.size
            if count > MAX_HOST_TOOL_MEMBERS or size > MAX_HOST_TOOL_BYTES:
                raise IntegrationError('host-tools extraction limit exceeded')
            bundle.extract(member, staging, filter='data')
    manifests = list(staging.rglob('manifest.json'))
    if len(manifests) != 1 or manifests[0].is_symlink() or manifests[0].stat().st_size > 16 * 1024 * 1024:
        raise IntegrationError('expected one regular host-tools manifest')
    manifest_path = manifests[0]
    if sha256_file(manifest_path) != pin['manifest_sha256']:
        raise IntegrationError('host-tools manifest SHA-256 mismatch')
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest.get('schema') != 'crt.experimental.host-tools' or manifest.get('format') != 1:
        raise IntegrationError('unsupported host-tools manifest')
    if manifest.get('host') != HOST_TARGETS[host] or manifest.get('version') != pin['version']:
        raise IntegrationError('host-tools manifest host/version mismatch')
    if manifest.get('assembler', {}).get('working_tree_dirty') is not False:
        raise IntegrationError('host-tools assembler provenance must be clean')
    relative = manifest_path.parent.relative_to(staging)
    staging.rename(destination)
    return destination / relative


def prepare_port_inputs(work, host, zlib, tools, *, input_dir=None):
    """Common online/offline input interface. Local fixtures obey the same byte pins."""
    _require_fields('zlib_source', zlib, ZLIB_FIELDS)
    _require_fields('host_tools', tools, HOST_TOOL_FIELDS)
    if host not in HOST_TARGETS:
        raise IntegrationError('unsupported port host')
    work = Path(work).resolve()
    work.mkdir(parents=True, exist_ok=False)
    downloads = work / 'downloads'
    downloads.mkdir()
    if zlib['name'] == tools['name']:
        raise IntegrationError('port inputs must have distinct file names')
    for label, entry in [('zlib source', zlib), ('host tools', tools)]:
        destination = downloads / entry['name']
        if input_dir is None:
            fetch_pinned(label, entry['url'], destination, entry)
        else:
            source = Path(input_dir) / entry['name']
            verify_download(label, source, entry)
            shutil.copyfile(source, destination)
            verify_download(label, destination, entry)
    bundle = extract_host_tools(downloads / tools['name'], work / 'host-tools', tools, host)
    prepared = {'schema': 'crt-cli.ci-port-inputs', 'format': 0, 'host': host,
                'source_archive': str(downloads / zlib['name']), 'source_sha256': zlib['sha256'],
                'host_tools_root': str(bundle), 'host_tools_sha256': tools['manifest_sha256'],
                'zlib_source': zlib, 'host_tools': tools, 'input_mode': 'local' if input_dir is not None else 'download',
                'native_validation': 'not_run'}
    (work / 'port-inputs.json').write_text(json.dumps(prepared, indent=2) + '\n', encoding='utf-8')
    return prepared


def judge_port_result(path, returncode, prepared):
    if returncode != 0:
        raise IntegrationError('Linux port adapter failed with exit %s' % returncode)
    result = json.loads(Path(path).read_text(encoding='utf-8'))
    if not isinstance(result, dict) or result.get('passed') is not True or result.get('allow_dirty_sdk') is not False:
        raise IntegrationError('port replay did not report a clean complete pass')
    cases = result.get('cases')
    if not isinstance(cases, list) or not cases or any(not isinstance(c, dict) or c.get('passed') is not True for c in cases):
        raise IntegrationError('port replay has missing or failed cases')
    suites = result.get('suites')
    if not isinstance(suites, dict) or set(suites) != set(PORT_SUITES):
        raise IntegrationError('port replay must report exactly the five required suites')
    for name, outcome in suites.items():
        if (not isinstance(outcome, dict) or outcome.get('passed') is not True or type(outcome.get('checks')) is not int or outcome['checks'] <= 0 or
                outcome.get('skipped', False) is not False):
            raise IntegrationError('%s did not execute and pass' % name)
    if (result.get('host_tools_sha256') != prepared['host_tools_sha256'] or
            result.get('archive_sha256') != prepared['source_sha256'] or
            result.get('contract_sha256') != prepared['contract_sha256']):
        raise IntegrationError('port replay inputs differ from the prepared pins')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['pins', 'redirect', 'ports'])
    parser.add_argument('--host', choices=['linux-x64', 'linux-arm64'], help='default: detect')
    parser.add_argument('--work', help='new directory for downloads and results')
    parser.add_argument('--pins', default=str(ROOT / '.github/ci/integration-pins.json'))
    parser.add_argument('--prepare-only', action='store_true', help='ports: verify/stage inputs without running native suites')
    parser.add_argument('--input-dir', type=Path, help='ports: use local files, verifying the same pins (also for test fixtures)')
    parser.add_argument('--llvm-bin', help='ports: declared native LLVM directory')
    parser.add_argument('--oracle-python', help='ports: absolute test-host Python for the SDK oracle only')
    parser.add_argument('--oracle-python-sha256', help='ports: independently selected oracle Python executable digest')
    parser.add_argument('--hide', action='append', default=[], help='ports: additional original input tree hidden by the Linux adapter')
    args = parser.parse_args()
    try:
        host = args.host or host_key()
        if args.command == 'pins':
            load_pins(args.pins, host)
            log('integration pins complete for %s' % host)
        elif args.command == 'redirect':
            if not args.work:
                parser.error('--work is required')
            command_redirect(args, host)
        else:
            command_ports(args, host)
    except (IntegrationError, OSError, ValueError, KeyError, tarfile.TarError, subprocess.TimeoutExpired) as error:
        log('INTEGRATION FAIL: %s' % error)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
