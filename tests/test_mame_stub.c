#include "t5_sample_mame.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    assert(!t5_mame_core_built());
    assert(t5_mame_state()==T5_MAME_IDLE);
    assert(t5_mame_start(NULL)==T5P_ERR_INVALID);
    assert(t5_mame_start("/tmp/game.zip")==T5P_ERR_INVALID);
    char oversized[610];
    memcpy(oversized,"/sdcard/arcade/roms/",20);
    memset(oversized+20,'x',sizeof(oversized)-21);
    oversized[sizeof(oversized)-1]='\0';
    assert(t5_mame_start(oversized)==T5P_ERR_INVALID);
    assert(t5_mame_start("/sdcard/arcade/roms/no_such_game.zip")==T5P_ERR_IO);
    assert(t5_mame_state()==T5_MAME_BAD_ROM);
    assert(strstr(t5_mame_status_text(),"missing")!=NULL);
    t5p_input_event_t key={0};
    key.type=T5P_INPUT_KEY_DOWN;
    key.code=0x28; /* Enter */
    t5_mame_post_input(&key);
    t5_mame_stop();
    puts("PASS MAME unlinked adapter host compile and basic behavior");
    return 0;
}
