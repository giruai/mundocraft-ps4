# Test status

## This distribution

**v0.3.19-alpha1 / APP_VER 01.19 — local build and package checks passed; the exact PKG is still awaiting a PS4 hardware test.** This distribution adds attribution patch 0119 and release build and documentation changes. A test of an earlier engine build should not be treated as a test of this version's PKG.

## Local verification for this distribution

The dependencies and client were built for PS4 from their pinned inputs. The TLS link and threaded libc checks passed. The PKG passed checks for format, identity, version, icon, executable, modules, and all 412 files listed in its content manifest. Third-party notices are included in the package. See [BUILD_REPORT.md](BUILD_REPORT.md) for hashes and review scope.

## Earlier private test of engine 01.19

An earlier build of engine 01.19 was tested on PS4 on a private LAN server running Mineclonia 0.124.1 in creative mode. The following were verified:

- Registering an identity; an incorrect password was rejected, then the user could sign in with the correct one.
- Rejoining after restarting the server.
- Walking, looking around, opening inventory, breaking blocks, and placing blocks.
- Pausing and returning normally to the PS4 menu.
- Position and inventory persisting on the server.

The public server list and geolocation lookup also worked in earlier tests. In the game test, the selected public game never appeared.

## Not yet validated

- This distribution's exact PKG on a console.
- Loading and playing a game from the public server list.
- Long survival, crafting, and combat sessions.
- Sound, which is disabled in this build.
- Compatibility with every Luanti server or mod.

These results cover only the cases listed. They do not certify stability, general compatibility, or operation on every network.

## Reporting an issue

Include the product version, `APP_VER`, the screen or action where the issue occurred, and the shortest steps to reproduce it. Do not publish account names, private network addresses, console data, or logs that may contain personal information. Review any log before sharing it and remove such information.

This distribution also rebuilds the PRX module startup object from `crtlib.c` in the pinned SDK input. This recipe and its added notices have not been tested on a console together with the new PKG.
