#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "tab5_platform.h"
int main(void){
    uint32_t w,h;
    t5p_fit_rect(224,288,T5P_ROTATION_0,1200,680,&w,&h);
    assert(w==528 && h==680);
    t5p_fit_rect(224,288,T5P_ROTATION_90,1200,680,&w,&h);
    assert(w==874 && h==680);
    t5p_fit_rect(320,240,T5P_ROTATION_0,960,720,&w,&h);
    assert(w==960 && h==720);
    t5p_fit_rect(0,240,T5P_ROTATION_0,960,720,&w,&h);
    assert(w==0 && h==0);
    puts("PASS: aspect fit/rotations/zero dimensions");
    return 0;
}
