#ifndef TRANSAKSI_JADWAL_SHARED_H
#define TRANSAKSI_JADWAL_SHARED_H

#include <stdio.h>
#include <string.h>
#include "globals.h"

#define FILE_JADWAL "jadwal_tiket.dat"

static inline void jadwal_safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

void jadwal_save(void);

#endif
