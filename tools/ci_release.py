#!/usr/bin/env python3
"""Release CI driver for crt-cli; a build-host orchestrator, never a CLI runtime dependency.

  package  pinned published CLI + pinned public SDK -> build the commit -> bootstrap resources -> deterministic archive
  verify   extract the archive on a fresh runner, check inventory, run the packaged executable and a real SDK acquisition
  publish  re-check the artifact digests, stage a draft GitHub release, verify it, publish it as a non-latest prerelease

Naming and note rules are pure functions so they are unit-tested; publication itself is the only step with write
permission and it sits behind a protected environment. CI success is not publication authority.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))


class ReleaseError(Exception):
    pass


# host key -> (tag architecture, asset machine name, SHA256SUMS suffix); the tag carries the OS-architecture, the asset the machine name.
HOSTS = {
    'linux-x64': ('linux-x86_64', 'linux-x86_64', 'linux-x86_64'),
    'linux-arm64': ('linux-arm64', 'linux-aarch64', 'linux-aarch64'),
    'macos-arm64': ('macos-arm64', 'macos-aarch64', 'macos-aarch64'),
    'win-x64': ('windows-x86_64', 'windows-x86_64', 'windows-x86_64'),
}
PRODUCT_VERSION = '0.1.0-dev'
DATE = re.compile(r'^20\d{6}$')
SHA40 = re.compile(r'^[0-9a-f]{40}$')


def names(host, date, serial):
    """Tag, archive and checksum-file names for one release, e.g. v0.1.0-preview.20261111.linux-arm64.1."""
    if host not in HOSTS:
        raise ReleaseError('unsupported release host: %s' % host)
    if not DATE.match(str(date)):
        raise ReleaseError('date must look like 20261111')
    if type(serial) is not int or serial < 1:
        raise ReleaseError('serial must be a positive integer')
    tag_arch, machine, sums = HOSTS[host]
    base = '0.1.0-preview.%s' % date
    return {
        'tag': 'v%s.%s.%d' % (base, tag_arch, serial),
        'archive': 'crt-cli-%s-%s-preview.%s.%d.tar.xz' % (PRODUCT_VERSION, machine, date, serial),
        'sums': 'SHA256SUMS-%s' % sums,
        'title': 'crt-cli Preview %s (%s)' % (date, {'linux-x64': 'Linux x86_64', 'linux-arm64': 'Linux aarch64',
                                                      'macos-arm64': 'macOS arm64', 'win-x64': 'Windows x86_64'}[host]),
    }


def sums_line(archive_name, digest):
    if not re.fullmatch(r'[0-9a-f]{64}', digest):
        raise ReleaseError('digest is not a lowercase SHA-256')
    return '%s  %s\n' % (digest, archive_name)


def parse_sums(text):
    entries = {}
    for line in text.splitlines():
        match = re.fullmatch(r'([0-9a-f]{64})  (\S+)', line)
        if not match:
            raise ReleaseError('malformed checksum line: %r' % line)
        if match.group(2) in entries:
            raise ReleaseError('duplicate checksum entry: %s' % match.group(2))
        entries[match.group(2)] = match.group(1)
    if not entries:
        raise ReleaseError('checksum file is empty')
    return entries


def require_commit(sha):
    if not SHA40.match(sha or ''):
        raise ReleaseError('a release is built from a full lowercase 40-character commit SHA, never a branch or tag')
    return sha


def require_artifact_match(archive_path, sums_text, archive_name):
    """The bytes about to be uploaded must equal the digest the verify job saw."""
    entries = parse_sums(sums_text)
    if set(entries) != {archive_name}:
        raise ReleaseError('checksum file must list exactly %s, found %s' % (archive_name, sorted(entries)))
    actual = hashlib.sha256(Path(archive_path).read_bytes()).hexdigest()
    if actual != entries[archive_name]:
        raise ReleaseError('archive digest %s differs from the verified digest %s' % (actual, entries[archive_name]))
    return actual


def locate_executable(artifact_dir, expected_sha256):
    """The artifact file whose bytes are the verified stage-2 executable; names are never guessed."""
    matches = [p for p in Path(artifact_dir).iterdir()
               if p.is_file() and not p.name.startswith('SHA256SUMS') and p.suffix != '.json'
               and hashlib.sha256(p.read_bytes()).hexdigest() == expected_sha256]
    if len(matches) != 1:
        raise ReleaseError('expected exactly one artifact with the verified executable digest, found %d' % len(matches))
    return matches[0]


def locate_sdk(store, sdk):
    """The installed SDK directory named by the summary's version and content identity."""
    directory = Path(store) / ('%s-%s' % (sdk['version'], sdk['content_sha256'][:12]))
    if not (directory / 'manifest.json').is_file() or not (directory / 'share/licenses').is_dir():
        raise ReleaseError('the installed SDK %s is not where the summary says' % directory.name)
    return directory


