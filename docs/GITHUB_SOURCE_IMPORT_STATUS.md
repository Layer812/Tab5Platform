# GitHub source-import status (2026-10-31)

**Complete: R2 source candidate imported into `Layer812/Tab5Platform`.**

- R2 source manifest: **38 of 38** files present on `main`.
- Repository total: **40 files** (including the GitHub Actions workflow and this import-status document).
- Imported source commit: `b4835ae6a36de6a41f80ce12bc385dc0083f9b4f`.
- Existing initial work was preserved during import; 22 missing files were added without overwriting the 16 previously imported source files.
- Host geometry/rotation test: **PASS** (run locally against the R2 source package).
- Native ESP-IDF 5.5.4 build: **NOT RUN**.
- Physical Tab5 test: **NOT RUN**.
- MAME core / working arcade game: **NOT INCLUDED / NOT RUNNING**; `sample/mame` is an adapter only.
- Original platform code release license: **PENDING** (must be selected before public release).
- GitHub visibility: **PRIVATE** (checked 2026-10-31).
- GitHub Release / M5Burner firmware: **NOT PUBLISHED**.

`RUN_THIS_BUILD.cmd` performs a local ESP-IDF build without flashing. Do not use `RUN_THIS_FLASH_APP_ONLY_EXPERIMENTAL.cmd` to test source import; it explicitly overwrites the installed app. X68KTab5 R6 production code and its partition data are unchanged.

For architecture and licensing boundaries, see [Platform Architecture R2](PLATFORM_ARCHITECTURE_R2.md), [Dependencies and Licenses](DEPENDENCIES_AND_LICENSES.md), and [Release Gates](RELEASE_GATES.md).
