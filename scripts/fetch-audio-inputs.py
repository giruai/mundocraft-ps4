#!/usr/bin/env python3
"""Fetch only checksum-locked public sound sources; never contact a console."""
# SPDX-License-Identifier: MIT
from pathlib import Path
import hashlib
import json
import urllib.request

repo = Path(__file__).resolve().parents[1]
cache = repo / '.local/downloads'
cache.mkdir(parents=True, exist_ok=True)
for name, entry in json.loads((repo / 'client/audio-dependencies.lock.json').read_text()).items():
    path = cache / entry['archive']
    if not path.is_file():
        with urllib.request.urlopen(entry['url'], timeout=60) as response:
            data = response.read(32 * 1024 * 1024 + 1)
        if len(data) > 32 * 1024 * 1024 or hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise SystemExit('Invalid locked audio download: ' + name)
        temporary = path.with_name(path.name + '.partial')
        temporary.write_bytes(data)
        temporary.replace(path)
    assert hashlib.sha256(path.read_bytes()).hexdigest() == entry['sha256'], name
    print('Verified sound source:', name, entry['version'])
