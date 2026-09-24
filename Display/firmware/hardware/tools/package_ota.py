"""Create a signed public OTA package. Never publish the private signing key."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import struct
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa

def generate(private_path, public_path):
    if private_path.exists() or public_path.exists():
        raise ValueError("Key already exists; refusing replacement")
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    private_path.parent.mkdir(parents=True, exist_ok=True)
    public_path.parent.mkdir(parents=True, exist_ok=True)
    private_path.write_bytes(key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
    public_path.write_bytes(key.public_key().public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo))

def package(image, version, sequence, key_path, output):
    data = image.read_bytes()
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version) or sequence <= 0:
        raise ValueError("Invalid release version/sequence")
    major, minor, patch = map(int, version.split('.'))
    if version != f'{major}.{minor}.{patch}' or major > 429495 or minor > 99 or patch > 99 or sequence != major * 10000 + minor * 100 + patch:
        raise ValueError("Sequence must match major*10000 + minor*100 + patch; minor/patch 0-99")
    # ESP32-S3 header and ESP-IDF descriptor; reject simulator, merged/full-flash and wrong version.
    if not 1024 <= len(data) <= 0x400000 or data[0] != 0xE9 or struct.unpack_from('<H', data, 12)[0] != 9:
        raise ValueError("Not an ESP32-S3 application image")
    if struct.unpack_from('<I', data, 32)[0] != 0xABCD5432 or data[48:80].split(b'\0')[0].decode() != version or data[80:112].split(b'\0')[0] != b'eaglenet_lcd5b':
        raise ValueError("Application descriptor/version mismatch")
    key = serialization.load_pem_private_key(key_path.read_bytes(), password=None)
    if not isinstance(key, rsa.RSAPrivateKey) or key.key_size != 2048:
        raise ValueError("RSA-2048 signing key required")
    digest = hashlib.sha256(data).hexdigest()
    payload = json.dumps(dict(schema=1, board="waveshare-lcd5b-28151", layout="eaglenet-ota-v1", version=version,
                              sequence=sequence, size=len(data), sha256=digest), separators=(',', ':')).encode()
    signature = key.sign(payload, padding.PKCS1v15(), hashes.SHA256())
    envelope = json.dumps(dict(payload=base64.b64encode(payload).decode(), signature=base64.b64encode(signature).decode()), separators=(',', ':')).encode()
    output.mkdir(parents=True, exist_ok=True)
    (output / (digest + '.bin')).write_bytes(data)
    (output / 'manifest.json').write_bytes(envelope)
    (output / ('eaglenet-' + version + '.eagleota')).write_bytes(struct.pack('>I', len(envelope)) + envelope + data)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='command', required=True)
    key = sub.add_parser('keygen'); key.add_argument('--private', type=Path, required=True); key.add_argument('--public', type=Path, required=True)
    pack = sub.add_parser('package')
    for name in ['image', 'key', 'output']: pack.add_argument('--' + name, type=Path, required=True)
    pack.add_argument('--version', required=True); pack.add_argument('--sequence', type=int, required=True)
    a = p.parse_args()
    if a.command == 'keygen': generate(a.private, a.public)
    else: package(a.image, a.version, a.sequence, a.key, a.output)
    print('OTA operation complete; private key was not printed or copied into release output.')

if __name__ == '__main__': main()
