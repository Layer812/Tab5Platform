# Tab5 Platform changelog

## R2 source release candidate — 2026-10-31

- Renamed top-level product from an arcade emulator app to reusable **Tab5 Platform**.
- Kept `components/tab5_platform/` independent of emulator implementations.
- Created `sample/mame/` as the sample application: libretro 1.x callback adapter, ROM path validation, CPU1 execution and return-to-launcher plumbing.
- Added a safe, pinned, **optional** upstream MAME2000 source checkout; no upstream MAME code is bundled or linked by default.
- Added upstream-source integration policies for vgmM5 (original-code MIT; per-dependency terms retained) and MidMod (GPL-3.0).
- Provided platform architecture, first-board plan, dependencies/license matrix and conservative release gates.
- Preserved R1 LCD/ES8388/SD/USB/PSRAM implementation and R6-derived library compatibility pins.
- The original X68KTab5 R6 source, firmware and release are unchanged.

Native IDF build, physical Tab5 runtime, and MAME game execution are not claimed.