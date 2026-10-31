# sample/mame — first working board plan

## Goal

First arcade target: **Pac-Man** class of Z80 8-bit boards, chosen for modest 224×288 output (within R2 frame bounds). The goal is a real machine/driver run, not an animated UI. No ROM set, BIOS or game data is bundled. An authorized 0.37b5-compatible ROM ZIP is required by the end user.

## Integration sequence

1. The R2 front end already provides sample registration, ROM catalog, game start/stop, 0RGB1555/RGB565 video handoff, PCM bridge and joypad-key input.
2. Optional `python sample/mame/fetch_upstream.py` downloads the pinned upstream libretro MAME2000 source into ignored `sample/mame/external/`. This does not port it to ESP32-P4.
3. Port the minimal 0.37b5 core/OSD and one Z80 Pac-Man driver, maintaining upstream copyright/license. Exclude all other unrelated drivers, and provide actual reviewed sources in `local_core_sources.cmake` for a separate MAME-containing build.
4. Native `idf.py build`, app-only testing if user chooses, then boot + game start + control + stable PCM + menu exit.
5. Measure performance only when there is a real running game; do not run long diagnostic-only flash cycles.

## Known technical gaps

- Compiler/toolchain adaptations for historical MAME C and 32/64-bit host assumptions.
- ROM ZIP and graphics/sample decode path for the chosen driver.
- Real core link, MAME OSD event loop, input routing, FPS pacing and audio accuracy.
- Controls for a USB gamepad; only USB Boot keyboard and touch navigator exist in R2.
- 384×384 RGB565 frame budget (not suitable for all future boards).
- Preserve legal source notice/redistribution terms for modified non-commercial MAME core.

## Success criterion

Only claim `MAME sample PASS` when an actual compatible ROM is opened by the native CPU/board emulator and provides interactive video, input and audio on Tab5, and can exit to launcher. Merely rendering the frontend or showing a list of ZIPs is a platform sample demonstration, not game emulation.
