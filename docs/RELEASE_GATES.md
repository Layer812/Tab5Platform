# Tab5Platform R2 release gates

| Gate | Result | Basis |
|---|---|---|
| R6 X68KTab5 production unchanged | PASS | new independent directory, GitHub R6 read-only |
| Platform API does not link PX68K/MAME/vgmM5/MidMod | PASS source structure | component CMake references M5 deps only |
| MAME sample code lives in `sample/mame` | PASS source structure | independent IDF component, optional core integration flag |
| No MAME game emulation claimed when core missing | PASS source structure | `t5_mame_start` returns NOT_READY with explicit error |
| vgmM5 and MidMod from their own upstream | PASS documentation | references pinned commit, not copies |
| Host geometry verification | RUN with tests | `tests/run_host_tests.sh` |
| Native ESP-IDF 5.5.4 build | NOT RUN | no configured Windows ESP-IDF in this authoring container |
| Physical Tab5 display/USB/SD/audio | NOT RUN | device inaccessible |
| Libretro/MAME real driver link/load/run | NOT DONE | external core unported/unlinked |
| License finalized for original platform code | PENDING | project owner must choose and apply original-code license; upstream code has separate terms |
| Public firmware GitHub/M5Burner release | NOT READY | native build, hardware and third-party license acceptance are still required |

Do not run full-flash, do not write bootloader or partition table. Existing HumanXDF occupies 0x810000, and the 8MB factory app begins at 0x10000. The optional app-flash script **replaces** the application; it does not preserve that application and should only be used after checking the installed layout. No backup required by the source package.
