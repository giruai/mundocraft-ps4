# Module startup sources

These are source files, not Sony libraries or precompiled tools.
`source-sha256.json` records this distribution's original source bytes. They
come from the SDK and OpenOrbis inputs pinned in `dependencies.lock.json`;
`orbis-tls.ld` is the TLS variant distributed by the pinned compatibility kit.

OpenOrbis GPL-3.0-only terms are preserved. See `../../LICENSES/` and
`../../THIRD_PARTY_NOTICES.md` for complete licenses, credits and the documented
scope of the linker scripts.

Packaging compiles `crt/crtlib.c` and the `libc` and `libSceFios2` modules from
these files. To modify them, edit their sources here and run `./build.sh`;
the changes are copied into the isolated build prefix. The SDK supplies link
inputs and tools and is not modified.

`crtlib.S` and `link.x` are retained as corresponding SDK reference sources;
these modules use the C CRT source. The client links with the pinned TLS
linker-script variant.
