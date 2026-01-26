#include <stdio.h>
#include <string.h>
#include "stasiun.h"
#include "stasiun_shared.h"

static int parse_sts_number(const char *id) {
    if (!id) return -1;
    if (strncmp(id, "STS", 3) != 0) return -1;
    int n = 0;
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int stasiun_get_max_number(void) {
    int max_n = 0;
    for (int i = 0; i < g_stasiunCount; i++) {
        int n = parse_sts_number(g_stasiun[i].id);
        if (n > max_n) max_n = n;
    }
    return max_n;
}

static void format_sts_id(int n, char *out_id, size_t out_sz) {
    if (!out_id || out_sz == 0) return;
    if (n >= 0 && n <= 999) snprintf(out_id, out_sz, "STS%03d", n);
    else snprintf(out_id, out_sz, "STS%d", n);
}

void stasiun_create(const char* id, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat, const char* status) {
    if (g_stasiunCount >= MAX_RECORDS) return;

    Stasiun s;
    memset(&s, 0, sizeof(s));

    stasiun_safe_copy(s.id, sizeof(s.id), id);
    stasiun_safe_copy(s.kode, sizeof(s.kode), kode);
    stasiun_safe_copy(s.nama, sizeof(s.nama), nama);
    s.mdpl = mdpl;
    stasiun_safe_copy(s.kota, sizeof(s.kota), kota);
    stasiun_safe_copy(s.alamat, sizeof(s.alamat), alamat);
    stasiun_safe_copy(s.status, sizeof(s.status), status);
    s.active = 1;

    g_stasiun[g_stasiunCount++] = s;
    stasiun_save();
}

void stasiun_create_auto(char *out_id, size_t out_sz,
                         const char* kode, const char* nama,
                         int mdpl, const char* kota, const char* alamat, const char* status) {
    if (g_stasiunCount >= MAX_RECORDS) return;

    int next_n = stasiun_get_max_number() + 1;
    char id[16];
    format_sts_id(next_n, id, sizeof(id));
    if (out_id && out_sz > 0) snprintf(out_id, out_sz, "%s", id);

    stasiun_create(id, kode, nama, mdpl, kota, alamat, status);
}
