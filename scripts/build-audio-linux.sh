#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# PS4 target libraries only; writes stay in the caller's isolated port prefix.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
p="${TASK_ROOT:-$repo/.build/ps4}"
export PATH="$p/host-bin:$PATH"
export REPO_ROOT="$repo"
jobs="${BUILD_JOBS:-2}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || exit 2
mkdir -p "$p/logs" "$p/client-audio-build" "$p/client-audio-deps"
python3 "$repo/scripts/fetch-audio-inputs.py"
python3 "$repo/scripts/prepare-audio-linux.py" "$p"

# Reconfigure this port's existing SDL library, preserving its platform inputs.
cmake -S "$p/sources/SDL2/orbis" -B "$p/sdl2-build" -G Ninja \
 -DCMAKE_TOOLCHAIN_FILE="$p/cmake/ps4.cmake" \
 -DORBIS_COMPAT_DIR="$p/sources/compat" -DORBIS_MESA_SRC="$p/sources/mesa" \
 -DORBIS_MESA_BUILD="$p/mesa-build" \
 -DSDL_ORBIS_AUDIO=ON -DSDL_ORBIS_JOYSTICK=ON -DSDL_ORBIS_VIDEO=ON \
 > "$p/logs/audio-sdl-configure.log" 2>&1
cmake --build "$p/sdl2-build" -j "$jobs" > "$p/logs/audio-sdl-build.log" 2>&1

prefix="$p/client-audio-deps"
common=(-G Ninja "-DCMAKE_TOOLCHAIN_FILE=$p/cmake/ps4.cmake" \
 "-DCMAKE_INSTALL_PREFIX=$prefix" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF)
cmake -S "$p/client-audio-sources/ogg" -B "$p/client-audio-build/ogg" "${common[@]}" \
 -DINSTALL_DOCS=OFF -DINSTALL_PKG_CONFIG_MODULE=ON \
 > "$p/logs/audio-ogg-configure.log" 2>&1
cmake --build "$p/client-audio-build/ogg" -j "$jobs" > "$p/logs/audio-ogg-build.log" 2>&1
cmake --install "$p/client-audio-build/ogg" > "$p/logs/audio-ogg-install.log" 2>&1
cmake -S "$p/client-audio-sources/vorbis" -B "$p/client-audio-build/vorbis" "${common[@]}" \
 "-DOGG_INCLUDE_DIR=$prefix/include" "-DOGG_LIBRARY=$prefix/lib/libogg.a" \
 > "$p/logs/audio-vorbis-configure.log" 2>&1
cmake --build "$p/client-audio-build/vorbis" -j "$jobs" > "$p/logs/audio-vorbis-build.log" 2>&1
cmake --install "$p/client-audio-build/vorbis" > "$p/logs/audio-vorbis-install.log" 2>&1
backends=()
for backend in PIPEWIRE PULSEAUDIO ALSA OSS SOLARIS SNDIO WINMM DSOUND WASAPI JACK COREAUDIO OBOE OPENSL PORTAUDIO WAVE; do
 backends+=("-DALSOFT_BACKEND_$backend=OFF")
done
# OpenAL replaces CMAKE_MODULE_PATH itself. Supply a config package pointing
# explicitly to the already-built PS4 archive, never a Linux SDL installation.
mkdir -p "$p/client-audio-sdl2"
cat > "$p/client-audio-sdl2/SDL2Config.cmake" <<CMAKE
include("$repo/client/cmake/FindSDL2.cmake")
CMAKE
cmake -S "$p/client-audio-sources/openal" -B "$p/client-audio-build/openal" "${common[@]}" \
 "-DSDL2_DIR=$p/client-audio-sdl2" "-DPORT_ROOT=$p" \
 "-DCMAKE_PROJECT_INCLUDE=$repo/client/cmake/audio-options.cmake" "-DPS4_PORT_SOURCE=$repo" \
 -DLIBTYPE=STATIC -DALSOFT_BACKEND_SDL2=ON -DALSOFT_REQUIRE_SDL2=ON \
 -DALSOFT_DLOPEN=OFF -DALSOFT_UTILS=OFF -DALSOFT_EXAMPLES=OFF \
 -DALSOFT_NO_CONFIG_UTIL=ON -DALSOFT_RTKIT=OFF -DALSOFT_EAX=OFF \
 -DALSOFT_INSTALL_CONFIG=OFF -DALSOFT_INSTALL_HRTF_DATA=OFF \
 -DALSOFT_INSTALL_AMBDEC_PRESETS=OFF -DALSOFT_EMBED_HRTF_DATA=OFF \
 -DALSOFT_UPDATE_BUILD_VERSION=OFF "${backends[@]}" \
 > "$p/logs/audio-openal-configure.log" 2>&1
cmake --build "$p/client-audio-build/openal" -j "$jobs" > "$p/logs/audio-openal-build.log" 2>&1
cmake --install "$p/client-audio-build/openal" > "$p/logs/audio-openal-install.log" 2>&1
echo 'PS4 SDL2/OpenAL/Ogg/Vorbis libraries built; hardware sound remains untested.'
