# Build MundoCraft for PS4

This guide builds the source code and creates a local PKG. A cross-build does not prove that the package works on a console. The recorded exact 0.3.20 build passed local checks and a bounded PS4 LAN test. A newly rebuilt binary needs its own validation. See [TESTING.md](TESTING.md).

## Requirements

Use Linux x86_64, such as Ubuntu 24.04, with:

- Python 3.12 or later with `venv` support.
- LLVM 18: Clang, Clang++, LLD, and `llvm-ar`, `llvm-ranlib`, `llvm-nm`, `llvm-objcopy`, `llvm-strip`, and `llvm-objdump`.
- CMake, Ninja, GNU Make, Git, `patch`, `pkg-config`, `dpkg-deb`, Bison, Flex, M4, `glslangValidator`, and SPIR-V Tools.
- Internet access to download public sources pinned by hash.
- The `orbis-sdk-v1` PS4 kit and compatible OpenOrbis module sources. The script fetches the files pinned in `dependencies.lock.json`; the kit and tools are not redistributed in this repository or source archive.

The script creates an isolated Python environment and installs the versions pinned in [`requirements-build.txt`](../requirements-build.txt). It also extracts the required helper tools into the build directory without installing them globally.

## Build

From the repository root, run:

```sh
./build.sh
```

The script downloads and verifies the locked files, prepares a local prefix, and runs the build and packaging stages. The first run needs disk space and an internet connection to obtain the dependencies.

Optional settings:

```sh
TASK_ROOT="$PWD/.build/ps4" BUILD_JOBS=2 ./build.sh
```

`TASK_ROOT` sets where extracted sources, tools, libraries, logs, and results are stored. `BUILD_JOBS` limits parallel jobs. Defaults are `.build/ps4` and two jobs. To build without packaging at the end, run `PACKAGE_AFTER_BUILD=0 ./build.sh`.

If you already have a compatible environment, `ORBIS_SDK_BUNDLE_DIR`, `HOST_PYTHON_ENV`, and `HOST_TOOLS_ROOT` can point to existing paths when the corresponding path under `TASK_ROOT` does not exist. Bootstrap fetches and verifies the locked files before using those paths; these settings do not skip the SDK download. The provided paths must match the recipe's required versions. The script does not adapt them or verify their provenance beyond its built-in checks.

## Stages and outputs

![Build stages](../assets/build-flow.svg)

The recipe builds Mesa and SDL2 with Orbis audio; JPEG and PNG; SQLite, zstd, and FreeType; curl and mbedTLS; static OpenAL Soft, Ogg and Vorbis; then the Luanti client and package. Inputs for each stage are pinned in the root, `voxel/`, and `client/` `dependencies.lock.json` files, as well as `client/tls-dependencies.lock.json` and `client/audio-dependencies.lock.json`. PS4 engine changes are recorded in `client/patches/`.

The audio stage uses only the port's SDL2 static archive through an explicit CMake package, with dynamic library loading and non-PS4 device backends disabled. It does not link the build machine's audio libraries. `client/assets/alsoft.ini` selects 48 kHz stereo S16 playback and the SDL2 backend. The final client imports the system AudioOut API; no proprietary module binary is bundled.

Packaging rebuilds the PRX modules and their CRT from the pinned source; it does not modify a shared SDK.

By default, the PKG is saved in `.build/ps4/client-package/`. Logs and verification manifests are saved under `.build/ps4/logs/`. These directories are excluded from Git. Review logs and artifacts from a machine before publishing them.

The distribution version is `0.3.20-beta1`; the PS4 `APP_VER` is `01.20`. They are separate identifiers. The beta promotes the previously tested binary without rebuilding it, so internal alpha build labels remain intentionally unchanged in the corresponding sources. The recipe sets `SOURCE_DATE_EPOCH` and uses prefix maps to replace known local paths in build metadata. This supports reproducibility and reduces machine-specific paths, but does not guarantee byte-for-byte matches across machines.

Local input and sound checks can run without a console. The input harness compiles the actual adapter against modeled GUI APIs under AddressSanitizer/UndefinedBehaviorSanitizer. The Linux audio smoke test verifies locked sources, generates/decodes its own Ogg/Vorbis tone and renders it using OpenAL loopback:

```sh
./scripts/test-input-host.sh
bash ./scripts/test-audio-semaphore-host.sh
bash ./scripts/test-audio-host.sh
```

The sound tests need the locked archives already downloaded. The semaphore test compiles the patched OpenAL implementation against the production PS4 adapter and modeled kernel calls with ASan/UBSan in C++14 and C++17. The Linux loopback test uses one build job and writes a bounded temporary prefix. These tests do not prove physical PS4 audio, rendering, controller behavior or Mineclonia acceptance on hardware.

## Third-party sources and SDK

Each published release supplies its corresponding `MundoCraft-PS4-vVERSION-sources.tar.gz`, manifests and hashes. The beta includes the exact modified engine and locked audio dependency sources used for the tested 0.3.20 binary. To verify published downloads, compare file SHA-256 values against that release's manifests and `SHA256SUMS`.

The archive also includes `vendor/patched-luanti/`, a snapshot of the modified engine, and `SOURCE_MANIFEST.json`, which records hashes of its inputs. Bootstrap reuses files from `vendor/archives/`; engine sources are reconstructed from upstream and the patches to verify correspondence. Do not edit only the snapshot: record changes as an additional patch in `client/patches/`. To replace or modify LGPL libraries, apply the relevant changes there, then rebuild and relink with `./build.sh`.

The PS4 SDK and packaging tools are not included. `dependencies.lock.json` points to the pinned kit download. Obtain and use it under its own terms. Do not copy an SDK from another installation into the repository or source archive.

For installation, see [INSTALLING.md](INSTALLING.md). For the scope of the checks, see [TESTING.md](TESTING.md).
