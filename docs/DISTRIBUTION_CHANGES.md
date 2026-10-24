# Distribution changes

## Beta 0.3.20-beta1

- Enable SDL2's Orbis AudioOut driver, static OpenAL Soft playback and Ogg/Vorbis decoding, with locked sources and complete notices.
- Make keyboard Done deliver native Enter after safely detaching the overlay; Cancel restores text without submitting.
- Map Square to native inventory secondary click, preserving event order, held-button masks and menu transition safety.
- Add local adapter regression checks and generated Ogg/Vorbis/OpenAL loopback tests.
- Promote the exact tested 0.3.20-alpha1 PKG to beta without rebuilding or changing its bytes. Internal alpha labels remain; distribution metadata and the asset name use beta1.
- Record PS4 LAN and public Mineclonia functional tests, plus successful stability testing reported by the maintainer. See [TESTING.md](TESTING.md).

## Published 0.3.19-alpha1

This is the first release, using APP_VER `01.19` and title ID `LUNT00001`.
An earlier development build 01.19 was tested on PS4; the distribution PKG
had different bytes; its subsequent exact-package hardware test is recorded in [TESTING.md](TESTING.md).

Publication-specific changes are:

- Neutral build paths and prefix maps in the libraries and client.
- An unofficial-port notice, engine license and source link in About, while
  preserving the original Luanti credits.
- Complete license notices inside the PKG.
- PRX modules built with a CRT object reconstructed from the pinned OpenOrbis
  `crtlib.c` source, without modifying the SDK.
- A standalone build recipe, corresponding sources, documentation and
  presentation artwork without console data.

This edition is not presented as a new fix for the historical memory fault.
The later 0.3.20 beta adds public-world and user stability acceptance. See
[TESTING.md](TESTING.md) for the evidence and limits.
