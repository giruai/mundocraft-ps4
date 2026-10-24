# MundoCraft for PS4

**English** · [Español](README.es.md)

![MundoCraft for PS4: illustrated voxel landscape](assets/banner.svg)

![Experimental alpha](assets/alpha-badge.svg) ![PS4 homebrew](assets/platform-badge.svg) ![Luanti 5.17.0](assets/engine-badge.svg) ![Component licenses](assets/license-badge.svg)

**v0.3.19-alpha1** · **APP_VER 01.19**

MundoCraft brings the [Luanti](https://www.luanti.org/) voxel game engine to
PlayStation 4. This is an experimental, unofficial, modified port. The release
PKG has been cross-compiled and validated locally; **this exact package still
needs its own PS4 hardware test**. Earlier testing of engine build 01.19 does
not substitute for that test.

Luanti is free software developed by its community under LGPL-2.1-or-later.
Corresponding modified source, third-party notices and build instructions
accompany this release.

<p align="center"><img src="client/assets/mundocraft-icon0.png" width="120" alt="MundoCraft icon: a cheerful block island and tree"></p>

## Play and build

- [Download the latest release](https://github.com/giruai/mundocraft-ps4/releases/tag/v0.3.19-alpha1)
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

An earlier private PS4 build of engine 01.19 was tested with Mineclonia 0.124.1
in a creative LAN world: account registration, rejection of an incorrect
password, and rejoining after a server restart worked. Movement, looking,
inventory, digging, placing blocks, pause and normal exit to the PS4 menu were
checked; position and inventory persisted.

Those tests used different PKG bytes. Public server listing and geolocation
worked in earlier tests, but a selected public game never finished loading.
Long survival sessions, crafting and combat remain unvalidated. Audio is
disabled. See [TESTING](docs/TESTING.md) and the [build report](docs/BUILD_REPORT.md).

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
