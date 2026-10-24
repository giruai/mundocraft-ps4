#!/usr/bin/env python3
"""Archive tracked sources and checksum-pinned dependency sources, without SDK binaries."""
# SPDX-License-Identifier: MIT
from pathlib import Path
import argparse
import gzip
import hashlib
import io
import json
import re
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('task_root', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
repo = Path(__file__).resolve().parents[1]
root = args.task_root.resolve()
version = (repo / 'VERSION').read_text().strip()
epoch = 1792800000
prefix = 'MundoCraft-PS4-v' + version
entries = {}
subprocess.run(['git', '-C', str(repo), 'diff', '--quiet', 'HEAD', '--'], check=True)

def register(name, path, expected=None):
    assert name not in entries and not Path(name).is_absolute() and '..' not in Path(name).parts
    digest = hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()
    if expected:
        assert digest == expected, 'Source input differs from lock: ' + name
    entries[name] = (path, digest)

tracked = subprocess.check_output(['git', '-C', str(repo), 'ls-files', '-z']).split(b'\0')
for raw in tracked:
    if not raw:
        continue
    name = raw.decode()
    path = repo / name
    assert path.is_file() and not path.is_symlink(), 'Only regular tracked files are exported'
    register(name, path)

for lock in ('dependencies.lock.json', 'voxel/dependencies.lock.json',
             'client/dependencies.lock.json', 'client/tls-dependencies.lock.json',
             'client/audio-dependencies.lock.json'):
    for key, entry in json.loads((repo / lock).read_text()).items():
        # SDK/toolchain and host executable inputs are fetched independently.
        if lock == 'dependencies.lock.json' and key not in ('mesa', 'sdl2', 'compat'):
            continue
        name = entry['archive']
        candidates = (root / 'downloads' / name, root / '.local/client-tls/downloads' / name,
                      repo / '.local/downloads' / name)
        source = next((p for p in candidates if p.is_file()), None)
        assert source is not None, 'Missing source archive: ' + name
        register('vendor/archives/' + name, source, entry['sha256'])

wrap = (root / 'sources/mesa/subprojects/zlib.wrap').read_text()
for name_key, hash_key in (('source_filename', 'source_hash'), ('patch_filename', 'patch_hash')):
    name = re.search(r'^' + name_key + r'\s*=\s*(.+)$', wrap, re.M).group(1).strip()
    expected = re.search(r'^' + hash_key + r'\s*=\s*(.+)$', wrap, re.M).group(1).strip()
    register('vendor/archives/' + name, root / 'sources/mesa/subprojects/packagecache' / name, expected)

state = json.loads((root / 'client-source-state.json').read_text())
engine = root / 'client-sources/luanti'
for name, expected in state['files'].items():
    register('vendor/patched-luanti/' + name, engine / name, expected)

manifest = {
    'format': 1,
    'version': version,
    'repository_commit': subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'HEAD'], text=True).strip(),
    'description': 'Tracked port recipe, pristine locked dependency sources and exact patched Luanti tree. SDK/tools are excluded.',
    'files': {name: {'sha256': digest, 'bytes': path.stat().st_size} for name, (path, digest) in sorted(entries.items())},
}
manifest_bytes = (json.dumps(manifest, indent=2, ensure_ascii=False) + '\n').encode()
args.output.parent.mkdir(parents=True, exist_ok=True)
pending = args.output.with_name(args.output.name + '.pending')
with pending.open('wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=epoch, compresslevel=6) as compressed, tarfile.open(fileobj=compressed, mode='w|', format=tarfile.USTAR_FORMAT) as archive:
    for name, (path, digest) in sorted(entries.items()):
        info = tarfile.TarInfo(prefix + '/' + name)
        info.size = path.stat().st_size
        info.mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
        info.mtime = epoch
        with path.open('rb') as source:
            archive.addfile(info, source)
    info = tarfile.TarInfo(prefix + '/SOURCE_MANIFEST.json')
    info.size = len(manifest_bytes)
    info.mode = 0o644
    info.mtime = epoch
    archive.addfile(info, io.BytesIO(manifest_bytes))
pending.replace(args.output)
print(json.dumps({'file': args.output.name, 'entries': len(entries) + 1, 'bytes': args.output.stat().st_size,
                  'sha256': hashlib.file_digest(args.output.open('rb'), 'sha256').hexdigest()}))
