# Third-party notices

MundoCraft is an unofficial PS4 port of Luanti, with platform support and other modifications. This application uses the Luanti engine, copyright 2010–2024 Perttu Ahola (celeron55) and contributors, licensed under LGPL-2.1-or-later. The corresponding modified sources and build recipes are supplied with the same release at https://github.com/giruai/mundocraft-ps4/releases. Access to that repository may require an invitation. License rights are not restricted by that access control.

These notices accompany both the executable package and source distribution. Original source headers prevail for their files. A collected source-comment inventory supplements, and does not replace, the original notices. Inclusion of a license text does not mean every source-tree component is linked into the executable.

| Component | License / included notice | Distribution treatment |
|---|---|---|
| Luanti at `c0e6812b1a4260bb25a1f606f70f55f4962bb97d`, including platform changes, builtin Lua and shaders | `LICENSES/LGPL-2.1.txt`, `Luanti-LICENSE.txt`, `Luanti-SOURCE-NOTICES.txt` | Exact modified sources, change record and build scripts accompany this release. |
| mini-GMP included in Luanti | LGPL-3.0-or-later option; `mini-GMP-NOTICE.txt`, `LGPL-3.0.txt`, `GPL-3.0.txt` | Exact sources and complete rebuild/relink route supplied; no restriction on reverse engineering needed to debug modifications. |
| IrrlichtMt / JThread | zlib-style / MIT; `IrrlichtMt-LICENSE.txt`, `Luanti-LICENSE.txt` | Preserve per-file notices and mark changed sources. |
| Lua 5.1.5, Lua BitOp, Lua lstrpack | MIT; `Lua-5.1-COPYRIGHT.txt`, `Lua-BitOp-LICENSE.txt`, `Lua-lstrpack-LICENSE.txt` | Preserve original copyright and permissions. |
| JsonCpp | MIT / public-domain alternative; `JsonCpp-LICENSE.txt` | MIT notice retained for jurisdictions that do not recognize the public-domain dedication. |
| Bundled SHA256 implementation | Legacy OpenSSL license; `Legacy-OpenSSL-SHA256-NOTICE.txt` | This is separate from the Mbed TLS backend. Required acknowledgements appear below. |
| Mesa PS4 at `1a59cc147b54e3970814b37313e2f27357672e9d` | Predominantly MIT; per-file terms, `Mesa/`, `SDK-Apache-2.0-Khronos.txt` | Per-file author/license inventory supplied; source-tree license collection also includes licenses for components not used by this binary. |
| SDL2 PS4 at `8967814a1e2c1a7ffd9da5c3eb8ffbeec4aed81a` | zlib; `SDL2-LICENSE.txt` | Original author notices retained; platform changes identified in sources. |
| orbis-compat at `314078413251644820955bd09335836ad19f88bd` | MIT; BSD-3-Clause exception for `include/sys/ioccom.h`; `orbis-compat-LICENSE.txt`, `SDK-BSD-3-Clause-FreeBSD.txt` | Actual linked support sources and modified build inputs supplied. |
| OpenOrbis-derived TLS linker script | GPL-3.0-only; `GPL-3.0.txt` | Exact script supplied; see copyright-scope qualification in `docs/UPSTREAM_POLICY.md`. |
| OpenOrbis replacement `libc.prx`, `libSceFios2.prx` and module CRT | GPL-3.0-only; `GPL-3.0.txt` | Built from OpenOrbis source, not copied proprietary console modules. Exact source and module build recipe supplied. |
| OpenOrbis musl libc and executable CRT | MIT and incorporated notices; `SDK-MIT-musl.txt` | Preserve the complete musl notice, including its internal component qualifications. |
| LLVM 11 libc++, libc++abi, libunwind, compiler-rt runtime code | Apache-2.0 WITH LLVM-exception | Four distinct `SDK-LLVM-*-LICENSE.TXT` files preserve the applicable appendices; the SDK asset name does not determine runtime version. |
| zlib 1.3.1 | zlib; `SDK-Zlib-zlib.txt` | Notice retained. |
| libpng at `2b978915d82377df13fcbb1fb56660195ded868a` | PNG Reference Library License v2; `libpng-LICENSE.txt` | Notices and modified-file markings retained. |
| libjpeg-turbo at `4e151a4ad91001b3aa8c2ece2205c15f487ce320` | IJG, BSD3 and zlib; `libjpeg-turbo-LICENSE.md`, `libjpeg-turbo-README.ijg` | Required IJG credit below; source notices retained. |
| FreeType at `42608f77f20749dd6ddc9e0536788eaad70ea4b5` | FTL option; `FreeType-FTL.txt`, `FreeType-LICENSE.txt` | Required FreeType acknowledgement below; subsidiary notices remain in source. |
| SQLite 3.46.1 | Public domain | https://www.sqlite.org/copyright.html; original source dedication retained. |
| Zstandard at `794ea1b0afca0f020f4e57b6732332231fb23c70` | BSD-3-Clause option; `Zstandard-BSD3.txt` | Copyright, disclaimer and no-endorsement terms retained. |
| curl 8.23.0 | curl license; `curl-COPYING.txt` | Complete copyright and permission retained. |
| Mbed TLS 3.6.7 | Apache-2.0 option (upstream dual Apache2 OR GPL2+); `MbedTLS-LICENSE.txt` | Preserve all applicable notices and mark modified files. |
| Mbed TLS Project Everest / HACL* / KreMLin code | Apache-2.0; `Apache-2.0.txt`, `MbedTLS-Everest-NOTICES.txt` | INRIA and Microsoft Corporation copyright notices retained, including support-header notices. |
| Mbed TLS p256-m and driver glue | Apache-2.0 option; `Apache-2.0.txt`, `MbedTLS-p256-m-NOTICES.txt` | Mbed TLS Contributors and author Manuel Pégourié-Gonnard credited; original dual-license markings retained. |
| Bundled SHA1 implementation | MIT; `SHA1-MIT-NOTICE.txt` | Copyright 2005 Michael D. Leonhard; distinct from the legacy OpenSSL SHA256 code. |
| Catch2 3.6.0 source | Boost Software License 1.0; `Catch2-BSL-1.0.txt`, `Catch2-NOTICE.txt` | Retained in the source distribution; unit tests and benchmarks are disabled in this executable. |
| Mozilla CA bundle converted by curl, 2026-09-25 | MPL-2.0; `MPL-2.0.txt` | Exact PEM and provenance supplied; https://curl.se/ca/cacert-2026-09-25.pem and https://curl.se/docs/caextract.html. |
| Luanti textures / sounds | CC BY-SA 3.0 default, named CC BY-SA4 / CC BY4 / Apache2 exceptions | Exact author-to-file mapping is preserved in `Luanti-LICENSE.txt`; full Creative Commons texts included. Source assets retain their original terms. |
| Arimo, Cousine, DroidSansFallback fonts | Apache-2.0 | The three corresponding `*-LICENSE.txt` files include the actual font notices. OFL1.1 text is included for the upstream menu-header lettering provenance recorded by Luanti. |
| SDK source/header support retained in corresponding sources | MIT, Apache2, zlib and stated per-file exceptions | `SDK-*` files preserve supporting notices, including SDL_image, SDL_ttf, stb and Khronos; these are not a claim that unused SDK libraries ship in the binary. |

This software is based in part on the work of the Independent JPEG Group.

Portions of this software are copyright © The FreeType Project (https://freetype.org). All rights reserved.

This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit (http://www.openssl.org/). This product includes cryptographic software written by Eric Young (eay@cryptsoft.com). This product includes software written by Tim Hudson (tjh@cryptsoft.com).

The authors' names are credits, not endorsements. No affiliation with or endorsement by Luanti, Sony, OpenOrbis or the other upstream projects is claimed.

Build-scope clarification: the shipped client disables Catch2 unit tests/benchmarks and Gettext. No bundled `lib/fmt`, tinygettext or separate utf8 library exists in the corresponding Luanti tree. UTF-8 helpers are part of Luanti/IrrlichtMt and retain their per-file notices. JsonCpp is bundled and linked under its included MIT notice. No direct stb include was found in the client/engine sources; the SDK stb notice is preserved for source/header provenance, not claimed as evidence that an stb library is linked.
