#ifndef MASTER_KERETA_SHARED_H
#define MASTER_KERETA_SHARED_H

#include <stdio.h>
#include <string.h>
#include "globals.h"

#define FILE_KERETA "kereta.dat"

static inline void kereta_safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

/* util: token lower tanpa spasi/simbol */
static inline void kereta_token_lower_nospace(char *out, size_t out_sz, const char *in) {
    if (!out || out_sz == 0) return;
    out[0] = '\0';
    if (!in) return;

    size_t j = 0;
    for (const unsigned char *p = (const unsigned char*)in; *p && j + 1 < out_sz; p++) {
        unsigned char c = *p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        else if (c >= 'a' && c <= 'z') { /* ok */ }
        else if (c >= '0' && c <= '9') { /* ok */ }
        else continue; /* buang simbol */
        out[j++] = (char)c;
    }
    out[j] = '\0';
}

static inline int kereta_normalize_kelas(char *out, size_t out_sz, const char *in) {
    char tok[32];
    kereta_token_lower_nospace(tok, sizeof(tok), in);
    if (tok[0] == '\0') return 0;

    if (strcmp(tok, "1") == 0 || strcmp(tok, "bisnis") == 0) {
        kereta_safe_copy(out, out_sz, "Bisnis");
        return 1;
    }
    if (strcmp(tok, "2") == 0 || strcmp(tok, "ekonomi") == 0) {
        kereta_safe_copy(out, out_sz, "Ekonomi");
        return 1;
    }
    if (strcmp(tok, "3") == 0 || strcmp(tok, "eksekutif") == 0) {
        kereta_safe_copy(out, out_sz, "Eksekutif");
        return 1;
    }
    return 0;
}

static inline int kereta_normalize_status_operasional(char *out, size_t out_sz, const char *in) {
    char tok[32];
    kereta_token_lower_nospace(tok, sizeof(tok), in);
    if (tok[0] == '\0') return 0;

    if (strcmp(tok, "1") == 0 || strcmp(tok, "aktif") == 0) {
        kereta_safe_copy(out, out_sz, "Aktif");
        return 1;
    }

    /* toleransi typo umum: maintenacne */
    if (strcmp(tok, "2") == 0 || strcmp(tok, "perawatan") == 0 || strcmp(tok, "pemeliharaan") == 0 ||
        strcmp(tok, "maintenance") == 0 || strcmp(tok, "maintenacne") == 0 || strncmp(tok, "maint", 5) == 0) {
        kereta_safe_copy(out, out_sz, "Perawatan");
        return 1;
    }

    return 0;
}

void kereta_save(void);

#endif
