#!/usr/bin/env bash
set -euo pipefail
TASK_ROOT="${TASK_ROOT:-${REPO_ROOT:-$PWD}/.build/ps4}"
SOURCE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$TASK_ROOT/host-bin:$PATH"
mkdir -p "$TASK_ROOT/logs"
python3 "$SOURCE_ROOT/scripts/prepare-client-linux.py" "$TASK_ROOT" > "$TASK_ROOT/logs/client-source-verify.log" 2>&1
cmake -S "$TASK_ROOT/client-sources/luanti" -B "$TASK_ROOT/client-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TASK_ROOT/cmake/ps4.cmake" -DPORT_ROOT="$TASK_ROOT" -DPS4_PORT_SOURCE="$SOURCE_ROOT" -DCMAKE_BUILD_TYPE=Release -DAPP_VERSION="${APP_VERSION:-01.19}" > "$TASK_ROOT/logs/client-configure.log" 2>&1
cmake --build "$TASK_ROOT/client-build" --target "${BUILD_TARGET:-luanti}" -j "${BUILD_JOBS:-2}" > "$TASK_ROOT/logs/client-build.log" 2>&1
