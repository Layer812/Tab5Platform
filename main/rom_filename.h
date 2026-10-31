#pragma once
#include <stddef.h>
#include <string.h>

/* A ROM filename is either stored intact (including its terminator) or
 * ignored. Never expose a truncated filename as a selectable ROM. */
static inline int t5p_rom_filename_copy(char *dst, size_t capacity, const char *name) {
    if (!dst || !name || capacity == 0) return 0;
    const size_t length = strlen(name);
    if (length >= capacity) return 0;
    memcpy(dst, name, length + 1);
    return 1;
}