def log(message):
    print(message, file=sys.stderr, flush=True)


def run(label, argv, **kwargs):
    log('[%s] %s' % (label, ' '.join(map(str, argv))))
    done = subprocess.run([str(a) for a in argv], **kwargs)
    if done.returncode != 0:
        raise ReleaseError('%s failed with exit %d' % (label, done.returncode))
    return done


def command_package(args):
    """Build the exact commit with the pinned published CLI and the pinned public SDK, then pack deterministically.

    The heavy lifting is tools/ci_bootstrap.py (bootstrap, SDK acquisition, stage 1 and stage 2); this adds the resource
    assembly and the archive, and refuses a tree that is not the requested commit.
    """
    commit = require_commit(args.commit)
    head = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != commit:
        raise ReleaseError('checked-out commit %s is not the requested %s' % (head, commit))
    if subprocess.run(['git', '-C', str(ROOT), 'status', '--porcelain'], capture_output=True, text=True).stdout.strip():
        raise ReleaseError('the working tree is not clean; a release is built from a committed tree only')
    n = names(args.host, args.date, args.serial)
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=False)
    ci_work = work / 'ci'
    argv = [sys.executable, ROOT / 'tools/ci_bootstrap.py', '--work', ci_work, '--host', args.host, '--native-test-jobs', '2']
    if args.llvm_bin:
        argv += ['--llvm-bin', args.llvm_bin]
    run('ci-bootstrap', argv)
    summary = json.loads((ci_work / 'artifact/ci-summary.json').read_text(encoding='utf-8'))
    if summary.get('result') != 'pass' or summary.get('source_commit') != commit:
        raise ReleaseError('the CI driver did not pass for the requested commit')
    exe = locate_executable(ci_work / 'artifact', summary['stage2']['sha256'])
    sdk_root = locate_sdk(ci_work / 'home' / '.crt' / 'sdks', summary['sdk'])
    package = work / 'package' / 'crt-cli'
    run('assemble', [sys.executable, ROOT / 'tools/create_bootstrap_resources.py', '--binary', exe, '--sdk-root', sdk_root, '--output', package])
    out = work / 'release'
    out.mkdir()
    archive = out / n['archive']
    run('pack', [sys.executable, ROOT / 'tools/package_cli_archive.py', 'pack', '--package', package, '--output', archive])
    twice = work / 'second.tar.xz'
    run('pack-again', [sys.executable, ROOT / 'tools/package_cli_archive.py', 'pack', '--package', package, '--output', twice])
    if archive.read_bytes() != twice.read_bytes():
        raise ReleaseError('packing the same package twice gave different bytes')
    twice.unlink()
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (out / n['sums']).write_text(sums_line(n['archive'], digest), encoding='ascii')
    (out / 'release.json').write_text(json.dumps({'commit': commit, 'host': args.host, 'names': n, 'archive_sha256': digest,
                                                  'archive_bytes': archive.stat().st_size, 'executable_sha256': summary['stage2']['sha256'],
                                                  'sdk': summary['sdk'], 'bootstrap': summary['bootstrap']}, indent=2) + '\n', encoding='utf-8')
    log('packaged %s (%d bytes) sha256=%s' % (n['archive'], archive.stat().st_size, digest))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    for name in ('package',):
        p = sub.add_parser(name)
        p.add_argument('--host', choices=sorted(HOSTS), required=True)
        p.add_argument('--commit', required=True)
        p.add_argument('--date', required=True)
        p.add_argument('--serial', type=int, required=True)
        p.add_argument('--work', required=True)
        p.add_argument('--llvm-bin')
    args = parser.parse_args()
    try:
        if args.command == 'package':
            command_package(args)
    except ReleaseError as error:
        log('RELEASE FAIL: %s' % error)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
