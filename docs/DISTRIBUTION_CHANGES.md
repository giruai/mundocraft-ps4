# Distribution build 0.3.19-alpha1

This is the first release, using APP_VER `01.19` and title ID `LUNT00001`.
An earlier development build 01.19 was tested on PS4; the distribution PKG
has different bytes and needs its own hardware test.

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
Long sessions and fully loading public games remain unvalidated. See
[TESTING.md](TESTING.md) for the evidence and limits.
