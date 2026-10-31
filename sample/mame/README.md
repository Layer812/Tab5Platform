# sample/mame — optional MAME 2000 (0.37b5) frontend

**Status: host adapter implemented; actual MAME engine and executable game driver NOT shipped, NOT built, NOT run.**

The `t5_sample_mame.c` adapter provides an actual libretro frontend API implementation: 384×384-or-smaller RGB565/0RGB1555 frame forwarding, stereo PCM forwarding/resampling to 44.1kHz, HID keyboard to joypad mapping, CPU1 game loop, start/stop, and ROM-path validation. When an **actually ported and linked** MAME2000 core is present, it calls `retro_init`, `retro_load_game`, and `retro_run`. Without one, launch reports "MAME 2000 engine not linked". It **never pretends to emulate a game**.

## Source policy
- No MAME files, ROMs, BIOS, fonts, or unlicensed assets bundled.
- Candidate backend: libretro MAME 2000, MAME 0.37b5. https://github.com/libretro/mame2000-libretro
- Its **historic MAME non-commercial** license applies to the backend. Keep it isolated under this sample; do not merge it into `components/tab5_platform` or assume permissive terms.
- MAME 0.37b5 ROM set compatibility is required. User supplies authorized ROMs at `/sdcard/arcade/roms/*.zip`.
- If distributing backend code or binary, include the actual license, source and notices and ensure non-commercial conditions are met. Public release of the platform can omit the sample binary.

## Real backend integration gate
A port author can place a private, intentionally untracked `local_core_sources.cmake` here to append all **actually ported** libretro MAME2000 sources to `mame_sample_sources` and set `T5_MAME_CORE_PORT_READY TRUE`. Defining the flag alone does not implement or link a core. This gate is intentionally off by default; no unreviewed upstream compile is attempted.

Core ABI should be checked against official `libretro.h`, and the MAME-specific OSD + file paths, memory map, ROM ZIP, frame pacing, CPU load, and license must be verified before the sample can be described as a working emulator. Use a ROM set with distribution permission or locally owned original data; none is bundled.

## Controls (when engine linked)
- Arrow keys = directions; Return = start; Shift = select/coin; Z/X = A/B.
- ESC = quit to platform launcher. USB gamepad HID mapping remains a separate later task.

Do not identify an adapter boot/status screen as a running MAME board; no game executes in the default R2 package.

### Optional source checkout

If you want to work on the actual backend, run `python sample/mame/fetch_upstream.py` from the project root. This pins libretro `mame2000-libretro` at commit `181b42e5da0d2f0c7a2e59244f23633ed594de2e` (observed on 2026-10-31). Files are kept in ignored `sample/mame/external/` and are **not compiled** until separately ported and reviewed. The platform source release does not bundle this third-party tree.
