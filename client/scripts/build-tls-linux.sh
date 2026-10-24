#!/usr/bin/env bash
# Build PS4 static TLS libraries in this port's private guest tree.
set -euo pipefail

TASK_ROOT="${TASK_ROOT:-${REPO_ROOT:-$PWD}/.build/ps4}"
export PATH="$TASK_ROOT/host-bin:$PATH"
export PYTHONDONTWRITEBYTECODE=1
SOURCE_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# Mirror only this recipe's own inputs into the private guest scaffold.
if [[ "$SOURCE_ROOT" != "$TASK_ROOT" ]]; then
 mkdir -p "$TASK_ROOT/client/scripts" "$TASK_ROOT/client/cmake"
 for rel in client/scripts/fetch-tls.py client/tls-dependencies.lock.json client/cmake/mbedtls-orbis-config.h; do
  if ! cmp -s "$SOURCE_ROOT/$rel" "$TASK_ROOT/$rel"; then cp "$SOURCE_ROOT/$rel" "$TASK_ROOT/$rel"; fi
 done
fi
source_root="$TASK_ROOT/.local/client-tls/sources"
build_root="$TASK_ROOT/client-tls-build"
prefix="$TASK_ROOT/client-deps"
log_dir="$build_root/logs"

python3 "$TASK_ROOT/client/scripts/fetch-tls.py" --root "$TASK_ROOT"
mkdir -p "$log_dir" "$prefix"

cmake -S "$source_root/mbedtls-3.6.7" -B "$build_root/mbedtls" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TASK_ROOT/cmake/ps4.cmake" \
  -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DGEN_FILES=OFF \
  -DUSE_STATIC_MBEDTLS_LIBRARY=ON -DUSE_SHARED_MBEDTLS_LIBRARY=OFF \
  -DMBEDTLS_USER_CONFIG_FILE="$TASK_ROOT/client/cmake/mbedtls-orbis-config.h" \
  > "$log_dir/mbedtls-configure.log" 2>&1
cmake --build "$build_root/mbedtls" -j "${BUILD_JOBS:-2}" \
  > "$log_dir/mbedtls-build.log" 2>&1
cmake --install "$build_root/mbedtls" \
  > "$log_dir/mbedtls-install.log" 2>&1

# OpenOrbis exposes IP_BIND_ADDRESS_NO_PORT but has no SOL_IP. The option is
# merely a Linux port-binding optimization; omit it when the socket level is
# unavailable. Preserve the exact generated source change for audit.
python3 - "$source_root/curl-8.23.0/lib/cf-socket.c" \
  "$TASK_ROOT/.local/client-tls/patches/curl-orbis-bind-no-port.patch" <<'PY'
import difflib
from pathlib import Path
import sys

source, patch = map(Path, sys.argv[1:])
original = source.read_text()
old = "#ifdef IP_BIND_ADDRESS_NO_PORT\n"
new = "#if defined(IP_BIND_ADDRESS_NO_PORT) && defined(SOL_IP)\n"
if original.count(old) == 2:
    updated = original.replace(old, new)
    patch.parent.mkdir(parents=True, exist_ok=True)
    patch.write_text("".join(difflib.unified_diff(
        original.splitlines(keepends=True), updated.splitlines(keepends=True),
        fromfile="a/lib/cf-socket.c", tofile="b/lib/cf-socket.c")))
    source.write_text(updated)
elif original.count(new) != 2 or original.count(old) != 0:
    raise SystemExit("unexpected curl cf-socket.c; refusing to patch")
PY

