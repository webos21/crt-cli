"""Detached RSA-PSS publisher signature verification with the Python standard library only (no OpenSSL, no third-party package).

Accepts exactly what `crt-cli fetch --publisher-key --signature` accepts: an RSA public key of 2048..4096 bits in a PEM
SubjectPublicKeyInfo, and a signature over the exact asset bytes made with RSASSA-PSS, SHA-256, MGF1-SHA256 and a 32-byte salt
(the signature is as long as the modulus). The key is always a caller-chosen file; nothing here fetches or trusts a key.
"""
import base64
import hashlib
from pathlib import Path
import re

RSA_ENCRYPTION_OID = bytes.fromhex('2a864886f70d010101')
SALT_BYTES = 32
MIN_BITS, MAX_BITS = 2048, 4096


class SignatureError(Exception):
    pass


def _der(data, offset, expected_tag):
    if offset + 2 > len(data) or data[offset] != expected_tag:
        raise SignatureError('malformed public key')
    length = data[offset + 1]
    start = offset + 2
    if length & 0x80:
        count = length & 0x7F
        if count == 0 or count > 3 or start + count > len(data):
            raise SignatureError('malformed public key')
        length = int.from_bytes(data[start:start + count], 'big')
        start += count
    if start + length > len(data):
        raise SignatureError('malformed public key')
    return data[start:start + length], start + length


def public_key_der(path):
    text = Path(path).read_text(encoding='ascii')
    match = re.fullmatch(r'\s*-----BEGIN PUBLIC KEY-----\s*([A-Za-z0-9+/=\s]+?)\s*-----END PUBLIC KEY-----\s*', text)
    if match is None:
        raise SignatureError('the key file is not a PEM SubjectPublicKeyInfo')
    return base64.b64decode(''.join(match.group(1).split()), validate=True)


def spki_sha256(path):
    """The published fingerprint: SHA-256 of the DER SubjectPublicKeyInfo."""
    return hashlib.sha256(public_key_der(path)).hexdigest()


def load_public_key(path):
    der = public_key_der(path)
    body, end = _der(der, 0, 0x30)
    if end != len(der):
        raise SignatureError('trailing data after the public key')
    algorithm, offset = _der(body, 0, 0x30)
    oid, _ = _der(algorithm, 0, 0x06)
    if oid != RSA_ENCRYPTION_OID:
        raise SignatureError('the public key is not RSA')
    bits, offset = _der(body, offset, 0x03)
    if offset != len(body) or not bits or bits[0] != 0:
        raise SignatureError('malformed public key')
    pair, end = _der(bits[1:], 0, 0x30)
    if end != len(bits) - 1:
        raise SignatureError('malformed public key')
    modulus_bytes, offset = _der(pair, 0, 0x02)
    exponent_bytes, offset = _der(pair, offset, 0x02)
    if offset != len(pair):
        raise SignatureError('malformed public key')
    modulus, exponent = int.from_bytes(modulus_bytes, 'big'), int.from_bytes(exponent_bytes, 'big')
    if not MIN_BITS <= modulus.bit_length() <= MAX_BITS or exponent < 3 or exponent % 2 == 0 or exponent >= modulus:
        raise SignatureError('the RSA key must be 2048..4096 bits with an odd public exponent')
    return modulus, exponent


def _mgf1(seed, length):
    out = b''
    counter = 0
    while len(out) < length:
        out += hashlib.sha256(seed + counter.to_bytes(4, 'big')).digest()
        counter += 1
    return out[:length]


