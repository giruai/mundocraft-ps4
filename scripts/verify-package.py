#!/usr/bin/env python3
"""Validate the actual local PKG, extract its SFO/content and compare executable/modules."""
import hashlib
import json
import os
import pathlib
import re
import struct
import subprocess
import sys
import uuid

r = pathlib.Path(sys.argv[1])
package_dir = sys.argv[2] if len(sys.argv)>2 else 'package'
build_dir = sys.argv[3] if len(sys.argv)>3 else 'probe-build'
packages = [p for p in (r / package_dir).glob('*.pkg') if not p.is_symlink()]
assert len(packages) == 1, 'Expected exactly one regular package'
pkg = packages[0]
def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        while data := stream.read(1048576):
            h.update(data)
    return h.hexdigest()

env = dict(os.environ, DOTNET_SYSTEM_GLOBALIZATION_INVARIANT='1',
           LD_LIBRARY_PATH=str(r / 'host-root/usr/lib/x86_64-linux-gnu'))
tool = str(r / 'orbis-sdk-v1/sdk/bin/linux/PkgTool.Core')
def run(*args):
    return subprocess.run([tool, *map(str, args)], env=env, check=True,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout

validation = run('pkg_validate', pkg)
(r / 'logs/package-validate.log').write_text(validation)
assert '[OK]' in validation and '[FAIL]' not in validation and '[ERROR]' not in validation, validation
entries = run('pkg_listentries', pkg)
matches = [re.split(r'\s+', line.strip()) for line in entries.splitlines() if line.rstrip().endswith(' PARAM_SFO')]
assert len(matches) == 1, 'Missing or ambiguous PARAM_SFO'
# PkgTool's argument named entry_id is actually the table index printed in column four.
index = int(matches[0][3])
out = r / 'package-verification' / str(uuid.uuid4())
out.mkdir(parents=True)
run('pkg_extractentry', pkg, index, out / 'param.sfo')
data = (out / 'param.sfo').read_bytes()
magic, version, keys, values, count = struct.unpack_from('<5I', data)
assert magic == 0x46535000
sfo = {}
for i in range(count):
    key_off, kind, length, maximum, value_off = struct.unpack_from('<HHIII', data, 20 + i * 16)
    start = keys + key_off
    key = data[start:data.index(0, start)].decode()
    value = data[values + value_off:values + value_off + length]
    sfo[key] = value.rstrip(b'\0').decode() if kind == 0x204 else int.from_bytes(value, 'little')
is_client = package_dir == 'client-package'
cache = (r / build_dir / 'CMakeCache.txt').read_text()
version = re.search(r'^APP_VERSION:STRING=(.+)$', cache, re.M).group(1)
client_title = 'MundoCraft - Luanti' if version >= '01.17' else 'Luanti PS4'
assert (sfo['TITLE_ID'], sfo['TITLE']) == (('LUNT00001', client_title) if is_client else ('LUGR00001', 'Luanti Graphics Probe'))
assert sfo['APP_VER'] == version
icon_metadata = None
if is_client and version >= '01.17':
    icon_entries = [re.split(r'\s+', line.strip()) for line in entries.splitlines() if line.rstrip().endswith(' ICON0_PNG')]
    assert len(icon_entries) == 1, 'Missing or ambiguous launcher icon'
    icon = out / 'icon0.png'
    run('pkg_extractentry', pkg, int(icon_entries[0][3]), icon)
    expected_icon = pathlib.Path(__file__).resolve().parents[1] / 'client/assets/mundocraft-icon0.png'
    assert sha(icon) == sha(expected_icon), 'Wrong packaged launcher icon'
    raw = icon.read_bytes()
    assert raw[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>IIBB', raw[16:26]) == (512, 512, 8, 2), 'Expected opaque 512px RGB8 launcher PNG'
    icon_metadata = {'sha256': sha(icon), 'size': icon.stat().st_size, 'width': 512, 'height': 512, 'format': 'RGB8 PNG'}
run('pkg_extract', pkg, out / 'content')
files = {}
for rel, expected in {
    'eboot.bin': r / build_dir / ('bin/eboot.bin' if is_client else 'eboot.bin'),
    'sce_module/libc.prx': r / 'runtime-source/OpenOrbis/PS4Toolchain/src/modules/libc/libc.prx',
    'sce_module/libSceFios2.prx': r / 'runtime-source/OpenOrbis/PS4Toolchain/src/modules/libSceFios2/libSceFios2.prx',
}.items():
    p = out / 'content/uroot' / rel
    assert p.read_bytes()[:4] == bytes.fromhex('4f153d1d'), f'Not SELF: {rel}'
    assert sha(p) == sha(expected), f'PKG content differs from build: {rel}'
    files[rel] = {'sha256': sha(p), 'size': p.stat().st_size}
if package_dir == 'voxel-package':
    expected_assets = json.loads((r / 'logs/voxel-assets.json').read_text())
    actual_assets = {str(p.relative_to(out / 'content/uroot/voxel')) for p in (out / 'content/uroot/voxel').rglob('*') if p.is_file()}
    assert actual_assets == set(expected_assets), 'Packaged assets differ'
    for rel, expected in expected_assets.items():
        p = out / 'content/uroot/voxel' / rel
        assert sha(p) == expected['sha256'] and p.stat().st_size == expected['size'], rel
        files['voxel/'+rel] = expected
if is_client:
    expected_assets = json.loads((r / 'logs/client-package-assets.json').read_text())
    root = out / 'content/uroot'
    for rel, expected in expected_assets.items():
        p = root / rel
        assert p.is_file() and sha(p) == expected['sha256'] and p.stat().st_size == expected['size'], rel
        files[rel] = expected
    actual_assets = {str(p.relative_to(root)) for base in ('luanti', 'client') for p in (root / base).rglob('*') if p.is_file()}
    assert actual_assets == set(expected_assets), 'Client packaged asset set differs'
manifest = {'title': sfo['TITLE'], 'title_id': sfo['TITLE_ID'], 'app_version': version,
            'package': pkg.name, 'size': pkg.stat().st_size, 'sha256': sha(pkg),
            'package_validation': 'passed', 'extracted_contents': files,
            'console_transferred': False, 'console_installed': False, 'console_tested': False}
if icon_metadata is not None:
    manifest['launcher_icon'] = icon_metadata
(r / package_dir / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps(manifest, indent=2))