# The client supplies an IPv4 getaddrinfo() bridge; curl's SDK probe cannot
# link it in isolation and otherwise falls back to the SDK gethostbyname().
# OpenOrbis eventfd() always returns ENOSYS; Luanti polls active transfers.
cmake -S "$source_root/curl-8.23.0" -B "$build_root/curl" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TASK_ROOT/cmake/ps4.cmake" \
  -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DBUILD_CURL_EXE=OFF \
  -DBUILD_TESTING=OFF -DCURL_ENABLE_SSL=ON -DCURL_USE_MBEDTLS=ON \
  -DCURL_USE_OPENSSL=OFF -DCURL_DEFAULT_SSL_BACKEND=mbedtls \
  -DMBEDTLS_USE_STATIC_LIBS=ON -DMBEDTLS_INCLUDE_DIR="$prefix/include" \
  -DMBEDTLS_LIBRARY="$prefix/lib/libmbedtls.a" \
  -DMBEDX509_LIBRARY="$prefix/lib/libmbedx509.a" \
  -DMBEDCRYPTO_LIBRARY="$prefix/lib/libmbedcrypto.a" \
  -DCURL_CA_BUNDLE=/app0/client/cacert.pem -DCURL_CA_PATH=none \
  -DCURL_ZLIB=ON \
  -DZLIB_INCLUDE_DIR="$TASK_ROOT/sources/mesa/subprojects/zlib-1.3.1" \
  -DZLIB_LIBRARY="$TASK_ROOT/mesa-build/subprojects/zlib-1.3.1/libz.a" \
  -DCURL_BROTLI=OFF -DCURL_ZSTD=OFF -DUSE_NGHTTP2=OFF \
  -DUSE_LIBIDN2=OFF -DCURL_USE_LIBPSL=OFF -DENABLE_ARES=OFF \
  -DENABLE_THREADED_RESOLVER=ON -DENABLE_IPV6=OFF \
  -DHAVE_GETADDRINFO=1 -DHAVE_GETADDRINFO_THREADSAFE=1 \
  -DCURL_DISABLE_SOCKETPAIR=ON \
  -DHAVE_STROPTS_H=0 \
  -DCURL_DISABLE_DICT=ON -DCURL_DISABLE_FILE=ON -DCURL_DISABLE_FTP=ON \
  -DCURL_DISABLE_GOPHER=ON -DCURL_DISABLE_IMAP=ON -DCURL_DISABLE_LDAP=ON \
  -DCURL_DISABLE_LDAPS=ON -DCURL_DISABLE_MQTT=ON -DCURL_DISABLE_POP3=ON \
  -DCURL_DISABLE_RTSP=ON -DCURL_DISABLE_SMTP=ON -DCURL_DISABLE_TELNET=ON \
  -DCURL_DISABLE_TFTP=ON -DCURL_DISABLE_WEBSOCKETS=ON \
  -DCURL_DISABLE_DOH=ON \
  > "$log_dir/curl-configure.log" 2>&1
cmake --build "$build_root/curl" -j "${BUILD_JOBS:-2}" \
  > "$log_dir/curl-build.log" 2>&1
cmake --install "$build_root/curl" \
  > "$log_dir/curl-install.log" 2>&1

install -D -m 0644 "$TASK_ROOT/.local/client-tls/cacert.pem" \
  "$prefix/share/luanti/client/cacert.pem"

for archive in libmbedcrypto.a libmbedx509.a libmbedtls.a libcurl.a; do
  test -s "$prefix/lib/$archive" || {
    echo "Missing PS4 archive: $prefix/lib/$archive" >&2
    exit 1
  }
done

# Link a small PS4 ELF against every archive and the native random import.
# It is a host-side link check only; the application supplies this same
# hardware-poll contract and must be tested on the console separately.
probe_source="$build_root/link-probe-src"
mkdir -p "$probe_source"
cat > "$probe_source/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.16)
project(luanti_tls_link_probe C)
add_executable(luanti_tls_link_probe main.c)
target_include_directories(luanti_tls_link_probe PRIVATE "$prefix/include")
target_link_libraries(luanti_tls_link_probe PRIVATE
  -Wl,--start-group
  "$prefix/lib/libcurl.a"
  "$prefix/lib/libmbedtls.a"
  "$prefix/lib/libmbedx509.a"
  "$prefix/lib/libmbedcrypto.a"
  "$TASK_ROOT/mesa-build/subprojects/zlib-1.3.1/libz.a"
  -Wl,--end-group
  SceRandom SceNet)
EOF
cat > "$probe_source/main.c" <<'EOF'
#include <stdint.h>
#include <stddef.h>
#include <curl/curl.h>
#include <mbedtls/entropy.h>

/* Verified SceRandom ABI; the shipped client owns its production adapter. */
extern int32_t sceRandomGetRandomNumber(void *buffer, size_t size);

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;
    *olen = 0;
    while (*olen < len) {
        size_t chunk = len - *olen;
        if (chunk > 64) chunk = 64;
        if (sceRandomGetRandomNumber(output + *olen, chunk) != 0)
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        *olen += chunk;
    }
    return 0;
}

int main(void)
{
    CURL *handle;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 1;
    handle = curl_easy_init();
    if (!handle) return 2;
    curl_easy_setopt(handle, CURLOPT_URL, "https://servers.luanti.org/");
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    curl_global_cleanup();
    return 0;
}
EOF
cmake -S "$probe_source" -B "$build_root/link-probe" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TASK_ROOT/cmake/ps4.cmake" \
  -DCMAKE_BUILD_TYPE=Release > "$log_dir/link-probe-configure.log" 2>&1
cmake --build "$build_root/link-probe" \
  > "$log_dir/link-probe-build.log" 2>&1
test -s "$build_root/link-probe/luanti_tls_link_probe"

echo "PS4 TLS archives: $prefix/lib/{libcurl,libmbedtls,libmbedx509,libmbedcrypto}.a"
echo "Package CA bundle at /app0/client/cacert.pem; source: $prefix/share/luanti/client/cacert.pem"
echo "PS4 cross-link passed: $build_root/link-probe/luanti_tls_link_probe"
