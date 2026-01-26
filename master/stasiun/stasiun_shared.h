#ifndef MASTER_STASIUN_SHARED_H
#define MASTER_STASIUN_SHARED_H

#include <stdio.h>
#include <string.h>
#include "globals.h"

#define FILE_STASIUN "stasiun.dat"

static inline void stasiun_safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

void stasiun_save(void);

#endif
