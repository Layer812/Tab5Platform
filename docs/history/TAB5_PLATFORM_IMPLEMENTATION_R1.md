# ArcadeTab5 Platform R1 — implementation contract

Date: 2026-10-31. Source-only build candidate; Tab5 native compilation and device validation not yet completed.

## Implementation inventory

| API subsystem | Current R1 source | Behavior | Limitations |
|---|---|---|---|
| Boot/system | `src/tab5_platform.cpp` | `M5.begin()`, 1280×720 LCD detection, PSRAM allocations, core0 workers | no graceful shutdown in R1 |
| Video | `src/tab5_platform.cpp` + `src/tab5_geometry.c` | two immutable PSRAM slots; 0/90/180/270 rotation, nearest-neighbor aspect fit to 1200×680; CPU0 presenter | max frame 384×384; no direct-native sparse DoubleFB optimization yet |
| Audio | `src/tab5_platform.cpp` | copied S16 stereo 44.1k PCM, 4096-frame ring, 256-frame Speaker staging, callback-released buffers; R6 codec recovery | no MAME mixer/rate conversion; exact device sound untested |
| Input | `src/tab5_input_usb.cpp` + platform touch | USB Boot keyboard/mouse HID usages, pointer events, 64-event queue | generic HID joypad parser not yet added |
| Storage | `src/tab5_platform.cpp` | 4-bit SDMMC mount, nonformatting file enumeration | no ZIP extraction, metadata DB or game loader |
| Launcher | `main/main.c` | readable English UI; ZIP list, sorting, paging, touch and keyboard selection, SD rescan | Selecting item is informational; no emulator attached |

## High-priority behaviors preserved from X68KTab5 R6

- M5Unified 0.2.28 SHA `368ad60807161625aa65baad37a4a3af2a7eccea` and M5GFX 0.2.35 SHA `65c816a38386062d2c3dc47d41237a9083acbc75`.
- Compatible panel/touch detection through tested upstream stack and R6 guarded overlays.
- Default HP 360MHz, ESP-IDF 5.5.4, 16MiB flash, HEX PSRAM, 128KiB L2 config.
- ES8388 256fs and Speaker buffer release callback through pinned R6 overlay scripts.
- `humanxdf` 0x810000/0x140000 retained in partition CSV; app-only flash script never requests full flash.

## Differences from X68KTab5 production

- No PX68K, X68000 screen line latch, custom GVRAM compositor, XDF media UI, YM2151, MidMod or Human68k dependency.
- R1 video is **generic and correct-first** using `M5.Display.pushImage` strips. R6 full-speed direct-native DoubleFB/sparse compositor is intentionally *not* represented as migrated or benchmarked.
- CPU1 reserves its time domain for future MAME; until that connection there is no emulation task.
- Only standardized USB boot keyboard/mouse report formats decoded; do not advertise joystick support until descriptor-aware parser is done.

## API concurrency (R1)

- `t5p_init()` only once from `app_main()` on core0.
- `t5p_video_acquire/present/cancel` use a short-try mutex; they are task context only. No retries/busy-wait inside API; on presentation failure producer retains the slot.
- `t5p_video_present` consumes buffer ownership, and the CPU0 task frees it after full `pushImage` transfer.
- UI calls must be bracketed by `t5p_ui_begin/end`; do not call them from audio callbacks or guest CPU hot loops.
- `t5p_audio_submit` copies samples into ring and returns accepted frame count; it never exposes the caller memory to the speaker. Speaker stage buffers are reused only after buffer-release callback.
- `t5p_input_poll` always returns promptly; touch/USB publish into a bounded FreeRTOS queue.
- Storage scan is for launcher task, never the guest CPU timing-critical path.

## Next implementation step

Upon native build/device confirmation, add a descriptor-aware gamepad mapper and generic arcade adaptation interface, and connect one licensed MAME subset driver. Do not fold MAME into the platform component.
