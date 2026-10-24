#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Execute actual adapter control flow with modeled GUI objects, no console I/O.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/mundocraft-input-test.XXXXXX")"
trap 'rm -rf "$test_root"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror \
 -fsanitize=address,undefined -g -I "$repo/client/tests/input_mocks" \
 "$repo/client/tests/ps4_input_test.cpp" -o "$test_root/input-test"
"$test_root/input-test"
