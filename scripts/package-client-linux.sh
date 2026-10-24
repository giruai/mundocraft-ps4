#!/usr/bin/env bash
# Build only this port's required PRX modules and create a local PS4 PKG.
set -euo pipefail

task_root="${TASK_ROOT:-${REPO_ROOT:-$PWD}/.build/ps4}"
repo_root="${REPO_ROOT:-$PWD}"
build_root="$task_root/client-build"
package_root="$task_root/client-package"
log_dir="$task_root/logs"
modules="$task_root/runtime-source/OpenOrbis/PS4Toolchain/src/modules"
eboot="$build_root/bin/eboot.bin"

test -f "$repo_root/client/assets/default.conf"
test -f "$repo_root/client/assets/launch.ini"
test -s "$eboot" || { echo "Client eboot not linked: $eboot" >&2; exit 1; }
python3 - "$eboot" <<'PY'
from pathlib import Path
import sys
path = Path(sys.argv[1])
assert path.read_bytes()[:4] == bytes.fromhex("4f153d1d"), f"Invalid SELF: {path}"
PY

version="$(sed -n 's/^APP_VERSION:STRING=//p' "$build_root/CMakeCache.txt")"
[[ "$version" =~ ^[0-9]{2}\.[0-9]{2}$ ]] || {
  echo "Invalid compiled APP_VERSION" >&2; exit 1;
}
mkdir -p "$package_root" "$log_dir"
python3 "$repo_root/scripts/verify-inputs.py" "$task_root" \
  "$repo_root/dependencies.lock.json" \
  > "$log_dir/client-package-input-verify.log" 2>&1
python3 "$repo_root/scripts/prepare-client-assets.py" "$task_root" "$repo_root" \
  > "$log_dir/client-package-assets.log" 2>&1

export PATH="$task_root/host-bin:$task_root/host-venv/bin:$PATH"
export OO_PS4_TOOLCHAIN="$task_root/orbis-sdk-v1/sdk"
export PS4_PKGTOOL_OPENSSL_LIB="$task_root/host-root/usr/lib/x86_64-linux-gnu"
# Rebuild the module startup object from its supplied source, rather than
# redistributing an opaque prebuilt CRT object without a known source recipe.
mkdir -p "$task_root/runtime-crt"
clang --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -O0 -g0 \
  -isysroot "$OO_PS4_TOOLCHAIN" -isystem "$OO_PS4_TOOLCHAIN/include" \
  -ffile-prefix-map="$OO_PS4_TOOLCHAIN"=/opt/openorbis \
  -c "$repo_root/vendor/openorbis-runtime/crt/crtlib.c" -o "$task_root/runtime-crt/crtlib.o" \
  > "$log_dir/client-crtlib-build.log" 2>&1
cat > "$task_root/runtime-crt/module-source-crt.mk" <<'MAKE'
LDFLAGS := $(subst $(TOOLCHAIN)/lib/crtlib.o,$(MUNDO_CRTLIB),$(LDFLAGS))
MAKE
for module in libc libSceFios2; do
  # Compile the corresponding module sources shipped with this distribution.
  cp "$repo_root/vendor/openorbis-runtime/modules/$module/Makefile" "$modules/$module/Makefile"
  cp "$repo_root/vendor/openorbis-runtime/modules/$module/$module/lib.c" "$modules/$module/$module/lib.c"
  make -B -C "$modules/$module" -f Makefile \
    -f "$task_root/runtime-crt/module-source-crt.mk" \
    MUNDO_CRTLIB="$task_root/runtime-crt/crtlib.o" -j 2 \
    > "$log_dir/client-$module-build.log" 2>&1
  python3 - "$modules/$module/$module.prx" <<'PY'
from pathlib import Path
import sys
path = Path(sys.argv[1])
assert path.read_bytes()[:4] == bytes.fromhex("4f153d1d"), f"Invalid SELF: {path}"
PY
done

cp "$repo_root/client/assets/mundocraft-icon0.png" "$package_root/icon0.png"
assets=()
while IFS= read -r -d '' asset; do
  relative="${asset#"$task_root/client-assets/"}"
  assets+=(--extra "$asset:$relative")
done < <(find "$task_root/client-assets" -type f -print0 | sort -z)

bash "$task_root/orbis-sdk-v1/orbis-compat/scripts/ps4/make-pkg.sh" \
  "${assets[@]}" \
  --eboot "$eboot" --out-dir "$package_root" \
  --sdk "$OO_PS4_TOOLCHAIN" --title-id LUNT00001 --title "MundoCraft - Luanti" \
  --version "$version" --content-label LUANTICLIENT \
  --icon "$package_root/icon0.png" \
  --extra "$modules/libc/libc.prx:sce_module/libc.prx" \
  --extra "$modules/libSceFios2/libSceFios2.prx:sce_module/libSceFios2.prx" \
  > "$log_dir/client-package.log" 2>&1

python3 "$repo_root/scripts/verify-package.py" "$task_root" \
  client-package client-build > "$log_dir/client-package-verify.log" 2>&1
sha256sum "$package_root/"*.pkg
