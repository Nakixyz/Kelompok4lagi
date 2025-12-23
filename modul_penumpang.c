#include <stdio.h>
#include <string.h>
#include "modul_penumpang.h"
#include "globals.h"

#define FILE_PENUMPANG "penumpang.dat"

static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

static int parse_pnp_number(const char *id) {
    if (!id) return -1;
    if (strncmp(id, "PNP", 3) != 0) return -1;
    int n = 0;
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int penumpang_get_max_number() {
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

static void penumpang_save() {
    FILE *f = fopen(FILE_PENUMPANG, "wb");
    if (f) {
        fwrite(g_penumpang, sizeof(Penumpang), (size_t)g_penumpangCount, f);
        fclose(f);
    }
}

void penumpang_init() {
    FILE *f = fopen(FILE_PENUMPANG, "rb");
    if (f) {
        g_penumpangCount = (int)fread(g_penumpang, sizeof(Penumpang), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_penumpangCount = 0;
    }

    /* Seed dummy bila kosong */
    if (g_penumpangCount == 0) {
        for (int i = 1; i <= 50 && g_penumpangCount < MAX_RECORDS; i++) {
            Penumpang p;
            snprintf(p.id, sizeof(p.id), "PNP%03d", i);
            snprintf(p.nama, sizeof(p.nama), "Penumpang %02d", i);
            snprintf(p.email, sizeof(p.email), "penumpang%02d@gmail.com", i);
            snprintf(p.no_telp, sizeof(p.no_telp), "08%09d", 100000000 + i);
            snprintf(p.tanggal_lahir, sizeof(p.tanggal_lahir), "01-01-2000");
            snprintf(p.jenis_kelamin, sizeof(p.jenis_kelamin), "%s", (i % 2 == 0) ? "L" : "P");
            p.active = 1;
            g_penumpang[g_penumpangCount++] = p;
        }
        penumpang_save();
    }
}

void penumpang_create(const char* id, const char* nama, const char* email,
                      const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin) {
    if (g_penumpangCount >= MAX_RECORDS) return;

    Penumpang p;
    safe_copy(p.id, sizeof(p.id), id);
    safe_copy(p.nama, sizeof(p.nama), nama);
    safe_copy(p.email, sizeof(p.email), email);
    safe_copy(p.no_telp, sizeof(p.no_telp), no_telp);
    safe_copy(p.tanggal_lahir, sizeof(p.tanggal_lahir), tanggal_lahir);
    safe_copy(p.jenis_kelamin, sizeof(p.jenis_kelamin), jenis_kelamin);
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

void penumpang_update(int index, const char* nama, const char* email,
                      const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin) {
    if (index < 0 || index >= g_penumpangCount) return;
    if (!g_penumpang[index].active) return;

    if (nama) safe_copy(g_penumpang[index].nama, sizeof(g_penumpang[index].nama), nama);
    if (email) safe_copy(g_penumpang[index].email, sizeof(g_penumpang[index].email), email);
    if (no_telp) safe_copy(g_penumpang[index].no_telp, sizeof(g_penumpang[index].no_telp), no_telp);
    if (tanggal_lahir) safe_copy(g_penumpang[index].tanggal_lahir, sizeof(g_penumpang[index].tanggal_lahir), tanggal_lahir);
    if (jenis_kelamin) safe_copy(g_penumpang[index].jenis_kelamin, sizeof(g_penumpang[index].jenis_kelamin), jenis_kelamin);

    penumpang_save();
}

void penumpang_delete(int index) {
    if (index >= 0 && index < g_penumpangCount) {
        g_penumpang[index].active = 0;
        penumpang_save();
    }
}
