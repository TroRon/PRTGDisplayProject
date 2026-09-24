"""Offline publication preflight; supplements, never replaces, a human review."""
import hashlib
from pathlib import Path
import re
import zipfile

root=Path(__file__).resolve().parents[1]
excluded={'.git','.venv','__pycache__','build','managed_components','.pio'}
errors=[]
def inspect(name,data):
    if re.search(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----\r?\n[A-Za-z0-9+/=]{32,}',data):errors.append(name+': private key')
    if re.search(rb'[A-Za-z]:[\\/]Users[\\/][A-Za-z0-9_.-]+',data,re.I):errors.append(name+': personal build path')
    if re.search(rb'\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,})\b',data):errors.append(name+': credential pattern')
for p in root.rglob('*'):
    if not p.is_file() or excluded.intersection(p.relative_to(root).parts):continue
    if p.name=='.env' or 'secrets' in p.relative_to(root).parts or 'data' in p.relative_to(root).parts:errors.append(str(p.relative_to(root))+': local configuration directory')
    inspect(str(p.relative_to(root)),p.read_bytes())
    if p.suffix=='.zip':
        with zipfile.ZipFile(p) as archive:
            for name in archive.namelist():inspect(p.name+'/'+name,archive.read(name))
for checksum in (root/'Display/releases').glob('*/SHA256.txt'):
    for line in checksum.read_text().splitlines():
        digest,name=line.split('  ',1)
        target=(checksum.parent/name).resolve()
        if not target.is_relative_to(checksum.parent.resolve()) or not target.is_file() or hashlib.sha256(target.read_bytes()).hexdigest()!=digest:errors.append('Release checksum: '+name)
if errors:raise SystemExit('\n'.join(errors))
print('Publication preflight passed: release hashes, key/token patterns, build paths and archive contents. Manually review hostnames, inventory and all staged files before publishing.')
