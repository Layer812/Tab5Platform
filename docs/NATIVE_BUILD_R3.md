# R3 build-enablement notes — 2026-10-31

Goal: establish an **independent, repeatable ESP-IDF 5.5.4 ESP32-P4 native build** before touching any device. Platform source on GitHub remains isolated from the X68KTab5 R6 production application and firmware.

## Defects fixed

- The optional `sample/mame` adapter referenced `sizeof(s_rom)` in the no-core build, even though the ROM buffer was declared only with `T5_SAMPLE_MAME_WITH_LIBRETRO_CORE`. R3 introduces a fixed 512-byte capacity independent of that build flag.
- Add an actual host GCC `-Wall -Wextra -Werror` compile/link check of the **unlinked** MAME sample adapter, its error handling and HID event API, alongside geometry tests.
- Host test binaries are written only to a temporary directory, then removed.
- A native GitHub Actions workflow builds the **platform and stub MAME frontend** using official `espressif/idf:v5.5.4` Docker tooling, pinned M5Unified 0.2.28 and M5GFX 0.2.35, and the R6 overlays isolated to this checkout. It runs `idf.py set-target esp32p4` then `idf.py build`.

## Scope and verification

- Host geometry tests: PASS locally.
- Host MAME unlinked adapter test: PASS locally (compile + basic API checks).
- Native ESP-IDF build result: **check Actions run for this exact commit**; workflow addition is not a claim of successful firmware build.
- Firmware flash / Tab5 physical launcher test: NOT RUN (no connected physical device).
- Real arcade-game execution: NOT AVAILABLE; no MAME engine linked.
- MAME ROM sets, BIOS and copyrighted data: NOT DISTRIBUTED.

`RUN_THIS_BUILD.cmd` is a build-only path for Windows ESP-IDF 5.5.4 at `G:\esp-idf-5.5.4`. No full-flash, partition-table change or HumanXDF data access is required.

## GitHub Actions execution environment observed on 2026-10-31

- Both the native build workflow [run 41184883560](https://github.com/Layer812/Tab5Platform/actions/runs/41184883560) and the host workflow [run 41184883737](https://github.com/Layer812/Tab5Platform/actions/runs/41184883737) completed with **failure before any recorded job steps** (empty steps; no logs available through the GitHub API). Older host runs exhibit the same pattern. This does **not** establish a compilation error or a PASS.
- Confirm Actions runner availability/account/repository execution restrictions in GitHub UI if these pre-step failures continue; the available logs do not prove a specific cause.
- Independent local host GCC tests did pass. Native build and physical Tab5 operation remain unverified.

## Static M5GFX API compatibility fix

- Qualify fonts as `lgfx::fonts::FreeSansBold18pt7b` and `lgfx::fonts::FreeSansBold24pt7b`, and text datum as `lgfx::textdatum_t::top_left`, matching the pinned upstream M5GFX 0.2.35 public headers. Typography size is unchanged.

## R3 build-only single-report Windows entrypoint

From the private Tab5Platform checkout, run \`RUN_THIS_NATIVE_BUILD_REPORT.cmd\`.
It activates ESP-IDF 5.5.4 from \`IDF_PATH\` or \`G:\esp-idf-5.5.4\`, executes the actual library bootstrap, target selection and native build, and writes one \`Tab5Platform_NATIVE_BUILD_RESULT.zip\` containing \`BUILD.log\` and \`RESULT.json\`. A successfully generated \`tab5_platform_reference.bin\` is added only if the build command returned success. The JSON contains the exact commit, exit code and BIN SHA-256. No flash and no backups. The report directory and ZIP are ignored by Git.

On systems without Windows / ESP-IDF, the script exits nonzero and emits an explicit FAIL report instead of claiming a build success. The authoring environment verified Python syntax and the negative-path report ZIP; a successful ESP-IDF/physical build remains unverified.
