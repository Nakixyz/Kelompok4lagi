#include <stdio.h>
#include <string.h>
#include "modul_karyawan.h"
#include "globals.h"

#define FILE_KARYAWAN "karyawan.dat"
static int parse_peg_number(const char *id) {
    // Terima format: PEG001, PEG002, dst (case-sensitive "PEG")
    if (!id) return -1;
    if (strncmp(id, "PEG", 3) != 0) return -1;

    int n = 0;
    // baca angka setelah PEG
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int karyawan_get_max_peg_number() {
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

/* Helper aman untuk copy string (selalu null-terminate) */
static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

/* Simpan seluruh array ke file */
static void karyawan_save() {
    FILE *f = fopen(FILE_KARYAWAN, "wb");
    if (f) {
        fwrite(g_karyawan, sizeof(Karyawan), (size_t)g_karyawanCount, f);
        fclose(f);
    }
}

/* Load data atau seed dummy */
void karyawan_init() {
    FILE *f = fopen(FILE_KARYAWAN, "rb");
    if (f) {
        g_karyawanCount = (int)fread(g_karyawan, sizeof(Karyawan), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_karyawanCount = 0;
    }

    /* Seed data dummy jika kosong */
    if (g_karyawanCount == 0) {
        for (int i = 1; i <= 50 && g_karyawanCount < MAX_RECORDS; i++) {
            Karyawan k;
            snprintf(k.id, sizeof(k.id), "PEG%03d", i);
            snprintf(k.nama, sizeof(k.nama), "Karyawan %02d", i);
            snprintf(k.email, sizeof(k.email), "user%02d@kai.id", i);
            snprintf(k.jabatan, sizeof(k.jabatan), (i % 2 == 0) ? "Staff" : "Supervisor");
            k.active = 1;

            g_karyawan[g_karyawanCount++] = k;
        }
        karyawan_save();
    }
}

/* Tambah data */
void karyawan_create(const char* id, const char* nama, const char* email, const char* jabatan) {
    if (g_karyawanCount >= MAX_RECORDS) return;

    Karyawan k;
    safe_copy(k.id, sizeof(k.id), id);
    safe_copy(k.nama, sizeof(k.nama), nama);
    safe_copy(k.email, sizeof(k.email), email);
    safe_copy(k.jabatan, sizeof(k.jabatan), jabatan);
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

/* Soft delete */
void karyawan_delete(int index) {
    if (index >= 0 && index < g_karyawanCount) {
        g_karyawan[index].active = 0;
        karyawan_save();
    }
}

/* Edit data */
void karyawan_update(int index, const char* nama, const char* email, const char* jabatan) {
    if (index < 0 || index >= g_karyawanCount) return;
    if (!g_karyawan[index].active) return;

    if (nama)    safe_copy(g_karyawan[index].nama, sizeof(g_karyawan[index].nama), nama);
    if (email)   safe_copy(g_karyawan[index].email, sizeof(g_karyawan[index].email), email);
    if (jabatan) safe_copy(g_karyawan[index].jabatan, sizeof(g_karyawan[index].jabatan), jabatan);

    karyawan_save();
}
