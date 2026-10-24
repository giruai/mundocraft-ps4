#!/usr/bin/env python3
"""Check locked downloads and every original extracted source file, without console access."""
import hashlib
import json
import pathlib
import sys
import tarfile

def digest(stream):
    h = hashlib.sha256()
    while chunk := stream.read(1024 * 1024):
        h.update(chunk)
    return h.digest()

port = pathlib.Path(sys.argv[1])
lock = json.loads(pathlib.Path(sys.argv[2]).read_text())
for name in ('mesa', 'sdl2', 'compat', 'sdk_bundle', 'openorbis_module_sources', 'host_libssl'):
    entry = lock[name]
    archive = port / 'downloads' / entry.get('archive', 'orbis-sdk-v1.tar.gz' if name == 'sdk_bundle' else 'toolchain-llvm-18.tar.gz')
    with archive.open('rb') as stream:
        archive_hash = digest(stream).hex()
    if archive_hash != entry['sha256']:
        raise SystemExit(f'FAIL {name}: archive checksum')
    if name in ('sdk_bundle', 'host_libssl'):
        continue
    source = port / 'sources' / {'sdl2': 'SDL2'}.get(name, name)
    module_prefix = 'OpenOrbis/PS4Toolchain/src/modules/'
    if name == 'openorbis_module_sources':
        source = port / 'runtime-source'
    count = 0
    expected = set()
    with tarfile.open(archive) as tree:
        for member in tree:
            if name == 'openorbis_module_sources' and not member.name.startswith(module_prefix):
                continue
            # The release archive also contains prebuilt modules/build outputs. Rebuilt bytes
            # deliberately replace those; compare the actual source and project inputs only.
            if name == 'openorbis_module_sources' and (member.name.endswith('.prx') or '/x64/' in member.name):
                continue
            if member.issym():
                actual = source / member.name
                if not actual.is_symlink() or actual.readlink().as_posix() != member.linkname:
                    raise SystemExit(f'FAIL {name}: changed symlink {member.name}')
                expected.add(member.name)
            if not member.isfile():
                continue
            expected.add(member.name)
            rel = pathlib.PurePosixPath(member.name)
            if rel.is_absolute() or '..' in rel.parts:
                raise SystemExit('Unsafe archive member')
            original = tree.extractfile(member)
            actual = source / member.name
            if not actual.is_file():
                raise SystemExit(f'FAIL {name}: missing source {member.name}')
            with actual.open('rb') as stream:
                matches = digest(stream) == digest(original)
            if not matches:
                raise SystemExit(f'FAIL {name}: modified source {member.name}')
            count += 1
    scan = source / module_prefix if name == 'openorbis_module_sources' else source
    for actual in scan.rglob('*'):
        if not actual.is_file() and not actual.is_symlink():
            continue
        rel = actual.relative_to(source).as_posix()
        if rel in expected:
            continue
        generated = (name == 'compat' and rel.startswith('build/')) or (
            name == 'mesa' and (rel == 'subprojects/.wraplock' or rel.startswith(('subprojects/packagecache/', 'subprojects/zlib-1.3.1/'))))
        generated = generated or ('__pycache__' in actual.parts and actual.suffix == '.pyc')
        if name == 'openorbis_module_sources':
            generated = '/x64/' in rel or rel.endswith('.prx')
        if not generated:
            raise SystemExit(f'FAIL {name}: unexpected source file {rel}')
    print(f'OK {name}: {entry.get("commit", "locked archive")}, {count} original source files')
print('OK source inputs; SDK extracted contents checked separately by bundle verify.sh')
