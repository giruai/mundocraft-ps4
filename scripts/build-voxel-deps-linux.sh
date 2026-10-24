#!/usr/bin/env bash
set -euo pipefail
TASK_ROOT="${TASK_ROOT:-${REPO_ROOT:-$PWD}/.build/ps4}"
SOURCE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$SOURCE_ROOT/scripts/prepare-image-deps.py" "$TASK_ROOT"
export PATH="$TASK_ROOT/host-bin:$PATH"
p="$TASK_ROOT"
mkdir -p "$p/logs" "$p/voxel-deps"
cmake -S "$p/voxel-sources/libjpeg-turbo" -B "$p/voxel-jpeg-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$p/cmake/ps4.cmake" -DCMAKE_INSTALL_PREFIX="$p/voxel-deps" -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_SIMD=OFF -DWITH_TURBOJPEG=OFF -DCMAKE_BUILD_TYPE=Release > "$p/logs/voxel-jpeg-configure.log" 2>&1
cmake --build "$p/voxel-jpeg-build" -j "${BUILD_JOBS:-2}" > "$p/logs/voxel-jpeg-build.log" 2>&1
cmake --install "$p/voxel-jpeg-build" > "$p/logs/voxel-jpeg-install.log" 2>&1
cmake -S "$p/voxel-sources/libpng" -B "$p/voxel-png-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$p/cmake/ps4.cmake" -DCMAKE_INSTALL_PREFIX="$p/voxel-deps" -DPNG_SHARED=OFF -DPNG_STATIC=ON -DPNG_TESTS=OFF -DPNG_TOOLS=OFF -DPNG_HARDWARE_OPTIMIZATIONS=OFF -DZLIB_INCLUDE_DIR="$p/sources/mesa/subprojects/zlib-1.3.1" -DZLIB_LIBRARY="$p/mesa-build/subprojects/zlib-1.3.1/libz.a" -DCMAKE_BUILD_TYPE=Release > "$p/logs/voxel-png-configure.log" 2>&1
cmake --build "$p/voxel-png-build" -j "${BUILD_JOBS:-2}" > "$p/logs/voxel-png-build.log" 2>&1
cmake --install "$p/voxel-png-build" > "$p/logs/voxel-png-install.log" 2>&1
