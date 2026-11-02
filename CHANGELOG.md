# Tab5 Platform changelog

## R3 native build achieved — 2026-11-02

- **ESP-IDF 5.5.4 / ESP32-P4 native compile and link PASS** on the owner's Windows 11 build (`Project build complete`; `RESULT: PASS`; no device flashed).
- Native compilation is now validated; physical Tab5 peripherals/launcher and real MAME core remain unverified.
- No changes to production X68KTab5 R6; neither bootloader nor partition table nor protected HumanXDF was written.

## R3 build enablement — 2026-10-31

- Corrected the no-core MAME adapter compile path and M5GFX font namespacing.
- Added a host-compiled MAME stub regression and native build CI workflow (GitHub Actions jobs currently fail before steps, not a verified build).
- Added \`RUN_THIS_NATIVE_BUILD_REPORT.cmd\`: real Windows ESP-IDF build, one report ZIP with log/JSON/BIN-on-success, no device flashing.
- Production X68KTab5 R6 remains untouched; MAME engine not yet linked.

## R2 source release candidate — 2026-10-31

- Renamed top-level product from an arcade emulator app to reusable **Tab5 Platform**.
- Kept `components/tab5_platform/` independent of emulator implementations.
- Created `sample/mame/` as the sample application: libretro 1.x callback adapter, ROM path validation, CPU1 execution and return-to-launcher plumbing.
- Added a safe, pinned, **optional** upstream MAME2000 source checkout; no upstream MAME code is bundled or linked by default.
- Added upstream-source integration policies for vgmM5 (original-code MIT; per-dependency terms retained) and MidMod (GPL-3.0).
- Provided platform architecture, first-board plan, dependencies/license matrix and conservative release gates.
- Preserved R1 LCD/ES8388/SD/USB/PSRAM implementation and R6-derived library compatibility pins.
- The original X68KTab5 R6 source, firmware and release are unchanged.

Physical Tab5 runtime and MAME game execution are not claimed.