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
import platform
import shutil
import ssl
import subprocess
import sys
import tarfile
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


class IntegrationError(Exception):
    pass


HOST_TOOL_FIELDS = ('url', 'name', 'sha256', 'size', 'manifest_sha256', 'version')
ZLIB_FIELDS = ('url', 'name', 'sha256', 'size')


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
    for name in ('sha256', 'manifest_sha256'):
        if name in entry and (len(entry[name]) != 64 or any(c not in '0123456789abcdef' for c in entry[name])):
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
    # Every input is checked before any runner time is spent on the replay.
    zlib, tools = load_pins(ROOT / '.github/ci/integration-pins.json', host)
    raise IntegrationError('port suites are not wired to a Linux replay driver yet (inputs are pinned: zlib %s, host tools %s)'
                           % (zlib['sha256'][:12], tools['version']))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['pins', 'redirect', 'ports'])
    parser.add_argument('--host', choices=['linux-x64', 'linux-arm64'], help='default: detect')
    parser.add_argument('--work', help='new directory for downloads and results')
    args = parser.parse_args()
    try:
        host = args.host or host_key()
        if args.command == 'pins':
            load_pins(ROOT / '.github/ci/integration-pins.json', host)
            log('integration pins complete for %s' % host)
        elif args.command == 'redirect':
            if not args.work:
                parser.error('--work is required')
            command_redirect(args, host)
        else:
            command_ports(args, host)
    except IntegrationError as error:
        log('INTEGRATION FAIL: %s' % error)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
