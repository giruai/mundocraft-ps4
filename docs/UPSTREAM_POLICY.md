# Upstream policy and distribution notes

This is an unofficial modified Luanti port. Preserve the Luanti credits and existing copyright notices, identify significant platform changes, and provide the exact corresponding modified source for each binary release. Keep source access available to every binary recipient; private GitHub visibility must not be presented as a prohibition on exercising the licenses' redistribution rights.

The engine retains LGPL-2.1-or-later. Bundled mini-GMP is used under its LGPL-3.0-or-later option. This distribution supplies the complete engine and integration source, dependencies and patches, and build/relink recipes so recipients can rebuild with modified LGPL components. LGPL3 section 4 permits a Combined Work under terms of choice subject to its conditions; the presence of an independently licensed application component does not by itself prove an incompatible combination. Do not impose reverse-engineering restrictions that prevent debugging modifications to LGPL components. Preserve the original licenses rather than changing every file to MIT or GPL3.

The two packaged replacement PRX modules come from OpenOrbis source; their simple dummy implementations and module CRT have the GPL3 grant documented by that upstream. Supply their corresponding sources, Makefiles and build instructions under GPL-3.0-only, alongside the binary. The executable startup object is musl code under MIT; it is distinct from the GPL3 module CRT.

The OpenOrbis-derived `orbis-tls.ld` is GPL-3.0-only and is supplied with its source and notice. It directs the link and also emits fixed interpreter-path data through `QUAD` directives. Whether that small fixed-data output makes the complete executable a derivative covered work has not been established by this inventory. Neither the fact that a GPL tool ran nor this script's label alone is a basis for asserting that the entire binary is GPL3. No blanket relicensing claim or universal compatibility guarantee is made here.

The bundled SHA256 implementation retains its legacy OpenSSL notice and advertising acknowledgement. It must not be mistaken for modern OpenSSL's Apache2 license, and Mbed TLS being the TLS backend does not remove that SHA256 notice. A blanket GPL3 conversion of the entire application would require a separate compatibility analysis; it is not performed by these notices.

Assets are licensed separately from code. Preserve Luanti's exact texture/sound author mapping, the CC BY-SA3/4, CC BY4 and Apache2 exceptions, and the font notices. If changing covered artwork, identify the changes and apply the asset license's attribution and share-alike conditions. The generated MundoCraft icon has separate provenance documented in `client/assets/ICON.md`.

The OpenOrbis SDK, import-stub files and packaging-tool binaries are not redistributed in this release. The source build can fetch the pinned public SDK independently. Upstream has recorded questions concerning the generated import stubs' redistribution grant and corresponding source for some bundled tools; linking imports into an application is distinct from redistributing those tool/stub files. Linked musl and LLVM runtime notices are nevertheless included because runtime code does travel in the binary. Do not silently add an SDK archive to the release without reviewing those separate obligations.

Primary references:

- [Luanti licensing guidance](https://docs.luanti.org/for-creators/licensing/)
- [Luanti distribution and rebranding guidance](https://docs.luanti.org/for-creators/distributing-games-outside-contentdb/)
- [Exact Luanti source/asset license](https://github.com/luanti-org/luanti/blob/c0e6812b1a4260bb25a1f606f70f55f4962bb97d/LICENSE.txt)
- [LGPL3 section 4](https://www.gnu.org/licenses/lgpl-3.0.html)
- [OpenOrbis source and license](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/v0.5.4)
- [Pinned orbis-compat licensing investigation](https://github.com/orbis-ports/orbis-compat/blob/314078413251644820955bd09335836ad19f88bd/LICENSING.md)
- [Porting-kit linker-script terms and qualification](https://github.com/orbis-ports/orbis-porting-kit/blob/main/cmake/orbis-tls.ld)
- [Mozilla CA conversion provenance](https://curl.se/docs/caextract.html)
- [OpenAI terms for generated output](https://openai.com/policies/terms-of-use/)

These sources explain the provenance and the identified conditions. They do not constitute a guarantee about all legal questions or independently clear all trademarks. Product naming and documentation should clearly identify the unofficial port and avoid suggesting upstream or platform-vendor endorsement.
