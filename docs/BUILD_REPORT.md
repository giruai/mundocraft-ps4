# Build report: v0.3.19-alpha1

Historical build record for the published 0.3.19 artifact. Its later exact-package PS4 test completed on 2026-10-24; see [TESTING.md](TESTING.md). The 0.3.20 development candidate has separate build and hardware status.

First private MundoCraft distribution. Cross-build from Linux x86_64 to PS4,
completed on October 24, 2026. No PS4 was accessed to prepare this release.

## Artifact

| Field | Value |
| --- | --- |
| Product | MundoCraft - Luanti 0.3.19-alpha1 |
| Title ID / APP_VER | LUNT00001 / 01.19 |
| PKG size | 104,857,600 bytes |
| PKG SHA-256 | `ba9791c4630d652f6ca38832f59cc45326901fa76d2e44882f1b8522d586fd63` |
| Linked ELF SHA-256 | `59db0c2fd7d670bf3a2050a443650c2132995d833200b481104fa35106dd384b` |
| Eboot SHA-256 | `2c00b7743f31419ed6e434aea1cc3235662e567ed4787a63f27f6cdcf86db6a1` |
| Exact PKG hardware test | Completed on 2026-10-24; one-hour LAN Mineclonia session, inventory/crafting, persisted account/world state after server restart and normal Home exit. See [TESTING.md](TESTING.md). |

## Local checks

- Complete rebuild of graphics, image, font, storage and TLS libraries,
  the client and required modules.
- Pinned input hashes and modified engine hashes verified. All 21 native
  integration files match the previously tested development build 01.19.
  Of 2,413 engine files, only About changes through attribution patch 0119.
- TLS cross-link and the `libc.threaded` check passed.
- PKG format validation and comparison of SFO, icon, eboot, both PRX modules
  and 412 declared content files passed. The package includes 70 license
  and notice files, with 64 third-party legal texts/collections verified
  against their recorded hashes.
- Selected sources, native artifacts and extracted package contents were
  checked for personal paths, known environment values and credential
  patterns. PEM markers in the TLS code were verified as public format-label
  constants without key bodies.

Tools: Ubuntu Clang 18.1.3, CMake 3.28.3, Ninja 1.11.1, GNU Make 4.3,
Python 3.12.3 and Meson 1.12.1. Other Python packages are pinned in
`requirements-build.txt`. Compatible SDK tools were reused; all port libraries
were rebuilt from pinned sources.

## Scope

Hashes identify this build. Bit-for-bit reproducibility across machines and
long-session stability beyond the recorded test are not promised. The later
hardware test covered this exact PKG and its rebuilt module CRT. See [TESTING.md](TESTING.md)
and [DISTRIBUTION_CHANGES.md](DISTRIBUTION_CHANGES.md).

Release manifests relate the repository revision, binary and corresponding
sources. The source bundle preserves original inputs and the exact modified
engine tree. SDK and binary tools are obtained separately.
