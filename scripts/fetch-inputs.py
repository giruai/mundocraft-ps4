#!/usr/bin/env python3
"""Recover exact public inputs into .local/downloads. No build, deployment, or global installs."""
import hashlib
import json
import pathlib
import subprocess
import urllib.request

def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        while chunk := stream.read(1024 * 1024):
            h.update(chunk)
    return h.hexdigest()

root = pathlib.Path(__file__).resolve().parents[1]
lock = json.loads((root / 'dependencies.lock.json').read_text())
downloads = root / '.local/downloads'
downloads.mkdir(parents=True, exist_ok=True)
for name, entry in lock.items():
    path = downloads / entry.get('archive', 'orbis-sdk-v1.tar.gz' if name == 'sdk_bundle' else 'toolchain-llvm-18.tar.gz')
    if not path.exists():
        partial = path.with_suffix(path.suffix + '.partial')
        if 'commit' in entry:
            repo = root / '.local/git' / name
            repo.parent.mkdir(parents=True, exist_ok=True)
            if not repo.exists():
                subprocess.run(['git', 'init', '--bare', str(repo)], check=True)
            subprocess.run(['git', '-C', str(repo), 'fetch', '--depth', '1', entry['url'], entry['commit']], check=True)
            subprocess.run(['git', '-C', str(repo), 'archive', '--format=tar', entry['commit'], '-o', str(partial)], check=True)
        else:
            with urllib.request.urlopen(entry['url']) as response, partial.open('wb') as output:
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
        if sha256(partial) != entry['sha256']:
            raise SystemExit(f'FAIL {name}: downloaded bytes differ from lock; not promoted')
        partial.rename(path)
    if sha256(path) != entry['sha256']:
        raise SystemExit(f'FAIL {name}: existing bytes differ from lock')
    print(f'OK {name}: {path.name}')
