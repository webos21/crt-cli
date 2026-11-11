#!/usr/bin/env python3
"""Review aid for .github/ci/pins.json: re-derive every pin from the published release bytes.

A pin is only as good as the check that produced it. This downloads each host's pinned bootstrap CLI archive and the SDK
manifest, archive and contract from their public releases and compares size and SHA-256 (and the SDK content identity named by
the manifest) with the pins. It changes nothing and publishes nothing; a mismatch exits 1. Run it in the same pull request that
changes a pin, and quote its output in the review.
"""
import argparse
import hashlib
import json
import ssl
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


class PinError(Exception):
    pass


def fetch(url):
    with urllib.request.urlopen(url, timeout=300, context=ssl.create_default_context()) as response:
        return response.read()


def compare(label, data, size, sha256):
    actual = hashlib.sha256(data).hexdigest()
    problems = []
    if len(data) != size:
        problems.append('%s: size %d differs from the pin %d' % (label, len(data), size))
    if actual != sha256:
        problems.append('%s: SHA-256 %s differs from the pin %s' % (label, actual, sha256))
    return problems


def manifest_asset(manifest, name):
    for asset in manifest.get('assets', []):
        if asset.get('name') == name:
            return asset
    return None


def review_host(host, pins, get=fetch):
    """Return the list of problems for one host; an empty list means every pin matches the published bytes."""
    problems = []
    boot, sdk = pins['bootstrap'], pins['sdk']
    base = 'https://github.com/webos21/crt-cli/releases/download/%s/' % boot['release']
    problems += compare('%s bootstrap archive' % host, get(base + boot['asset']), boot['size'], boot['sha256'])
    manifest_bytes = get(sdk['release_base'] + sdk['manifest']['name'])
    problems += compare('%s SDK manifest' % host, manifest_bytes, sdk['manifest']['size'], sdk['manifest']['sha256'])
    manifest = json.loads(manifest_bytes)
    archive = manifest_asset(manifest, sdk['archive'])
    if archive is None:
        return problems + ['%s: the SDK manifest does not list the pinned archive %s' % (host, sdk['archive'])]
    problems += compare('%s SDK archive' % host, get(sdk['release_base'] + sdk['archive']), archive['size'], archive['sha256'])
    contract = get(sdk['release_base'] + sdk['contract'])
    if hashlib.sha256(contract).hexdigest() != sdk['contract_sha256']:
        problems.append('%s: SDK contract SHA-256 differs from the pin' % host)
    listed = manifest_asset(manifest, sdk['contract'])
    if listed is not None and listed.get('sha256') != sdk['contract_sha256']:
        problems.append('%s: the pinned contract digest differs from the one the SDK manifest lists' % host)
    for key in ('content_sha256',):
        content = manifest.get('content_sha256') or manifest.get('sdk', {}).get(key)
        if content is not None and content != sdk[key]:
            problems.append('%s: the pinned %s differs from the SDK manifest' % (host, key))
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pins', default=str(ROOT / '.github/ci/pins.json'))
    parser.add_argument('--host', action='append', help='limit to these hosts (default: all)')
    args = parser.parse_args()
    pins = json.loads(Path(args.pins).read_text(encoding='utf-8'))['hosts']
    failed = False
    for host in args.host or sorted(pins):
        try:
            problems = review_host(host, pins[host])
        except (OSError, ValueError, KeyError) as error:
            problems = ['%s: could not be checked: %s' % (host, error)]
        for problem in problems:
            print('MISMATCH ' + problem, file=sys.stderr)
        print('%s %s' % ('FAIL' if problems else 'OK  ', host), flush=True)
        failed = failed or bool(problems)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
