#ifndef TRANSAKSI_PEMBAYARAN_SHARED_H
#define TRANSAKSI_PEMBAYARAN_SHARED_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "../jadwal/jadwal.h"

#define FILE_PEMBAYARAN "pembayaran_tiket.dat"

static inline void pembayaran_safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

void pembayaran_save(void);

#endif
