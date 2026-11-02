#!/usr/bin/env python3
"""Assemble a local one-executable bootstrap candidate with explicit data/notice pins.

Producer-only tooling. The shipped CLI executes no Python and does not infer
publisher trust from this locally generated manifest.
"""
import argparse
import hashlib
import json
import shutil
import tempfile
from pathlib import Path

COMPONENTS = ('crt-libc', 'llvm-runtimes', 'curl', 'mbedtls', 'zlib', 'nlohmann-json')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def assemble(binary, sdk, output, ca_file=None):
    binary, sdk, output = binary.resolve(), sdk.resolve(), output.absolute()
    if output.exists() or output.is_symlink():
        raise ValueError('bootstrap output already exists; select a new candidate')
    output = output.resolve()
    if output == sdk or sdk in output.parents or output in sdk.parents or output in binary.parents:
        raise ValueError('bootstrap output and immutable inputs must be disjoint')
    manifest = json.loads((sdk / 'manifest.json').read_text(encoding='utf-8'))
    suffix = '.exe' if manifest['target']['os'] == 'windows' else ''
    output.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix='.bootstrap-', dir=output.parent))
    try:
        name = 'crt-cli' + suffix
        shutil.copy2(binary, staging / name)
        for component in COMPONENTS:
            for prefix in ('share/licenses', 'share/crt/dependencies'):
                shutil.copytree(sdk / prefix / component, staging / prefix / component, symlinks=True)
        shutil.copy2(sdk / 'LICENSE.md', staging / 'LICENSE.md')
        trust = {'mode': 'explicit-at-fetch', 'ca_file': None}
        if ca_file:
            destination = staging / 'share/trust/ca.pem'; destination.parent.mkdir(parents=True)
            shutil.copy2(ca_file, destination)
            trust = {'mode': 'explicit-ca-file', 'ca_file': 'share/trust/ca.pem'}
        inventory = []
        for path in sorted(staging.rglob('*'), key=lambda p: p.relative_to(staging).parts):
            if path.is_symlink():
                raise ValueError('bootstrap resources do not accept symlinks')
            if path.is_file():
                inventory.append([path.relative_to(staging).as_posix(), 'file', digest(path)])
        data = {'schema': 'crt-cli.bootstrap-resources', 'format': 1, 'target': manifest['target'],
                'executable': name, 'trust': trust, 'inventory': inventory,
                'external_prerequisites': manifest.get('external_prerequisites', []),
                'source_sdk_manifest_sha256': digest(sdk / 'manifest.json')}
        (staging / 'manifest.json').write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        staging.rename(output)
        return digest(output / 'manifest.json')
    finally:
        if staging.exists():
            shutil.rmtree(staging)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--sdk-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--ca-file', type=Path)
    args = parser.parse_args()
    print(assemble(args.binary, args.sdk_root, args.output, args.ca_file))


if __name__ == '__main__':
    main()
