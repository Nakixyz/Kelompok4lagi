#include <stdio.h>
#include <string.h>
#include "kereta.h"
#include "kereta_shared.h"

static int parse_ka_number(const char *kode) {
    if (!kode) return -1;
    if (strncmp(kode, "KA", 2) != 0) return -1;
    int n = 0;
    if (sscanf(kode + 2, "%d", &n) != 1) return -1;
    return n;
}

static int kereta_get_max_number(void) {
    int max_n = 0;
    for (int i = 0; i < g_keretaCount; i++) {
        int n = parse_ka_number(g_kereta[i].kode);
        if (n > max_n) max_n = n;
    }
    return max_n;
}

static void format_ka_id(int n, char *out_id, size_t out_sz) {
    if (!out_id || out_sz == 0) return;
    if (n >= 0 && n <= 999) snprintf(out_id, out_sz, "KA%03d", n);
    else snprintf(out_id, out_sz, "KA%d", n);
}

void kereta_create(const char* kode, const char* nama, const char* kelas,
                   int kapasitas, int gerbong, const char* status) {
    if (g_keretaCount >= MAX_RECORDS) return;

    Kereta k;
    memset(&k, 0, sizeof(k));

    kereta_safe_copy(k.kode, sizeof(k.kode), kode);
    kereta_safe_copy(k.nama, sizeof(k.nama), nama);

    /* normalisasi kelas */
    {
        char kn[32];
        if (kereta_normalize_kelas(kn, sizeof(kn), kelas)) kereta_safe_copy(k.kelas, sizeof(k.kelas), kn);
        else kereta_safe_copy(k.kelas, sizeof(k.kelas), "Ekonomi");
    }

    k.kapasitas = kapasitas;
    k.gerbong = gerbong;

    /* normalisasi status operasional */
    {
        char sn[32];
        if (kereta_normalize_status_operasional(sn, sizeof(sn), status)) kereta_safe_copy(k.status, sizeof(k.status), sn);
        else kereta_safe_copy(k.status, sizeof(k.status), "Aktif");
    }

    k.active = 1;

    g_kereta[g_keretaCount++] = k;
    kereta_save();
}

void kereta_create_auto(char *out_id, size_t out_sz,
                        const char* nama, const char* kelas,
                        int kapasitas, int gerbong, const char* status) {
    if (g_keretaCount >= MAX_RECORDS) return;

    int next_n = kereta_get_max_number() + 1;
    char id[16];
    format_ka_id(next_n, id, sizeof(id));
    if (out_id && out_sz > 0) snprintf(out_id, out_sz, "%s", id);

    kereta_create(id, nama, kelas, kapasitas, gerbong, status);
}
