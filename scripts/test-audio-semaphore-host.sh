#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Exercise the actual patched OpenAL implementation with modeled kernel calls.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/mundocraft-audio-sem-test.XXXXXX")"
trap 'rm -rf "$test_root"' EXIT
python3 - "$repo" "$test_root" <<'PY'
from pathlib import Path
import hashlib, json, sys, tarfile
repo, target = map(Path, sys.argv[1:])
entry = json.loads((repo / 'client/audio-dependencies.lock.json').read_text())['openal']
archive = repo / '.local/downloads' / entry['archive']
assert hashlib.sha256(archive.read_bytes()).hexdigest() == entry['sha256']
with tarfile.open(archive) as tree:
    for relative in ('common/threads.h', 'common/threads.cpp', 'common/opthelpers.h'):
        member = next(m for m in tree.getmembers() if m.name.endswith('/' + relative))
        path = target / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(tree.extractfile(member).read())
(target / 'common/config.h').write_text('/* Isolated semaphore host test. */\n')
PY
patch --fuzz=0 -d "$test_root" -p1 -i "$repo/client/patches/audio/openal-orbis-semaphore.patch"
for standard in c++14 c++17; do
 "${CXX:-clang++}" -std="$standard" -Wall -Wextra -Werror -pthread \
  -fsanitize=address,undefined -g -D__ORBIS__ \
  '-DPS4_SEMAPHORE_NATIVE_HEADER="audio_sem_native_mock.h"' \
  -I "$repo/client/src" -I "$repo/client/tests" -I "$test_root/common" \
  "$test_root/common/threads.cpp" "$repo/client/tests/openal_semaphore_test.cpp" \
  -o "$test_root/semaphore-test"
 "$test_root/semaphore-test"
done