def verify(message_sha256, signature, key):
    """RFC 8017 RSASSA-PSS-VERIFY over a precomputed SHA-256 digest (bytes) with the fixed parameters above."""
    modulus, exponent = key
    modulus_bytes = (modulus.bit_length() + 7) // 8
    if len(message_sha256) != 32 or len(signature) != modulus_bytes:
        return False
    value = int.from_bytes(signature, 'big')
    if value >= modulus:
        return False
    bits = modulus.bit_length() - 1
    em_len = (bits + 7) // 8
    encoded = pow(value, exponent, modulus).to_bytes(modulus_bytes, 'big')[modulus_bytes - em_len:]
    if em_len < 32 + SALT_BYTES + 2 or encoded[-1] != 0xBC:
        return False
    masked, digest = encoded[:em_len - 32 - 1], encoded[em_len - 32 - 1:-1]
    top_bits = 8 * em_len - bits
    if masked[0] >> (8 - top_bits) if top_bits else False:
        return False
    block = bytes(a ^ b for a, b in zip(masked, _mgf1(digest, len(masked))))
    if top_bits:
        block = bytes([block[0] & (0xFF >> top_bits)]) + block[1:]
    padding = em_len - 32 - SALT_BYTES - 2
    if block[:padding] != b'\x00' * padding or block[padding] != 0x01:
        return False
    salt = block[padding + 1:]
    expected = hashlib.sha256(b'\x00' * 8 + message_sha256 + salt).digest()
    return len(salt) == SALT_BYTES and expected == digest


def verify_file(archive, signature_path, key_path):
    """True only if the pinned key's signature covers exactly the bytes of `archive`."""
    digest = hashlib.sha256()
    with Path(archive).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return verify(digest.digest(), Path(signature_path).read_bytes(), load_public_key(key_path))


def pinned_publisher(entry, root):
    """Validate a pin file's `publisher` object and return {'key_file': Path, 'key_spki_sha256': hex}.

    The key is a repository-relative file and the fingerprint is the published value; they must agree, so editing either alone fails."""
    from pathlib import PurePosixPath
    if not isinstance(entry, dict) or set(entry) - {'key_file', 'key_spki_sha256', 'note'}:
        raise SignatureError('publisher: pin is missing or has unknown fields')
    fingerprint, relative = entry.get('key_spki_sha256'), entry.get('key_file')
    if not isinstance(fingerprint, str) or re.fullmatch(r'[0-9a-f]{64}', fingerprint) is None:
        raise SignatureError('publisher: key_spki_sha256 is not a lowercase SHA-256')
    if (not isinstance(relative, str) or PurePosixPath(relative).is_absolute() or '..' in PurePosixPath(relative).parts or
            '\\' in relative):
        raise SignatureError('publisher: key_file must be a repository-relative path')
    key = Path(root) / relative
    if key.is_symlink() or not key.is_file() or key.stat().st_size > 65536:
        raise SignatureError('publisher: key_file is not a regular file of at most 64 KiB')
    if spki_sha256(key) != fingerprint:
        raise SignatureError('publisher: key_file does not match the pinned key_spki_sha256')
    load_public_key(key)
    return {'key_file': key, 'key_spki_sha256': fingerprint}


def check_signature_pin(label, entry, max_bytes=1024):
    """A signature pin is a small file with the same url/name/size/sha256 shape as every other pinned download."""
    if (not isinstance(entry, dict) or not isinstance(entry.get('name'), str) or type(entry.get('size')) is not int or
            not 0 < entry['size'] <= max_bytes or not isinstance(entry.get('sha256'), str) or
            re.fullmatch(r'[0-9a-f]{64}', entry['sha256']) is None or any(c in entry['name'] for c in '/\\:')):
        raise SignatureError('%s: the signature pin needs a plain name, a size of at most %d bytes and a lowercase sha256' % (label, max_bytes))


def authenticate(label, archive, signature_path, signature_pin, publisher):
    """The signature file must be the pinned bytes, and the pinned key must have signed exactly `archive`. Raises SignatureError otherwise."""
    path = Path(signature_path)
    check_signature_pin(label, signature_pin)
    if not path.is_file() or path.stat().st_size != signature_pin['size'] or hashlib.sha256(path.read_bytes()).hexdigest() != signature_pin['sha256']:
        raise SignatureError('%s: the signature file does not match its pin' % label)
    if not verify_file(archive, path, publisher['key_file']):
        raise SignatureError('%s: publisher signature verification failed' % label)
