#ifndef MASTER_KARYAWAN_SHARED_H
#define MASTER_KARYAWAN_SHARED_H

#include <stdio.h>
#include <string.h>
#include "globals.h"

#define FILE_KARYAWAN "karyawan.dat"

/* Helper aman untuk copy string (selalu null-terminate) */
static inline void karyawan_safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

/* Disediakan oleh karyawan_R.c */
void karyawan_save(void);

#endif
