#include "tab5_platform.h"
#include <stdint.h>
void t5p_fit_rect(uint32_t sw, uint32_t sh, int rotation,
                  uint32_t bw, uint32_t bh, uint32_t *w, uint32_t *h) {
    if (!w || !h) return;
    *w=*h=0;
    if (!sw || !sh || !bw || !bh) return;
    if (rotation==T5P_ROTATION_90 || rotation==T5P_ROTATION_270) {
        uint32_t t=sw;sw=sh;sh=t;
    }
    /* Fit without floats; avoiding overflows of 32-bit intermediate. */
    if ((uint64_t)bw*sh <= (uint64_t)bh*sw) {
        *w=bw; *h=(uint32_t)((uint64_t)sh*bw/sw);
    } else { *h=bh; *w=(uint32_t)((uint64_t)sw*bh/sh); }
    if (!*w) *w=1;
    if (!*h) *h=1;
}
