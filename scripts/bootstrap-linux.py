#!/usr/bin/env python3
"""Prepare only an isolated Linux build prefix from checksum-locked public inputs."""
# SPDX-License-Identifier: MIT
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile

repo = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
if sys.platform != 'linux' or os.uname().machine != 'x86_64':
    raise SystemExit('Use Linux x86_64 with Python 3.12+ and LLVM 18')
if sys.version_info < (3, 12):
    raise SystemExit('Python 3.12+ is required for safe archive extraction')
root.mkdir(parents=True, exist_ok=True)
cache = repo / '.local/downloads'
cache.mkdir(parents=True, exist_ok=True)
vendor = repo / 'vendor/archives'
if vendor.is_dir():
    for archive in vendor.iterdir():
        if archive.is_file() and not (cache / archive.name).exists():
            (cache / archive.name).symlink_to(archive.resolve())
for script in ('fetch-inputs.py', 'fetch-client-inputs.py', 'fetch-voxel-inputs.py'):
    subprocess.run([sys.executable, str(repo / 'scripts' / script)], check=True)
downloads = root / 'downloads'
downloads.mkdir(exist_ok=True)
for path in cache.iterdir():
    if path.is_file() and not (downloads / path.name).exists():
        (downloads / path.name).symlink_to(path.resolve())

def extract(archive, destination, members=None):
    with tarfile.open(archive) as tf:
        tf.extractall(destination, members=members(tf) if members else None, filter='data')

sdk = root / 'orbis-sdk-v1'
if not sdk.exists():
    external = os.environ.get('ORBIS_SDK_BUNDLE_DIR')
    if external:
        sdk.symlink_to(Path(external).resolve(), target_is_directory=True)
    else:
        extract(downloads / 'orbis-sdk-v1.tar.gz', root)
for name, directory in (('mesa', 'mesa'), ('sdl2', 'SDL2'), ('compat', 'compat')):
    target = root / 'sources' / directory
    if not target.exists():
        target.mkdir(parents=True)
        extract(downloads / f'{name}-source.tar', target)
if vendor.is_dir():
    packagecache = root / 'sources/mesa/subprojects/packagecache'
    packagecache.mkdir(exist_ok=True)
    for pattern in ('zlib-1.3.1.tar.gz', 'zlib_1.3.1-1_patch.zip'):
        source = vendor / pattern
        if source.is_file() and not (packagecache / pattern).exists():
            (packagecache / pattern).symlink_to(source.resolve())
    tls = root / '.local/client-tls/downloads'
    tls.mkdir(parents=True, exist_ok=True)
    for entry in json.loads((repo / 'client/tls-dependencies.lock.json').read_text()).values():
        source = vendor / entry['archive']
        if source.is_file() and not (tls / source.name).exists():
            (tls / source.name).symlink_to(source.resolve())
modules = root / 'runtime-source/OpenOrbis/PS4Toolchain/src/modules'
if not modules.exists():
    destination = root / 'runtime-source'
    destination.mkdir(exist_ok=True)
    prefix = 'OpenOrbis/PS4Toolchain/src/modules/'
    extract(downloads / 'toolchain-llvm-18.tar.gz', destination,
            lambda tf: [m for m in tf.getmembers() if m.name.startswith(prefix)
                        and not m.name.endswith('.prx') and '/x64/' not in m.name])

host_bin = root / 'host-bin'
host_bin.mkdir(exist_ok=True)
for name in ('clang', 'clang++', 'ld.lld', 'llvm-ar', 'llvm-ranlib',
             'llvm-nm', 'llvm-objcopy', 'llvm-strip', 'llvm-objdump'):
    tool = shutil.which(name + '-18')
    if not tool:
        raise SystemExit(f'Missing host prerequisite: {name}-18')
    target = host_bin / name
    if not target.exists():
        target.symlink_to(tool)
venv = root / 'host-venv'
if not venv.exists():
    external = os.environ.get('HOST_PYTHON_ENV')
    if external:
        venv.symlink_to(Path(external).resolve(), target_is_directory=True)
    else:
        subprocess.run([sys.executable, '-m', 'venv', str(venv)], check=True)
        subprocess.run([str(venv / 'bin/pip'), 'install', '-r',
                        str(repo / 'requirements-build.txt')], check=True)
host = root / 'host-root'
if not host.exists():
    external = os.environ.get('HOST_TOOLS_ROOT')
    if external:
        host.symlink_to(Path(external).resolve(), target_is_directory=True)
    else:
        host.mkdir()
        subprocess.run(['dpkg-deb', '-x', str(downloads / 'libssl1.1.deb'), str(host)], check=True)
(root / 'cmake').mkdir(exist_ok=True)
shutil.copyfile(repo / 'cmake/ps4.cmake', root / 'cmake/ps4.cmake')
environment = os.environ.copy()
environment['PATH'] = str(host_bin) + ':' + environment['PATH']
subprocess.run(['bash', str(sdk / 'verify.sh'), str(sdk)], check=True, env=environment,
               stdout=(root / 'logs/sdk-verify.log').open('w'), stderr=subprocess.STDOUT)
print('Locked isolated build inputs prepared; no console operations')
