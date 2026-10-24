#!/usr/bin/env python3
"""Recover registered sound sources without overwriting unrecorded edits."""
# SPDX-License-Identifier: MIT
from pathlib import Path
import hashlib
import json
import shutil
import sys
import tarfile
import tempfile
import subprocess
import os

root = Path(sys.argv[1]).resolve()
repo = Path(__file__).resolve().parents[1]
sources = root / 'client-audio-sources'
sources.mkdir(exist_ok=True)
def files(path):
    return {p.relative_to(path).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in path.rglob('*') if p.is_file()}
for name, entry in json.loads((repo / 'client/audio-dependencies.lock.json').read_text()).items():
    archive = repo / '.local/downloads' / entry['archive']
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == entry['sha256'], name
    destination = sources / name
    state = sources / (name + '-state.json')
    if destination.exists():
        assert state.is_file(), 'Unregistered audio source: ' + name
        saved = json.loads(state.read_text())
        assert saved['archive_sha256'] == entry['sha256'] and files(destination) == saved['files'], name
    with tempfile.TemporaryDirectory(dir=sources) as tmp:
        with tarfile.open(archive) as tree:
            tree.extractall(tmp, filter='data')
        children = list(Path(tmp).iterdir())
        assert len(children) == 1 and children[0].is_dir()
        pristine = children[0]
        patches = sorted((repo / 'client/patches/audio').glob(name + '-*.patch'))
        for patch in patches:
            subprocess.run(['patch', '--fuzz=0', '-p1', '-i', str(patch)], cwd=pristine, check=True)
        expected = files(pristine)
        destination.mkdir(exist_ok=True)
        for relative, digest in expected.items():
            target = destination / relative
            if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == digest:
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            pending = target.with_name(target.name + '.pending')
            shutil.copy2(pristine / relative, pending)
            os.replace(pending, target)
        assert files(destination) == expected, 'Unexpected sound source files: ' + name
    state.write_text(json.dumps({'archive_sha256': entry['sha256'],
        'patches': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in patches},
        'files': expected}, indent=2) + '\n')
    print('Prepared sound source:', name)
