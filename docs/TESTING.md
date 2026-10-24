# Test status

## Beta 0.3.20-beta1 / APP_VER 01.20

This beta promotes the exact tested 0.3.20-alpha1 binary with no rebuild or byte changes. Internal alpha build labels remain. On 2026-10-24 the maintainer confirmed that their stability tests concluded successfully and authorized beta publication. Their test duration, workload and detailed measurements were not provided; this user-reported acceptance is separate from the recorded checks below.

This candidate adds native audio, inventory search submission and Square secondary click. Its exact PKG was installed and tested on PS4 on **2026-10-24**, with full installed-package readback matching SHA-256 `5ebfdbcb0be9e79962b70f75224a671b7426ecf9a7f98de0e451f6c1fe19bb24` (106,430,464 bytes).

The local artifact, hashes and completed checks are recorded in [BUILD_REPORT_0320.md](BUILD_REPORT_0320.md).

Local checks cover:

- The actual input adapter compiled into a modeled GUI harness under AddressSanitizer/UndefinedBehaviorSanitizer: ordered left/right clicks and chord masks, held input, menu replacement, Done/Cancel, callback-driven GUI destruction and queued-input suppression.
- A generated Ogg/Vorbis tone encoded and decoded using the pinned codec libraries, then rendered into nonzero PCM twice through OpenAL Soft loopback, including context cleanup/reopening. No speakers or console are involved.
- The patched OpenAL semaphore implementation using the production native adapter with modeled kernel calls under ASan/UBSan: counts, blocking wake, interruption, errno preservation, handle cleanup, range rejection and fatal native errors, in C++14 and C++17.
- Cross-compilation of SDL2's Orbis audio backend, static OpenAL Soft, Ogg and Vorbis. Native client and package evidence is recorded separately in the candidate build report.

Local checks alone do not establish physical-console behavior. The subsequent PS4 test used the existing private Mineclonia 0.124.1 creative LAN world, a password-protected account and gamepad commands over the paired Remote Play session. Application observations spanned approximately **17 minutes 25 seconds**, including menus and two connections; this was not a sustained performance benchmark.

| PS4 check | Recorded result |
| --- | --- |
| Sound | Native OpenAL/SDL2 AudioOut initialized at 48 kHz stereo, game sound resources played and the user confirmed hearing audio. Captured audio changed around digging; mute produced eight zero-peak samples and unmute restored signal. Audio also returned after reconnecting. |
| Search | Entered `stone` with the on-screen keyboard; Done applied the filter immediately. Cancel closed the keyboard and retained the previous filter. |
| Inventory | Square split 64 cyan blocks into 32/32. While carrying 32, Square placed one item and left 31; server inventory data confirmed the counts. |
| Two-cell crafting | Placed one oak plank in each of two vertical crafting-table cells, returned the other 61 planks and collected four sticks. Server craft action and inventory data agreed. |
| Login and persistence | Existing name/password worked twice, including after a clean server restart. The complete logical player/inventory state was identical before/after. All four SQLite databases passed integrity checks and stopped-world backups were verified recoverable. |
| Exit and restoration | Normal Exit to OS returned to Home with `CLIENT_END result=0 cleanup=DONE`.  |

No native ERROR, fatal/out-of-memory marker, timeout or unintended disconnect was detected in this session. The server had two setup errors before its first successful start because its game search path was initially missing; supplying the existing game path resolved that local fixture issue without changing the PKG. The known Mesa optional-layer probe message also appeared at startup.

The last sampled audio counter reported 194,936 output grains, zero output errors and 104 late grains. Late-grain counters are diagnostic timing observations, not an audible-dropout or latency measurement. No new FPS, frame-time, long-session audio or memory-growth measurement was made. These bounded checks do not establish that every prior memory issue is fixed.

### Public-server functional session, 2026-10-24

The same exact 0.3.20 PKG also connected from Join Game to **German-Mineclonia-Server** over the Internet. MineCruft, the first candidate, refused new players; the client displayed that rejection and recovered to the menu. The alternative accepted an exclusive password-protected test account, downloaded the missing game resources and rendered its public world. This session did not use the private LAN server.

Movement, camera, server rules, inventory, audio, death/respawn and the 2x2 crafting interface were exercised. Four crafting cells were filled individually with cobbled deepslate; the polished-deepslate result was collected. Secondary click was used to split another stack and place one item. After a normal leave, the same account/password rejoined at the saved location, with the crafted item and split stacks in the same slots. Exact stack numerals were not independently legible in the 640x360 Remote Play captures; numerical counts are inferred from the requested actions and recipe. This is visual client-rejoin evidence, not a public-server database or restart test.

The character was killed once by another player and later drowned during exploration. Those were game events; no native crash, fatal/out-of-memory marker or unintended connection timeout was detected. Ordinary construction/mining in an unprotected area was not verified, and no protected spawn structures were modified. The last audio sample counter recorded 384,104 grains, zero output errors and 205 late grains; no latency/dropout or FPS/memory benchmark was made.

The test used the existing unified login/registration setting temporarily. Code review identified a separate usability concern: OSK Done sends Enter, which can submit the double-password registration form before both fields are complete. The separate dialog has not been fixed or hardware-validated. Normal Exit to OS returned to Home. The maintainer later reported successful stability testing, as noted above.

## Published 0.3.19-alpha1: exact-package hardware test

On 2026-10-24 the installed application was read back in full and matched the published PKG: 104,857,600 bytes, SHA-256 `ba9791c4630d652f6ca38832f59cc45326901fa76d2e44882f1b8522d586fd63`.

The private LAN fixture used Luanti 5.17.0 and Mineclonia 0.124.1, creative mode, damage/PvP disabled and a password-protected test account. Results:

- A continuous connected observation lasted **60 minutes 18 seconds**, including stationary observation, menus/inventory and short movement exercises. It was not an hour of uninterrupted active play.
- No crash, main-session disconnect, native/server ERROR, out-of-memory or fatal marker was detected. There were three normal joins/leaves across the test and four server pathfinding-quota warnings.
- Movement, camera, digging/placing, whole-stack transfer, armor equipment and the 3x3 crafting table worked. One oak button was crafted from 64 planks; 63 planks remained and the result was collected.
- The same account/password rejoined after a clean server restart. New blocks rendered again. Normalized position/orientation, inventory definitions and all item strings by logical inventory name/slot matched exactly. All four SQLite databases passed integrity checks before/after; stopped-world backups were verified recoverable.
- Normal Exit to OS returned to Home.
- The HUD had **44 timestamped readings**, spanning 47-58 smoothed FPS, usually 58, with jitter spikes up to 491.2%. This is smoothed loop cadence, not raw per-frame/presentation FPS; no percentiles or 1% lows were measured. Remote Play encoder cadence was not used as game FPS.

That version had sound disabled. Its keyboard Done did not apply inventory search, and controller secondary click was absent. These findings motivated the candidate. An earlier private build also rejected a wrong password; that distinct result is not claimed as a new wrong-password test of the published PKG.

## Remaining limits

Basic public-server gameplay and rejoining were observed on one Mineclonia server. Full public survival progression, public building/mining outside spawn protection, combat behavior, more complex recipes, every mod/server combination and offline local-world startup remain unvalidated. The separate double-password registration dialog needs further work for the on-screen keyboard. The engine retains embedded-server code; this PKG bundles no game. A bounded crash-free session does not prove previous memory faults fixed or measure a leak.

## Reporting an issue

Include product version, APP_VER, the action/screen and concise reproduction steps. Do not publish account names, private network addresses, console data or unreviewed logs. Keep raw diagnostics private and remove personal information before sharing evidence.
