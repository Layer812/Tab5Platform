#include "rom_filename.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    char dst[128];
    char fit[128];
    char too_long[129];
    memset(fit, 'a', 123);
    memcpy(fit + 123, ".zip", 5); /* exactly 127 chars + NUL */
    memset(too_long, 'b', 124);
    memcpy(too_long + 124, ".zip", 5); /* exactly 128 chars + NUL */
    memset(dst, 0xa5, sizeof(dst));
    assert(t5p_rom_filename_copy(dst, sizeof(dst), fit));
    assert(memcmp(dst, fit, sizeof(dst)) == 0);
    memset(dst, 0x7e, sizeof(dst));
    assert(!t5p_rom_filename_copy(dst, sizeof(dst), too_long));
    for (size_t i=0; i<sizeof(dst); ++i) assert(dst[i] == 0x7e);
    assert(!t5p_rom_filename_copy(NULL, sizeof(dst), fit));
    assert(!t5p_rom_filename_copy(dst, sizeof(dst), NULL));
    assert(!t5p_rom_filename_copy(dst, 0, fit));
    puts("PASS ROM filename is copied intact or rejected (127/128 boundary)");
    return 0;
}
