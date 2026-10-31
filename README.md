> **Repository import in progress (2026-10-31):** The full R2 source is in the downloadable publish archive, but the GitHub tree is not complete yet. See [import status](docs/GITHUB_SOURCE_IMPORT_STATUS.md). Do not attempt an ESP-IDF build from this branch until the remaining source files are imported.

# Tab5 Platform — R2 Platform Source Release Candidate

**A reusable ESP32-P4 / M5Stack Tab5 hardware platform. Emulators, players and ROMs are NOT part of the platform.**

**Status (2026-10-31): R2 source package. Source inspection and limited host tests only; native ESP-IDF 5.5.4 build and physical Tab5 verification remain NOT RUN. The MAME emulator engine is NOT included, and no arcade game runs in the bundled build.**

## Ownership and directory layout

```
components/tab5_platform/    Hardware abstraction/API only (not PX68K/MAME/vgmM5/MidMod)
main/                        Reference launcher, launches selected sample
sample/mame/                 Optional MAME2000/libretro adapter; NO MAME core included
integrations/vgmm5/          How an external app uses upstream vgmM5 (no copy)
integrations/midmod/          How an external app uses upstream MidMod (no copy)
docs/                        Platform, integration, source and release contracts
tools/                       Pinned dependency/overlay bootstrap for Tab5
```

**Dependency direction:** `Tab5 Platform <- application/sample <- emulator/music engine`. The platform component must never depend on `sample/`, X68KTab5, MAME, vgmM5 or MidMod. Future X68KTab5 integration changes only its app-side adapter; production X68KTab5 R6 remains read-only.

## What ships
- Reusable standalone `tab5_platform` component: M5Unified/M5GFX LCD, PSRAM RGB565 slots and CPU0 presenter; ES8388 44.1k stereo output; touch and USB boot keyboard/mouse; SDMMC 4-bit filesystem; bounded nonblocking inputs.
- Reference app with visible MAME sample ROM selector (`/sdcard/arcade/roms/*.zip`) and honest MAME-core availability status.
- `sample/mame` can call `retro_init`, `retro_load_game`, `retro_run` and return to the launcher **ONLY after** an actual ESP32-P4 MAME2000 libretro engine port is provided and linked. Without it ROM selection says `MAME 2000 engine not linked`.
- No embedded MAME code or copyrighted game data. No code is copied from vgmM5/MidMod; references point to their official repositories.

## Windows / ESP-IDF 5.5.4

1. Clone into its own directory (NOT `G:\X68KTab5`, and not the X68KTab5 production tree).
2. Run `RUN_THIS_BUILD.cmd`. It activates `G:\esp-idf-5.5.4` where needed, downloads pinned M5GFX 0.2.35 and M5Unified 0.2.28, and downloads the R6-validated overlays into this tree (not into X68KTab5).
3. It attempts `idf.py build` only. It does not flash anything.
4. The included `RUN_THIS_FLASH_APP_ONLY_EXPERIMENTAL.cmd` is a separate, deliberate action, and will **replace the currently installed app** if used. It must only be used with compatible partition table and intentionally approved app-only flashing. The existing `humanxdf` partition is at 0x810000 and must not be overwritten.

`platformio.ini` has been intentionally omitted: native ESP-IDF is the only authoritative target for R2.

## License boundaries

`components/tab5_platform` is independent original-code infrastructure. The shipped third-party dependency overlays and fetched source components retain their original notices. `sample/mame` is an original frontend adapter only; the optional external MAME2000 core would carry its historic **noncommercial MAME license**. vgmM5's original source declares MIT terms while bundled third-party code may differ. MidMod's main project is **GPL-3.0**, so downstream apps linking it require separate compliance analysis. See `docs/DEPENDENCIES_AND_LICENSES.md` before publishing a third-party-containing binary.

## Native/physical verification status

- Pure geometry host test: runnable with `sh tests/run_host_tests.sh`.
- Firmware source: no complete native ESP-IDF compile performed in this authoring environment.
- LCD/SD/USB/audio and any MAME gameplay: NOT TESTED on a physical Tab5.
- Official GitHub release / M5Burner: NOT PUBLISHED by this package.

See [Platform architecture](docs/PLATFORM_ARCHITECTURE_R2.md), [MAME sample integration](sample/mame/README.md), [release gates](docs/RELEASE_GATES.md).
