#!/usr/bin/env python3
"""Pack and verify the local Preview archive of a bootstrap-resources package (producer tooling; the CLI runs no Python).

`pack` turns the directory made by create_bootstrap_resources.py into one deterministic `.tar.xz` (sorted members, owner 0,
mtime 0, modes kept, one top-level directory). `verify` extracts an archive with the stdlib's `data` filter into a NEW directory
and checks it against its own manifest: schema/format, one executable, every inventory entry's digest and size class, no extra or
missing file, no link or special file, the mandatory notices, and execute bits. Both fail closed on anything unexpected.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import stat
import sys
import tarfile

REQUIRED_NOTICES = ('LICENSE.md', 'share/licenses/crt-cli/LICENSE')
REQUIRED_COMPONENTS = ('crt-libc', 'llvm-runtimes', 'curl', 'mbedtls', 'zlib', 'nlohmann-json', 'xz')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(root):
    """Verify an extracted or assembled package directory against its manifest; return the manifest."""
    root = Path(root)
    manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('schema') != 'crt-cli.bootstrap-resources' or manifest.get('format') != 1:
        raise ValueError('unsupported package manifest schema/format')
    recorded = {}
    for relative, kind, expected in manifest['inventory']:
        if kind != 'file' or relative in recorded or relative.startswith('/') or '..' in Path(relative).parts:
            raise ValueError('unsafe or duplicate inventory entry: ' + relative)
        recorded[relative] = expected
    actual = {}
    for path in sorted(root.rglob('*')):
        relative = path.relative_to(root).as_posix()
        if path.is_symlink() or not (path.is_file() or path.is_dir()):
            raise ValueError('link or special file in the package: ' + relative)
        if path.is_file() and relative != 'manifest.json':
            actual[relative] = digest(path)
    if actual != recorded:
        missing, extra = sorted(set(recorded) - set(actual)), sorted(set(actual) - set(recorded))
        changed = sorted(name for name in set(actual) & set(recorded) if actual[name] != recorded[name])
        raise ValueError(f'package differs from its inventory: missing={missing} extra={extra} changed={changed}')
    executable = manifest['executable']
    executables = sorted(name for name in actual if os.access(root / name, os.X_OK))
    if executables != [executable]:
        raise ValueError('the only executable must be ' + executable + ', found ' + str(executables))
    for notice in REQUIRED_NOTICES:
        if notice not in actual:
            raise ValueError('missing mandatory notice: ' + notice)
    for component in REQUIRED_COMPONENTS:
        if not any(name.startswith(f'share/licenses/{component}/') for name in actual):
            raise ValueError('missing license notice for component: ' + component)
    return manifest


def pack(package, output):
    package, output = Path(package).resolve(), Path(output)
    if output.exists():
        raise ValueError('archive output already exists; select a new name')
    manifest = inspect(package)
    top = package.name
    buffer = io.BytesIO()
    with tarfile.open(fileobj=buffer, mode='w:xz', format=tarfile.PAX_FORMAT, preset=6) as archive:
        for path in sorted([package, *package.rglob('*')], key=lambda p: p.relative_to(package.parent).parts):
            info = archive.gettarinfo(str(path), arcname=path.relative_to(package.parent).as_posix())
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mtime = 0
            info.mode = 0o755 if path.is_dir() or os.access(path, os.X_OK) else 0o644
            if path.is_file():
                with path.open('rb') as stream:
                    archive.addfile(info, stream)
            else:
                archive.addfile(info)
    output.write_bytes(buffer.getvalue())
    return {'archive': str(output), 'bytes': output.stat().st_size, 'sha256': digest(output), 'top': top,
            'files': len(manifest['inventory']) + 1, 'manifest_sha256': digest(package / 'manifest.json')}


def verify(archive, destination):
    archive, destination = Path(archive), Path(destination)
    if destination.exists():
        raise ValueError('extraction target must be new')
    destination.mkdir(parents=True)
    with tarfile.open(archive, 'r:xz') as handle:
        members = handle.getmembers()
        tops = {Path(m.name).parts[0] for m in members}
        if len(tops) != 1:
            raise ValueError('an archive must have exactly one top-level directory')
        for member in members:
            if not (member.isfile() or member.isdir()):
                raise ValueError('link or special member refused: ' + member.name)
        handle.extractall(destination, filter='data')
    root = destination / tops.pop()
    manifest = inspect(root)
    return {'root': str(root), 'files': len(manifest['inventory']) + 1, 'manifest_sha256': digest(root / 'manifest.json'),
            'executable': manifest['executable']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    p = sub.add_parser('pack'); p.add_argument('--package', required=True); p.add_argument('--output', required=True)
    v = sub.add_parser('verify'); v.add_argument('--archive', required=True); v.add_argument('--extract-to', required=True)
    args = parser.parse_args()
    result = pack(args.package, args.output) if args.command == 'pack' else verify(args.archive, args.extract_to)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    sys.exit(main())
