#include <stdio.h>
#include <string.h>
#include "karyawan.h"
#include "karyawan_shared.h"

static int parse_peg_number(const char *id) {
    // Terima format: PEG001, PEG002, dst (case-sensitive "PEG")
    if (!id) return -1;
    if (strncmp(id, "PEG", 3) != 0) return -1;

    int n = 0;
    // baca angka setelah PEG
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int karyawan_get_max_peg_number(void) {
    int max_n = 0;
    for (int i = 0; i < g_karyawanCount; i++) {
        int n = parse_peg_number(g_karyawan[i].id);
        if (n > max_n) max_n = n;
    }
    return max_n;
}

static void format_peg_id(int n, char *out_id, size_t out_sz) {
    // Default tetap PEG001..PEG999. Kalau >999 tetap aman: PEG1000 dst.
    if (!out_id || out_sz == 0) return;
    if (n >= 0 && n <= 999) snprintf(out_id, out_sz, "PEG%03d", n);
    else snprintf(out_id, out_sz, "PEG%d", n);
}

/* Tambah data */
void karyawan_create(const char* id, const char* nama, const char* email, const char* jabatan) {
    if (g_karyawanCount >= MAX_RECORDS) return;

    Karyawan k;
    karyawan_safe_copy(k.id, sizeof(k.id), id);
    karyawan_safe_copy(k.nama, sizeof(k.nama), nama);
    karyawan_safe_copy(k.email, sizeof(k.email), email);
    karyawan_safe_copy(k.jabatan, sizeof(k.jabatan), jabatan);
    k.active = 1;

    g_karyawan[g_karyawanCount++] = k;
    karyawan_save();
}

void karyawan_create_auto(char *out_id, size_t out_sz,
                          const char* nama, const char* email, const char* jabatan) {
    if (g_karyawanCount >= MAX_RECORDS) return;

    int next_n = karyawan_get_max_peg_number() + 1;

    char id[16];
    format_peg_id(next_n, id, sizeof(id));

    // kembalikan id ke caller (UI) bila diminta
    if (out_id && out_sz > 0) {
        snprintf(out_id, out_sz, "%s", id);
    }

    // reuse create biasa
    karyawan_create(id, nama, email, jabatan);
}
