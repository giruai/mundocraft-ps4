# Beta release record: v0.3.20-beta1

Prepared on October 24, 2026. Native cross-build and local tests completed.
**The exact candidate subsequently passed bounded PS4 LAN checks on the same date.**
Installed-package readback matched the SHA-256 below. See [TESTING.md](TESTING.md)
for hardware results and their limits.

## Artifact

| Field | Value |
| --- | --- |
| Distribution | MundoCraft - Luanti 0.3.20-beta1 |
| Original tested build | 0.3.20-alpha1 (internal labels retained) |
| Title ID / APP_VER | LUNT00001 / 01.20 |
| PKG | `MundoCraft-PS4-v0.3.20-beta1.pkg` |
| Size | 106,430,464 bytes |
| SHA-256 | `5ebfdbcb0be9e79962b70f75224a671b7426ecf9a7f98de0e451f6c1fe19bb24` |
| Eboot SHA-256 | `b9405d4ddf228b9b33c3244338a5d43f99619d7cb8194a316b1d10636972c9a2` |
| Source baseline | `ace45c57e217a28dd81a56e5b0fe4e2f50b3abb1` plus local candidate changes |
| Distribution status | Promoted to beta after LAN/public checks and maintainer stability acceptance; no rebuild |

The previously published 0.3.19 PKG remains unchanged. Its hardware results
are recorded separately in [TESTING.md](TESTING.md).

## Changes

- Enable OpenAL Soft 1.23.1, libogg 1.3.6 and libvorbis 1.3.7 from checksum-locked
  sources, with SDL2's native PS4 AudioOut backend. Only SDL2 owns the output port.
- Route OpenAL's semaphore through the existing native PS4 semaphore adapter.
  Access its thread-local context out of line to avoid an unsupported undefined
  weak TLS initializer in the SELF import table.
- Make on-screen keyboard Done dispatch Luanti's native edit-box Enter event
  after closing the overlay safely. Cancel restores text without submitting.
- Map Square to secondary click in menus/inventories: take the larger half with
  an empty cursor, or place one item while carrying a stack. Preserve the
  gameplay binding, ordered button edges and held-button masks.
- Include audio configuration, dependency notices and build/test instructions.

## Completed local checks

- Compile the actual input adapter against modeled GUI APIs with ASan/UBSan;
  cover Done/Cancel, callback-driven destruction, menu replacement, input edges,
  left/right chords and held-button quarantine.
- Compile patched OpenAL semaphore code with the production PS4 adapter and
  modeled kernel calls in C++14 and C++17 with ASan/UBSan. Verify token counts,
  blocking wake, interruption, errno preservation, cleanup and error paths.
- Generate an original Ogg/Vorbis tone, decode 22,050 stereo frames and render
  nonzero PCM in two OpenAL loopback contexts, including cleanup/reopening.
  The host test does not exercise SDL2 or physical audio output.
- Cross-build the PS4 audio libraries, native client and SELF, then rebuild
  required runtime modules from their supplied sources. Native inspection
  confirms AudioOut imports and kernel semaphore calls; OpenAL's semaphore
  object has no POSIX `sem_*` calls and the missing weak TLS import is resolved.
- Validate the actual PKG and extract its SFO, launcher icon, executable,
  runtime modules and resources. All 417 manifest entries match their build
  inputs; the audio configuration and added license texts are included.
- Check Python/shell syntax and whitespace. Check extracted packaged files
  for personal host paths, common GitHub token patterns and PEM private-key
  bodies. mbedTLS's static PEM format-label constants contain no key bodies.
  Generic guest build paths can remain in executable metadata; this is not a
  guarantee that every possible secret pattern was scanned.

See [BUILDING.md](BUILDING.md) for reproducible local test commands. Source and
artifact hashes are also retained in the private build evidence, outside the
published source tree.

## Completed hardware acceptance

Approximately 17 minutes 25 seconds of application observations covered audible
sound (confirmed by the user), mute/unmute, search Done/Cancel, half-stack and
single-item operations, two-cell crafting, password reconnect after a server
restart, persisted player/inventory state and normal Home exit. The installed
package was read back and matched the hash above. No new binary or corrective
patch was introduced during that test.

The same package subsequently passed the public Mineclonia functional session
recorded in [TESTING.md](TESTING.md). The maintainer reported successful stability
tests and requested beta publication. No duration or detailed measurements were
supplied for that acceptance. No new instrumented FPS/audio/memory-growth
benchmark is claimed. The beta asset is byte-identical to the tested candidate;
only the distribution filename and release metadata change. Any further console
use requires fresh user authorization.
