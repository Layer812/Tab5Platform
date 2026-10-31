# Source origins, links and release boundaries (2026-10-31)

| Source | Reference | Treatment |
|---|---|---|
| Tab5 Platform independent host | `components/tab5_platform` | original platform source; decide/retain formal platform LICENSE before external official release |
| M5Unified 0.2.28 | https://github.com/m5stack/M5Unified | external pinned source; MIT; original header/notice preserved |
| M5GFX 0.2.35 | https://github.com/m5stack/M5GFX | external pinned source; MIT core, embedded fonts/components may have other terms |
| ESP-IDF 5.5.4 | https://github.com/espressif/esp-idf | framework; Apache-2.0 and individual component notices |
| X68KTab5 R6 compatibility scripts | https://github.com/Layer812/X68KTab5/tree/802a10bf915d221ae91c950029d5e812ad4d085a | copied by bootstrap into this project when building; **not blanket relicensed** |
| vgmM5 | https://github.com/Layer812/vgmM5 (`0f311eb3a2a2fa690a9665cb2f8025c61c622970`) | **reference upstream**, no source bundled; original upstream code MIT, code in third-party sections retains independent obligations |
| MidMod | https://github.com/Layer812/MidMod (`72d6f2f7511eefb47dcd12be62149cd6f451a29e`) | **reference upstream**, no source bundled; GPL-3.0, needs application-level compliance for linking |
| MAME 2000 / MAME 0.37b5 | https://github.com/libretro/mame2000-libretro | optional **sample engine** not bundled; historical MAME **non-commercial** license, ROMs separate copyrighted works |

A source directory layout does not magically change license obligations: the terms of code actually compiled into a firmware binary still apply. **Platform and bundled MAME must never be advertised as entirely MIT-licensed.** Actual `LICENSE` for original platform code is currently left for the project owner to select; this source candidate is not a legally finalized binary release.

Name and logo: MAME trademark is not a license to use official branding. Sample project descriptions must identify upstream factually, not imply MAMEDev endorsement. Commercial ROMs are not provided.

Once an independent platform repository is established, pin revision identities for each integration and include its actual source/notice obligations in downstream releases; do not just copy `vgmm5` or `midmod` implementations into the platform.

MAME2000 candidate pinned upstream commit (2026-10-31): `181b42e5da0d2f0c7a2e59244f23633ed594de2e`. This is a source provenance reference, **not a compatible ESP32-P4 core**.
