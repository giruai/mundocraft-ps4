#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != Linux ]]; then
	echo "audio host smoke test requires Linux; use the existing ps4-dev guest" >&2
	exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOCK="$REPO_ROOT/client/audio-dependencies.lock.json"
DOWNLOADS="$REPO_ROOT/.local/downloads"
BUILD_ROOT="${AUDIO_TEST_BUILD_ROOT:-/tmp/mundocraft-audio-host0320}"
if [[ "$BUILD_ROOT" != /tmp/* ]]; then
	echo "AUDIO_TEST_BUILD_ROOT must stay under /tmp" >&2
	exit 2
fi

mkdir -p "$BUILD_ROOT" "$BUILD_ROOT/sources" "$BUILD_ROOT/build" \
	"$BUILD_ROOT/prefix" "$BUILD_ROOT/logs"
PREFIX="$BUILD_ROOT/prefix"

check_disk_cap() {
	local bytes
	bytes="$(du -sb "$BUILD_ROOT" | awk '{print $1}')"
	if (( bytes > 400000000 )); then
		echo "audio host build tree exceeded the 400 MB cap ($bytes bytes)" >&2
		exit 3
	fi
}

verify_archives() {
	python3 - "$LOCK" "$DOWNLOADS" <<'PY'
import hashlib, json, pathlib, sys

lock_path, downloads = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
lock = json.loads(lock_path.read_text())
expected = {
    "openal": ("1.23.1", "openal-soft-1.23.1.tar.gz"),
    "ogg": ("1.3.6", "libogg-1.3.6.tar.xz"),
    "vorbis": ("1.3.7", "libvorbis-1.3.7.tar.xz"),
}
for name, (version, archive) in expected.items():
    entry = lock.get(name)
    if not entry or entry.get("version") != version or entry.get("archive") != archive:
        raise SystemExit(f"audio dependency lock mismatch for {name}")
    path = downloads / archive
    if not path.is_file():
        raise SystemExit(f"locked source archive missing: {path}")
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != entry.get("size") or digest != entry.get("sha256"):
        raise SystemExit(f"source archive size/hash mismatch: {archive}")
    print(f"verified {name} {version}: {len(data)} bytes sha256={digest}")
PY
}

extract_once() {
	local archive="$1" type="$2" destination="$3" expected="$4"
	if [[ -f "$destination/$expected" ]]; then
		return
	fi
	if [[ -e "$destination" ]]; then
		echo "incomplete source tree already exists: $destination" >&2
		exit 3
	fi
	mkdir -p "$destination"
	if [[ "$type" == gz ]]; then
		tar -xzf "$DOWNLOADS/$archive" -C "$destination" --strip-components=1
	else
		tar -xJf "$DOWNLOADS/$archive" -C "$destination" --strip-components=1
	fi
	if [[ ! -f "$destination/$expected" ]]; then
		echo "archive extraction did not produce expected source: $destination/$expected" >&2
		exit 3
	fi
}

run_logged() {
	local name="$1"; shift
	local log="$BUILD_ROOT/logs/$name.log"
	echo "[audio-host] $name"
	if ! "$@" >"$log" 2>&1; then
		tail -80 "$log" >&2
		return 1
	fi
}

run_build() {
	local name="$1" build_dir="$2"
	local log="$BUILD_ROOT/logs/$name.log" rss="$BUILD_ROOT/logs/$name.rss"
	echo "[audio-host] $name (one build job)"
	if ! python3 - "$log" "$rss" "$build_dir" <<'PY'
import os, subprocess, sys, time
log_path, rss_path, build_dir = sys.argv[1:]
with open(log_path, "wb") as log:
	proc = subprocess.Popen(["cmake", "--build", build_dir, "--parallel", "1"], stdout=log, stderr=subprocess.STDOUT)
	peak = 0
	while proc.poll() is None:
		try:
			rows = []
			for entry in os.listdir("/proc"):
				if not entry.isdigit(): continue
				try:
					with open(f"/proc/{entry}/stat") as f: fields = f.read().split()
					rows.append((int(entry), int(fields[3])))
				except (OSError, ValueError, IndexError): pass
			parents = {proc.pid}
			while True:
				children = {pid for pid, ppid in rows if ppid in parents} - parents
				if not children: break
				parents |= children
			total = 0
			for pid, _ in rows:
				if pid in parents:
					try:
						with open(f"/proc/{pid}/status") as f:
							for line in f:
								if line.startswith("VmRSS:"):
									total += int(line.split()[1]); break
					except (OSError, ValueError, IndexError): pass
			peak = max(peak, total)
		except OSError: pass
		time.sleep(0.05)
	status = proc.wait()
with open(rss_path, "w") as f: f.write(f"PEAK_TREE_RSS_KB={peak}\n")
print(f"PEAK_TREE_RSS_KB={peak}")
sys.exit(status)
PY
	then
		tail -80 "$log" >&2
		return 1
	fi
	local max_rss
	max_rss="$(sed -n 's/PEAK_TREE_RSS_KB=//p' "$rss")"
	if [[ -z "$max_rss" ]] || (( max_rss > 409600 )); then
		echo "audio build RSS exceeded 400 MiB or could not be measured ($max_rss KB)" >&2
		exit 3
	fi
}

verify_archives
extract_once openal-soft-1.23.1.tar.gz gz "$BUILD_ROOT/sources/openal" CMakeLists.txt
extract_once libogg-1.3.6.tar.xz xz "$BUILD_ROOT/sources/ogg" CMakeLists.txt
extract_once libvorbis-1.3.7.tar.xz xz "$BUILD_ROOT/sources/vorbis" lib/CMakeLists.txt
check_disk_cap

run_logged ogg-configure cmake -S "$BUILD_ROOT/sources/ogg" \
	-B "$BUILD_ROOT/build/ogg" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
	-DBUILD_SHARED_LIBS=OFF -DINSTALL_DOCS=OFF \
	-DINSTALL_PKG_CONFIG_MODULE=OFF -DINSTALL_CMAKE_PACKAGE_MODULE=OFF
run_build ogg-build "$BUILD_ROOT/build/ogg"
run_logged ogg-install cmake --install "$BUILD_ROOT/build/ogg"
check_disk_cap

run_logged vorbis-configure cmake -S "$BUILD_ROOT/sources/vorbis" \
	-B "$BUILD_ROOT/build/vorbis" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
	-DBUILD_SHARED_LIBS=OFF -DINSTALL_CMAKE_PACKAGE_MODULE=OFF
run_build vorbis-build "$BUILD_ROOT/build/vorbis"
run_logged vorbis-install cmake --install "$BUILD_ROOT/build/vorbis"
check_disk_cap

run_logged openal-configure cmake -S "$BUILD_ROOT/sources/openal" \
	-B "$BUILD_ROOT/build/openal" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release -DLIBTYPE=STATIC \
	-DCMAKE_INSTALL_PREFIX="$PREFIX" \
	-DALSOFT_DLOPEN=OFF -DALSOFT_UTILS=OFF -DALSOFT_EXAMPLES=OFF \
	-DALSOFT_INSTALL=OFF -DALSOFT_INSTALL_CONFIG=OFF \
	-DALSOFT_INSTALL_HRTF_DATA=OFF -DALSOFT_INSTALL_AMBDEC_PRESETS=OFF \
	-DALSOFT_EMBED_HRTF_DATA=OFF -DALSOFT_UPDATE_BUILD_VERSION=OFF \
	-DALSOFT_BACKEND_PIPEWIRE=OFF -DALSOFT_BACKEND_PULSEAUDIO=OFF \
	-DALSOFT_BACKEND_ALSA=OFF -DALSOFT_BACKEND_OSS=OFF \
	-DALSOFT_BACKEND_SOLARIS=OFF -DALSOFT_BACKEND_SNDIO=OFF \
	-DALSOFT_BACKEND_DSOUND=OFF -DALSOFT_BACKEND_WASAPI=OFF \
	-DALSOFT_BACKEND_WINMM=OFF -DALSOFT_BACKEND_PORTAUDIO=OFF \
	-DALSOFT_BACKEND_JACK=OFF -DALSOFT_BACKEND_COREAUDIO=OFF \
	-DALSOFT_BACKEND_OPENSL=OFF -DALSOFT_BACKEND_OBOE=OFF \
	-DALSOFT_BACKEND_SDL2=OFF -DALSOFT_BACKEND_WAVE=OFF
run_build openal-build "$BUILD_ROOT/build/openal"
check_disk_cap

OPENAL_LIB="$BUILD_ROOT/build/openal/libopenal.a"
OGG_LIB="$PREFIX/lib/libogg.a"
VORBIS_LIB="$PREFIX/lib/libvorbis.a"
VORBISENC_LIB="$PREFIX/lib/libvorbisenc.a"
VORBISFILE_LIB="$PREFIX/lib/libvorbisfile.a"
for library in "$OPENAL_LIB" "$OGG_LIB" "$VORBIS_LIB" "$VORBISENC_LIB" "$VORBISFILE_LIB"; do
	if [[ ! -f "$library" ]]; then
		echo "expected static test library missing: $library" >&2
		find "$BUILD_ROOT" -maxdepth 5 -name '*.a' -print >&2
		exit 3
	fi
done

run_logged test-compile c++ -std=c++17 -O2 -Wall -Wextra \
	-I"$BUILD_ROOT/sources/openal/include" -I"$BUILD_ROOT/build/openal" \
	-I"$PREFIX/include" \
	"$REPO_ROOT/client/tests/audio_loopback_test.cpp" \
	-Wl,--start-group "$OPENAL_LIB" "$VORBISFILE_LIB" "$VORBISENC_LIB" \
	"$VORBIS_LIB" "$OGG_LIB" -Wl,--end-group -pthread -lm \
	-o "$BUILD_ROOT/audio_loopback_test"
run_logged test-run "$BUILD_ROOT/audio_loopback_test"
cat "$BUILD_ROOT/logs/test-run.log"
check_disk_cap

echo "HOST-ONLY LIMIT: covers Vorbis encoding/decoding and OpenAL Soft loopback rendering; does not test SDL2, PS4 AudioOut, device output, or console scheduling."
echo "Retained host build and logs: $BUILD_ROOT"
