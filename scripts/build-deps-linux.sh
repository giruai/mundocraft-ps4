#!/usr/bin/env bash
# Run inside the existing ps4-dev VM. All writes stay in this port's root.
set -euo pipefail
TASK_ROOT="${TASK_ROOT:-${REPO_ROOT:-$PWD}/.build/ps4}"
SOURCE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$TASK_ROOT/host-bin:$TASK_ROOT/host-venv/bin:$TASK_ROOT/host-root/usr/bin:$PATH"
if [[ -d "$TASK_ROOT/host-root/usr/share/bison" ]]; then
 export BISON_PKGDATADIR="$TASK_ROOT/host-root/usr/share/bison"
fi
if [[ -x "$TASK_ROOT/host-root/usr/bin/m4" ]]; then
 export M4="$TASK_ROOT/host-root/usr/bin/m4"
fi
export LD_LIBRARY_PATH="$TASK_ROOT/host-root/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
SDK="$TASK_ROOT/orbis-sdk-v1"
export OO_PS4_TOOLCHAIN="$SDK/sdk"
export ORBIS_COMPAT_DIR="$TASK_ROOT/sources/compat"
export ORBIS_KIT_DIR="$SDK/orbis-compat"
export ORBIS_MESA_SRC="$TASK_ROOT/sources/mesa"
export ORBIS_MESA_BUILD="$TASK_ROOT/mesa-build"
mkdir -p "$TASK_ROOT/logs" "$TASK_ROOT/cross/lib/pkgconfig"
python3 "$SOURCE_ROOT/scripts/verify-inputs.py" "$TASK_ROOT" "$SOURCE_ROOT/dependencies.lock.json" \
 > "$TASK_ROOT/logs/input-verify.log" 2>&1
if [[ ! -f "$ORBIS_COMPAT_DIR/build/liborbis-compat.a" ]]; then
 bash "$ORBIS_COMPAT_DIR/build.sh" > "$TASK_ROOT/logs/compat-build.log" 2>&1
fi
python3 - "$TASK_ROOT" <<'PY'
import pathlib,sys
r=pathlib.Path(sys.argv[1]); sdk=r/'orbis-sdk-v1'
p=sdk/'toolchain/orbis.ini.in'
s=p.read_text()
values={'__BUNDLE__':str(sdk),'@OO_PS4_TOOLCHAIN@':str(sdk/'sdk'),
 '@ORBIS_CROSS@':str(r/'cross'),'@ORBIS_COMPAT@':str(r/'sources/compat'),
 '@ORBIS_KIT@':str(sdk/'orbis-compat'),'@ORBIS_CC@':'clang',
 '@ORBIS_CXX@':'clang++','@ORBIS_AR@':'llvm-ar','@ORBIS_STRIP@':'llvm-strip'}
for a,b in values.items(): s=s.replace(a,b)
# Bundle template bakes its paired overlay path; replace with the same revision rebuilt here.
s=s.replace(str(sdk/'orbis-compat'),str(r/'sources/compat'))
s=s.replace(str(r/'sources/compat/cmake/orbis-tls.ld'),str(sdk/'toolchain/orbis-tls.ld'))
# Strip builder paths from Mesa's assertions and macros as well as CMake targets.
flags = [f'-ffile-prefix-map={r}=/build/mundocraft',
         f'-fmacro-prefix-map={r}=/build/mundocraft',
         f'-fdebug-prefix-map={r}=/build/mundocraft', '-g0']
import re
for key in ('c_args', 'cpp_args'):
 s,n = re.subn(r'(?m)^'+key+r'\s*=\s*\[', key+' = ['+', '.join(repr(x) for x in flags)+', ', s)
 assert n == 1, f'Missing or ambiguous cross-file {key}'
(r/'cross/orbis.ini').write_text(s)
PY
cd "$ORBIS_MESA_SRC"
meson subprojects download zlib > "$TASK_ROOT/logs/zlib-fetch.log" 2>&1
opts=()
[[ -f "$ORBIS_MESA_BUILD/meson-private/coredata.dat" ]] && opts+=(--reconfigure)
env PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR="$TASK_ROOT/cross/lib/pkgconfig" \
 meson setup "$ORBIS_MESA_BUILD" "${opts[@]}" --cross-file "$TASK_ROOT/cross/orbis.ini" \
 -Ddefault_library=static -Dwrap_mode=nodownload -Dvulkan-drivers=amd -Dgallium-drivers=zink \
 -Dopengl=true -Dgles1=enabled -Dgles2=enabled -Degl=enabled -Dglx=disabled \
 -Dgbm=disabled -Dglvnd=disabled -Dvideo-codecs= -Dllvm=disabled -Dxmlconfig=disabled \
 -Dbuildtype=release -Dplatforms=orbis -Dzlib=enabled -Dshader-cache=enabled \
 -Dforce_fallback_for=zlib -Dzstd=disabled -Dvalgrind=disabled -Dlmsensors=disabled \
 -Dlibunwind=disabled -Dradv-build-id=1a59cc147b54e3970814b37313e2f27357672e9d \
 > "$TASK_ROOT/logs/mesa-configure.log" 2>&1
grep -q 'Checking for function "sysconf" : YES' "$ORBIS_MESA_BUILD/meson-logs/meson-log.txt" || {
 echo 'Mesa did not link the PS4 compatibility sysconf implementation.' >&2; exit 1;
}
ninja -C "$ORBIS_MESA_BUILD" -j "${BUILD_JOBS:-2}" > "$TASK_ROOT/logs/mesa-build.log" 2>&1
mkdir -p "$TASK_ROOT/cmake"
cp "$SOURCE_ROOT/cmake/ps4.cmake" "$TASK_ROOT/cmake/"
cmake -S "$TASK_ROOT/sources/SDL2/orbis" -B "$TASK_ROOT/sdl2-build" -G Ninja \
 -DCMAKE_TOOLCHAIN_FILE="$TASK_ROOT/cmake/ps4.cmake" \
 -DORBIS_COMPAT_DIR="$ORBIS_COMPAT_DIR" -DORBIS_MESA_SRC="$ORBIS_MESA_SRC" \
 -DORBIS_MESA_BUILD="$ORBIS_MESA_BUILD" \
 -DSDL_ORBIS_AUDIO=ON -DSDL_ORBIS_JOYSTICK=ON -DSDL_ORBIS_VIDEO=ON \
 > "$TASK_ROOT/logs/sdl-configure.log" 2>&1
cmake --build "$TASK_ROOT/sdl2-build" -j "${BUILD_JOBS:-2}" > "$TASK_ROOT/logs/sdl-build.log" 2>&1
