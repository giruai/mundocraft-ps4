#!/usr/bin/env python3
"""Recover only the pinned image libraries needed by the client."""
# SPDX-License-Identifier: MIT
from pathlib import Path
import hashlib
import json
import sys
import tarfile

repo = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1])
lock = json.loads((repo / 'voxel/dependencies.lock.json').read_text())
for name in ('libjpeg-turbo', 'libpng'):
    entry = lock[name]
    archive = repo / '.local/downloads' / entry['archive']
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == entry['sha256']
    target = root / 'voxel-sources' / name
    if not target.exists():
        target.mkdir(parents=True)
        with tarfile.open(archive) as tf:
            tf.extractall(target, filter='data')
    with tarfile.open(archive) as tf:
        for member in tf:
            if member.isfile():
                assert (target / member.name).read_bytes() == tf.extractfile(member).read()
print('Pinned image sources verified')
