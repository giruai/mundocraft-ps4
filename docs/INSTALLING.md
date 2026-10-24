# Install on PS4

This guide covers installing the PKG with a package installer you already have. It requires a PS4 that can already run homebrew. It does not explain how to modify console security.

## Before installing

1. Download the `MundoCraft-PS4-v0.3.19-alpha1` PKG from this repository's Releases page.
2. Download `SHA256SUMS` and compare the PKG's SHA-256 hash. On Linux, use `sha256sum file.pkg`; on macOS, use `shasum -a 256 file.pkg`. The result must exactly match the PKG's entry in `SHA256SUMS`.
3. Use your existing package installer to open and install the PKG. Follow that installer's own instructions.

The title is **MundoCraft - Luanti**, with Title ID **LUNT00001** and application version **01.19**. When updating the same application, install over the existing title to keep its data. Do not uninstall the title if you want to keep local worlds or settings.

## First launch

Open MundoCraft from the console menu and choose a compatible server. To play in a world on your local network, the server must be running and reachable from the console. MundoCraft does not start a local server automatically.

When a server asks for a name, choose one for that server. If it asks for a password, use the password for that account. Identities and progress belong to each server; there is no central MundoCraft account or Minecraft account.

The client downloads the game and resources offered by the server. Mineclonia is not included in the PKG; to play Mineclonia, connect to a server that has it installed.

## Network and compatibility

You can browse the public server list or connect to a LAN server. Availability depends on the network and on the server using a protocol compatible with Luanti 5.17.0. Not every server, game, or mod is guaranteed to work.

For controls, see [USAGE.md](USAGE.md). Known connection behavior and limitations are described in [TESTING.md](TESTING.md).
