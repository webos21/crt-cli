#!/usr/bin/env python3
"""Review aid for .github/ci/pins.json: re-derive every pin from the published release bytes.

A pin is only as good as the check that produced it. This downloads each host's pinned bootstrap CLI archive and the SDK
manifest, archive and contract from their public releases and compares size and SHA-256 (and the SDK content identity named by
the manifest) with the pins. It changes nothing and publishes nothing; a mismatch exits 1. Run it in the same pull request that
changes a pin, and quote its output in the review. It also checks every pinned `.sig` against the published bytes and verifies the
bootstrap and SDK archives with the pinned publisher key.
"""
import argparse
import hashlib
import json
import ssl
import sys
import tempfile
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import publisher_signature  # noqa: E402

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


def review_signature(label, archive_bytes, signature_url, pin, publisher, get):
    """Problems with one signed archive: the signature pin must match the published `.sig`, and the pinned key must verify the archive."""
    try:
        publisher_signature.check_signature_pin(label, pin)
    except publisher_signature.SignatureError as error:
        return [str(error)]
    data = get(signature_url)
    problems = compare('%s signature' % label, data, pin['size'], pin['sha256'])
    with tempfile.TemporaryDirectory() as scratch:
        archive, signature = Path(scratch) / 'archive', Path(scratch) / 'signature'
        archive.write_bytes(archive_bytes)
        signature.write_bytes(data)
        try:
            if not publisher_signature.verify_file(archive, signature, publisher['key_file']):
                problems.append('%s: the pinned publisher key did not sign these bytes' % label)
        except (publisher_signature.SignatureError, OSError, ValueError) as error:
            problems.append('%s: signature check failed: %s' % (label, error))
    return problems


def review_host(host, pins, get=fetch, publisher=None):
    """Return the list of problems for one host; an empty list means every pin matches the published bytes."""
    problems = []
    boot, sdk = pins['bootstrap'], pins['sdk']
    base = 'https://github.com/webos21/crt-cli/releases/download/%s/' % boot['release']
    boot_bytes = get(base + boot['asset'])
    problems += compare('%s bootstrap archive' % host, boot_bytes, boot['size'], boot['sha256'])
    if publisher is not None:
        problems += review_signature('%s bootstrap archive' % host, boot_bytes, base + boot['asset'] + '.sig', boot.get('signature'), publisher, get)
    manifest_bytes = get(sdk['release_base'] + sdk['manifest']['name'])
    problems += compare('%s SDK manifest' % host, manifest_bytes, sdk['manifest']['size'], sdk['manifest']['sha256'])
    manifest = json.loads(manifest_bytes)
    archive = manifest_asset(manifest, sdk['archive'])
    if archive is None:
        return problems + ['%s: the SDK manifest does not list the pinned archive %s' % (host, sdk['archive'])]
    archive_bytes = get(sdk['release_base'] + sdk['archive'])
    problems += compare('%s SDK archive' % host, archive_bytes, archive['size'], archive['sha256'])
    if publisher is not None:
        problems += review_signature('%s SDK archive' % host, archive_bytes, sdk['release_base'] + sdk['archive'] + '.sig', sdk.get('signature'), publisher, get)
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
    document = json.loads(Path(args.pins).read_text(encoding='utf-8'))
    pins = document['hosts']
    try:
        publisher = publisher_signature.pinned_publisher(document.get('publisher'), ROOT)
    except publisher_signature.SignatureError as error:
        print('MISMATCH %s' % error, file=sys.stderr)
        return 1
    failed = False
    for host in args.host or sorted(pins):
        try:
            problems = review_host(host, pins[host], publisher=publisher)
        except (OSError, ValueError, KeyError) as error:
            problems = ['%s: could not be checked: %s' % (host, error)]
        for problem in problems:
            print('MISMATCH ' + problem, file=sys.stderr)
        print('%s %s' % ('FAIL' if problems else 'OK  ', host), flush=True)
        failed = failed or bool(problems)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
