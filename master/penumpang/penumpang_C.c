#include <stdio.h>
#include <string.h>
#include "penumpang.h"
#include "penumpang_shared.h"

static int parse_pnp_number(const char *id) {
    if (!id) return -1;
    if (strncmp(id, "PNP", 3) != 0) return -1;
    int n = 0;
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int penumpang_get_max_number(void) {
    int max_n = 0;
    for (int i = 0; i < g_penumpangCount; i++) {
        int n = parse_pnp_number(g_penumpang[i].id);
        if (n > max_n) max_n = n;
    }
    return max_n;
}

static void format_pnp_id(int n, char *out_id, size_t out_sz) {
    if (!out_id || out_sz == 0) return;
    if (n >= 0 && n <= 999) snprintf(out_id, out_sz, "PNP%03d", n);
    else snprintf(out_id, out_sz, "PNP%d", n);
}

void penumpang_create(const char* id, const char* nama, const char* email,
                      const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin) {
    if (g_penumpangCount >= MAX_RECORDS) return;

    Penumpang p;
    penumpang_safe_copy(p.id, sizeof(p.id), id);
    penumpang_safe_copy(p.nama, sizeof(p.nama), nama);
    penumpang_safe_copy(p.email, sizeof(p.email), email);
    penumpang_safe_copy(p.no_telp, sizeof(p.no_telp), no_telp);
    penumpang_safe_copy(p.tanggal_lahir, sizeof(p.tanggal_lahir), tanggal_lahir);
    penumpang_safe_copy(p.jenis_kelamin, sizeof(p.jenis_kelamin), jenis_kelamin);
    p.active = 1;

    g_penumpang[g_penumpangCount++] = p;
    penumpang_save();
}

void penumpang_create_auto(char *out_id, size_t out_sz,
                           const char* nama, const char* email,
                           const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin) {
    if (g_penumpangCount >= MAX_RECORDS) return;

    int next_n = penumpang_get_max_number() + 1;
    char id[16];
    format_pnp_id(next_n, id, sizeof(id));

    if (out_id && out_sz > 0) snprintf(out_id, out_sz, "%s", id);

    penumpang_create(id, nama, email, no_telp, tanggal_lahir, jenis_kelamin);
}
