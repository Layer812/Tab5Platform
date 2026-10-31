# Tab5 Platform — Architecture R2

## Purpose and invariants

**Independent reusable hardware platform**, NOT a forked X68000 or MAME application. R6 X68KTab5 (verified source commit `802a10bf915d221ae91c950029d5e812ad4d085a`) remains read-only. A new platform app is built from its own root with ESP-IDF 5.5.4, 360MHz HP CPU, 16MB flash and 32MB PSRAM. M5Unified 0.2.28 SHA `368ad60807161625aa65baad37a4a3af2a7eccea` and M5GFX 0.2.35 SHA `65c816a38386062d2c3dc47d41237a9083acbc75` are pinned; guarded Tab5-specific overlays are fetched separately.

## Dependency graph

```
X68KTab5 future / Arcade sample / vgmM5 consumer / MidMod consumer
                         |  depends on
                         v
                  tab5_platform
                         |
                         v
        ESP-IDF 5.5.4 + M5Unified + M5GFX
                         |
                         v
             M5Stack Tab5 hardware
```

Consumers do not import other consumers. Shared audio paths are **PCM device/transport only**; Yamaha YM2151 / ymfm / other synth chips stay with vgmM5 or the application using them, while MIDI mapping stays with MidMod. No X68000 device model, ROM loader, disk formats, or MAME driver in the platform component.

## API and schedule

- `t5p_init`: once from app_main on CPU0. Initializes LCD/PSRAM queues/audio/SD and USB/touch.
- `t5p_video_acquire/present/cancel`: bounded immutable RGB565 PSRAM frames, 2 slots, up to 384×384, no waits from game CPU1. Display owned by core0 and geometry helper. **Not yet the fast R6 direct-native dirty-present backend**.
- `t5p_audio_submit`: signed interleaved stereo PCM 44.1kHz; accepts partial frames, never blocks guest. Audio worker CPU0 priority 4, uses the R6 M5Unified Speaker release callback where available.
- `t5p_input_poll`: touch/USB HID standardized input events, no X68000 keycodes. USB generic gamepads are not yet decoded.
- `t5p_storage_mount/enumerate`: 4-bit SDMMC GPIO43,44,39–42; never format. File system is separate from machine-specific ZIP/XDF/ADF/disk formats.
- `t5p_ui_*`: launcher-only text and rectangles; not a game-render API.
- `t5p_shutdown`: unsupported at R2; app is expected to run boot-to-reset.

## Samples vs platform

`sample/mame/` owns the libretro callback adapter and (when supplied) the MAME CPU/driver/ROM set code. Its original adapter may be distributed with the platform, but old-MAME engine code may only be included under that engine's license. A missing core must be explicit to users. The MAME sample is not a platform feature, acceptance criterion or required dependency.

## Future X68KTab5 migration

1. Keep R6 production unchanged and a fixed baseline.
2. Create a dedicated app-side integration branch and `x68k_platform_adapter.*` that maps existing X68KTab5 render/audio/input contracts to `t5p_*` (no direct PX68K headers inside `tab5_platform`).
3. Port compatible device services one by one; preserve R6 audio wakeup, 256fs codec, presenter ownership, LP message flow and SD/HumanXDF semantics.
4. Native build + a single end-to-end device check comparing to R6 before any production merge. Replacing current Screen Manager or sparse dirty renderer with the R2 generic presenter without throughput proof is NOT authorized.

## Known limitations

R2 sample source is uncompiled on Tab5. R2 384×384 frames do not handle all arcade boards. Generic USB gamepads, dedicated MIDI provider API, full codec plug-ins, runtime module loading, ZIP ROM parser and actual MAME driver are not part of the shared platform R2.
