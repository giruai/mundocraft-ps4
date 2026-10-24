#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Standalone Linux cross-build; never connects to or controls a console.
set -euo pipefail
export REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
export TASK_ROOT="${TASK_ROOT:-$REPO_ROOT/.build/ps4}"
export BUILD_JOBS="${BUILD_JOBS:-2}"
export SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-1792800000}"
export PYTHONDONTWRITEBYTECODE=1
[[ "$BUILD_JOBS" =~ ^[1-9][0-9]*$ ]] || { echo 'Invalid BUILD_JOBS' >&2; exit 2; }
mkdir -p "$TASK_ROOT/logs"
python3 "$REPO_ROOT/scripts/bootstrap-linux.py" "$TASK_ROOT"
export PATH="$TASK_ROOT/host-bin:$TASK_ROOT/host-venv/bin:$TASK_ROOT/host-root/usr/bin:$PATH"
export LD_LIBRARY_PATH="$TASK_ROOT/host-root/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
bash "$REPO_ROOT/scripts/build-deps-linux.sh"
bash "$REPO_ROOT/scripts/build-voxel-deps-linux.sh"
bash "$REPO_ROOT/scripts/build-client-deps-linux.sh"
bash "$REPO_ROOT/client/scripts/build-tls-linux.sh"
bash "$REPO_ROOT/scripts/build-client-linux.sh"
if [[ "${PACKAGE_AFTER_BUILD:-1}" == 1 ]]; then
 bash "$REPO_ROOT/scripts/package-client-linux.sh"
fi
