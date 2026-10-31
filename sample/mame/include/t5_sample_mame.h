/* Tab5Platform sample/mame - intentionally NOT part of the platform API.
 * Original integration code, 2026.
 * The actual MAME 0.37b5 engine is not included in this package.
 */
#pragma once
#include "tab5_platform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    T5_MAME_IDLE = 0,
    T5_MAME_UNAVAILABLE = 1,
    T5_MAME_BAD_ROM = 2,
    T5_MAME_BOOTING = 3,
    T5_MAME_RUNNING = 4,
    T5_MAME_FAILED = 5
} t5_mame_state_t;
/* Starts the selected ROM with the optional libretro MAME2000 core on CPU1.
 * With no engine linked, fails fast and reports UNAVAILABLE. No imitation game.
 */
int t5_mame_start(const char *absolute_rom_path);
void t5_mame_stop(void); /* async request; safely returns to launcher when done */
void t5_mame_post_input(const t5p_input_event_t *event);
t5_mame_state_t t5_mame_state(void);
const char *t5_mame_status_text(void);
int t5_mame_core_built(void);
#ifdef __cplusplus
}
#endif
