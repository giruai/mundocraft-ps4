# MundoCraft for PS4

**English** · [Español](README.es.md)

![MundoCraft for PS4: illustrated voxel landscape](assets/banner.svg)

![Beta release](assets/beta-badge.svg) ![PS4 homebrew](assets/platform-badge.svg) ![Luanti 5.17.0](assets/engine-badge.svg) ![Component licenses](assets/license-badge.svg)

**Beta v0.3.20-beta1** · **APP_VER 01.20**

MundoCraft brings the [Luanti](https://www.luanti.org/) voxel game engine to
PlayStation 4. This is an experimental, unofficial, modified port. This release
adds sound, inventory search confirmation and controller stack splitting.
**The beta distributes the exact 0.3.20 PKG tested on PS4 in LAN and on a public
Mineclonia server. The maintainer also reports successful stability testing.**
This is a release-stage promotion with no binary changes: internal alpha labels
remain in the tested build. See [the build record](docs/BUILD_REPORT_0320.md).

Luanti is free software developed by its community under LGPL-2.1-or-later.
Corresponding modified source, third-party notices and build instructions
accompany this release.

<p align="center"><img src="client/assets/mundocraft-icon0.png" width="120" alt="MundoCraft icon: a cheerful block island and tree"></p>

## Play and build

- [Download v0.3.20-beta1](https://github.com/giruai/mundocraft-ps4/releases/tag/v0.3.20-beta1)
- [Install the PKG](docs/INSTALLING.md)
- [Controls, accounts and saved progress](docs/USAGE.md)
- [Build from source](docs/BUILDING.md)
- [Test results and limitations](docs/TESTING.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Licenses and acknowledgements](THIRD_PARTY_NOTICES.md)

Verify the PKG against the `SHA256SUMS` attached to the same release. The release
also contains the corresponding source bundle and `BUILDINFO.json`.

## How games work

Connect to a compatible Luanti server. Its game content and resources are
downloaded from that server; **Mineclonia is not bundled with the PKG**. Each
server manages its own accounts, world and saved progress. Rejoin the same
server with the same player name and password to resume your account. A LAN
server must remain running while you play.

MundoCraft is not Minecraft and does not support Minecraft Java or Bedrock
servers, worlds or accounts. It is not affiliated with Mojang or Microsoft.

## Current status

The previous 0.3.19-alpha1 package was tested with Mineclonia 0.124.1 in a
creative LAN world on 2026-10-24. A continuous observation lasted 60 minutes
18 seconds without a detected crash or disconnect. Whole-stack movement, armor,
placing/digging blocks and one simple crafting recipe worked. Position,
orientation and inventory matched after a server restart and password login.
The internal smoothed FPS display ranged from 47 to 58, usually 58, with jitter
spikes; this is not a guarantee of stable 60 FPS or survival-game performance.

The 0.3.20 beta enables the audio chain and adds Done-to-submit and Square
secondary click in inventory. Local tests exercise the actual input adapter and
an Ogg/Vorbis/OpenAL loopback pipeline. Its exact PKG also passed PS4 checks for
audible sound, mute/unmute, inventory search, half-stack/single-item operations,
two-cell crafting, persistence after server restart and normal Home exit.
The session covered about 17 minutes of application runtime; it was not a long
stability or FPS benchmark. A subsequent functional session also rendered a
public Mineclonia world and exercised inventory/crafting and password rejoining
with visually preserved items. Exact public stack counts were not independently
readable in Remote Play captures. Long survival/combat
and offline local-world startup remain unvalidated by the recorded functional tests.
The maintainer subsequently confirmed successful stability testing; its duration
and detailed measurements were not supplied. The separate double-password
registration dialog still needs work; see the [account workaround](docs/USAGE.md#servers-and-progress). Mineclonia is not bundled;
the engine retains embedded-server code. See [TESTING](docs/TESTING.md).

## Source and privacy

The source bundle includes the port, exact modified engine, dependency sources,
patches and licenses. SDK and packaging-tool binaries are fetched separately
from checksum-pinned sources; they are not redistributed here.

The repository contains no instructions for modifying console security.
Installation requires a PS4 already capable of running homebrew PKGs and
an existing package installer. See the [privacy notes](docs/PRIVACY.md) for the
publication review and information shared with game servers.

## Credits

MundoCraft uses and adapts Luanti and other open-source projects. Read
[LICENSE](LICENSE), [CREDITS.md](CREDITS.md), [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
and the [upstream distribution notes](docs/UPSTREAM_POLICY.md). Original
authorship and each component's terms remain in effect.

PlayStation is a trademark of Sony Interactive Entertainment. This port is
not endorsed by Luanti, Sony, Mojang or Microsoft. Third-party names identify
compatibility and provenance.
