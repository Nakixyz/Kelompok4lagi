#include <stdio.h>
#include <string.h>
#include "modul_kereta.h"
#include "globals.h"

#define FILE_KERETA "kereta.dat"

/* Versi lama untuk migrasi */
typedef struct {
    char kode[10];
    char nama[50];
    char kelas[20];
    int gerbong;
    int active;
} KeretaV1;

static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

static int parse_ka_number(const char *kode) {
    if (!kode) return -1;
    if (strncmp(kode, "KA", 2) != 0) return -1;
    int n = 0;
    if (sscanf(kode + 2, "%d", &n) != 1) return -1;
    return n;
}

static int kereta_get_max_number() {
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

static void kereta_save() {
    FILE *f = fopen(FILE_KERETA, "wb");
    if (f) {
        fwrite(g_kereta, sizeof(Kereta), (size_t)g_keretaCount, f);
        fclose(f);
    }
}

static long file_size(FILE *f) {
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, cur, SEEK_SET);
    return sz;
}

void kereta_init() {
    FILE *f = fopen(FILE_KERETA, "rb");
    if (f) {
        long sz = file_size(f);
        rewind(f);

        if (sz % (long)sizeof(Kereta) == 0) {
            g_keretaCount = (int)fread(g_kereta, sizeof(Kereta), MAX_RECORDS, f);
        } else if (sz % (long)sizeof(KeretaV1) == 0) {
            int count = (int)(sz / (long)sizeof(KeretaV1));
            if (count > MAX_RECORDS) count = MAX_RECORDS;
            KeretaV1 tmp[MAX_RECORDS];
            int readn = (int)fread(tmp, sizeof(KeretaV1), (size_t)count, f);

            g_keretaCount = 0;
            for (int i = 0; i < readn && g_keretaCount < MAX_RECORDS; i++) {
                Kereta k;
                memset(&k, 0, sizeof(k));
                safe_copy(k.kode, sizeof(k.kode), tmp[i].kode);
                safe_copy(k.nama, sizeof(k.nama), tmp[i].nama);
                safe_copy(k.kelas, sizeof(k.kelas), tmp[i].kelas);
                k.gerbong = tmp[i].gerbong;
                k.kapasitas = tmp[i].gerbong * 80; /* default */
                safe_copy(k.status, sizeof(k.status), tmp[i].active ? "Aktif" : "Nonaktif");
                k.active = tmp[i].active;
                g_kereta[g_keretaCount++] = k;
            }
            kereta_save();
        } else {
            g_keretaCount = 0;
        }

        fclose(f);
    } else {
        g_keretaCount = 0;
    }

    /* Seed 50 Data Dummy */
    if (g_keretaCount == 0) {
        const char *kelas_list[] = {"Eksekutif", "Bisnis", "Ekonomi"};

        for (int i = 1; i <= 50 && g_keretaCount < MAX_RECORDS; i++) {
            Kereta k;
            memset(&k, 0, sizeof(k));
            snprintf(k.kode, sizeof(k.kode), "KA%03d", i);
            snprintf(k.nama, sizeof(k.nama), "Argo %02d", i);

            safe_copy(k.kelas, sizeof(k.kelas), kelas_list[i % 3]);

            k.gerbong = 6 + (i % 7);
            k.kapasitas = k.gerbong * 80;
            safe_copy(k.status, sizeof(k.status), "Aktif");
            k.active = 1;

            g_kereta[g_keretaCount++] = k;
        }
        kereta_save();
    }
}

void kereta_create(const char* kode, const char* nama, const char* kelas,
                   int kapasitas, int gerbong, const char* status) {
    if (g_keretaCount >= MAX_RECORDS) return;

    Kereta k;
    memset(&k, 0, sizeof(k));

    safe_copy(k.kode, sizeof(k.kode), kode);
    safe_copy(k.nama, sizeof(k.nama), nama);
    safe_copy(k.kelas, sizeof(k.kelas), kelas);
    k.kapasitas = kapasitas;
    k.gerbong = gerbong;
    safe_copy(k.status, sizeof(k.status), status);
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

void kereta_update(int index, const char* nama, const char* kelas,
                   int kapasitas, int gerbong, const char* status) {
    if (index < 0 || index >= g_keretaCount) return;
    if (!g_kereta[index].active) return;

    if (nama) safe_copy(g_kereta[index].nama, sizeof(g_kereta[index].nama), nama);
    if (kelas) safe_copy(g_kereta[index].kelas, sizeof(g_kereta[index].kelas), kelas);
    if (status) safe_copy(g_kereta[index].status, sizeof(g_kereta[index].status), status);
    g_kereta[index].kapasitas = kapasitas;
    g_kereta[index].gerbong = gerbong;

    kereta_save();
}

void kereta_delete(int index) {
    if (index >= 0 && index < g_keretaCount) {
        g_kereta[index].active = 0;
        safe_copy(g_kereta[index].status, sizeof(g_kereta[index].status), "Nonaktif");
        kereta_save();
    }
}
