#!/usr/bin/env bash
set -euo pipefail
p="${TASK_ROOT:-${REPO_ROOT:-$PWD}/.build/ps4}"
SOURCE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$p/host-bin:$PATH"
mkdir -p "$p/client-sources" "$p/client-deps" "$p/logs"
python3 - "$p" "$SOURCE_ROOT" <<'PY'
from pathlib import Path
import sys,json,hashlib,tarfile,zipfile,tempfile,shutil
r=Path(sys.argv[1]);source=Path(sys.argv[2]);lock=json.loads((source/'client/dependencies.lock.json').read_text())
for name,e in lock.items():
 a=source/'.local/downloads'/e['archive'];assert hashlib.sha256(a.read_bytes()).hexdigest()==e['sha256'],name
 dst=r/'client-sources'/name
 with tempfile.TemporaryDirectory(dir=r) as tmp:
  root=Path(tmp)
  if a.suffix=='.zip':
   with zipfile.ZipFile(a) as z:z.extractall(root)
   pristine=next(root.iterdir())
  else:
   with tarfile.open(a) as t:t.extractall(root,filter='data')
   pristine=root
  if not dst.exists():shutil.copytree(pristine,dst)
  expected={p.relative_to(pristine) for p in pristine.rglob('*') if p.is_file()};actual={p.relative_to(dst) for p in dst.rglob('*') if p.is_file()};assert actual==expected,name
  for rel in expected:assert (dst/rel).read_bytes()==(pristine/rel).read_bytes(),f'{name}/{rel}'
 print('Verified client dependency',name)
PY
# A private CMake project compiles the official SQLite amalgamation.
mkdir -p "$p/client-sqlite-project"
cat > "$p/client-sqlite-project/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(sqlite_target C)
add_library(sqlite3 STATIC "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3-orbis.c")
target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_DEFAULT_MEMSTATUS=0 SQLITE_DQS=0 SQLITE_ENABLE_LOCKING_STYLE=1)
install(TARGETS sqlite3 ARCHIVE DESTINATION lib)
install(FILES "${SQLITE_SOURCE}/sqlite3.h" "${SQLITE_SOURCE}/sqlite3ext.h" DESTINATION include)
CMAKE
python3 "$SOURCE_ROOT/client/scripts/prepare-sqlite.py" "$p/client-sources/sqlite/sqlite3.c" "$p/client-sqlite-project/sqlite3-orbis.c" "$SOURCE_ROOT/client/patches/dependencies/sqlite-orbis-flock.patch" > "$p/logs/client-sqlite-patch.log" 2>&1
cmake -S "$p/client-sqlite-project" -B "$p/client-sqlite-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$p/cmake/ps4.cmake" -DSQLITE_SOURCE="$p/client-sources/sqlite" -DCMAKE_INSTALL_PREFIX="$p/client-deps" -DCMAKE_BUILD_TYPE=Release > "$p/logs/client-sqlite-configure.log" 2>&1
cmake --build "$p/client-sqlite-build" -j 2 > "$p/logs/client-sqlite-build.log" 2>&1
cmake --install "$p/client-sqlite-build" > "$p/logs/client-sqlite-install.log" 2>&1
# Optional zstd tracing has unresolved weak hooks unsupported by create-fself.
# Add only this definition; preserve every target/sysroot flag from the toolchain.
printf '%s\n' 'add_compile_definitions(ZSTD_TRACE=0)' > "$p/client-zstd-options.cmake"
cmake -U CMAKE_C_FLAGS -DCMAKE_PROJECT_INCLUDE="$p/client-zstd-options.cmake" -S "$p/client-sources/zstd/build/cmake" -B "$p/client-zstd-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$p/cmake/ps4.cmake" -DCMAKE_INSTALL_PREFIX="$p/client-deps" -DCMAKE_BUILD_TYPE=Release -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_TESTS=OFF -DZSTD_MULTITHREAD_SUPPORT=ON > "$p/logs/client-zstd-configure.log" 2>&1
cmake --build "$p/client-zstd-build" -j 2 > "$p/logs/client-zstd-build.log" 2>&1
cmake --install "$p/client-zstd-build" > "$p/logs/client-zstd-install.log" 2>&1
cmake -S "$p/client-sources/freetype" -B "$p/client-freetype-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$p/cmake/ps4.cmake" -DCMAKE_INSTALL_PREFIX="$p/client-deps" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_BROTLI=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_ZLIB=ON > "$p/logs/client-freetype-configure.log" 2>&1
cmake --build "$p/client-freetype-build" -j 2 > "$p/logs/client-freetype-build.log" 2>&1
cmake --install "$p/client-freetype-build" > "$p/logs/client-freetype-install.log" 2>&1
