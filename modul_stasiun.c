#include <stdio.h>
#include <string.h>
#include "modul_stasiun.h"
#include "globals.h"

#define FILE_STASIUN "stasiun.dat"

/* Versi lama (sebelum penambahan field) untuk migrasi */
typedef struct {
    char id[10];
    char nama[50];
    char kota[50];
    int active;
} StasiunV1;

static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, dst_size, "%s", src);
}

static int parse_sts_number(const char *id) {
    if (!id) return -1;
    if (strncmp(id, "STS", 3) != 0) return -1;
    int n = 0;
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int stasiun_get_max_number() {
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

static void stasiun_save() {
    FILE *f = fopen(FILE_STASIUN, "wb");
    if (f) {
        fwrite(g_stasiun, sizeof(Stasiun), (size_t)g_stasiunCount, f);
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

void stasiun_init() {
    FILE *f = fopen(FILE_STASIUN, "rb");
    if (f) {
        long sz = file_size(f);
        rewind(f);

        if (sz % (long)sizeof(Stasiun) == 0) {
            g_stasiunCount = (int)fread(g_stasiun, sizeof(Stasiun), MAX_RECORDS, f);
        } else if (sz % (long)sizeof(StasiunV1) == 0) {
            int count = (int)(sz / (long)sizeof(StasiunV1));
            if (count > MAX_RECORDS) count = MAX_RECORDS;
            StasiunV1 tmp[MAX_RECORDS];
            int readn = (int)fread(tmp, sizeof(StasiunV1), (size_t)count, f);

            g_stasiunCount = 0;
            for (int i = 0; i < readn && g_stasiunCount < MAX_RECORDS; i++) {
                Stasiun s;
                memset(&s, 0, sizeof(s));
                safe_copy(s.id, sizeof(s.id), tmp[i].id);
                safe_copy(s.nama, sizeof(s.nama), tmp[i].nama);
                safe_copy(s.kota, sizeof(s.kota), tmp[i].kota);
                /* field baru: isi default */
                safe_copy(s.kode, sizeof(s.kode), "");
                s.mdpl = 0;
                safe_copy(s.alamat, sizeof(s.alamat), "");
                safe_copy(s.status, sizeof(s.status), tmp[i].active ? "Aktif" : "Nonaktif");
                s.active = tmp[i].active;
                g_stasiun[g_stasiunCount++] = s;
            }
            /* simpan ulang ke format baru */
            stasiun_save();
        } else {
            /* format tidak dikenal: anggap kosong */
            g_stasiunCount = 0;
        }

        fclose(f);
    } else {
        g_stasiunCount = 0;
    }

    /* Seed 50 Data Dummy bila kosong */
    if (g_stasiunCount == 0) {
        for (int i = 1; i <= 50 && g_stasiunCount < MAX_RECORDS; i++) {
            Stasiun s;
            memset(&s, 0, sizeof(s));
            snprintf(s.id, sizeof(s.id), "STS%03d", i);
            snprintf(s.kode, sizeof(s.kode), "K%03d", i);
            snprintf(s.nama, sizeof(s.nama), "Stasiun Kota %02d", i);

            char kotaChar = 'A' + (i % 5);
            snprintf(s.kota, sizeof(s.kota), "Kota %c", kotaChar);

            s.mdpl = 10 + (i % 90);
            snprintf(s.alamat, sizeof(s.alamat), "Jalan Stasiun No.%d", i);
            snprintf(s.status, sizeof(s.status), "Aktif");
            s.active = 1;

            g_stasiun[g_stasiunCount++] = s;
        }
        stasiun_save();
    }
}

void stasiun_create(const char* id, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat, const char* status) {
    if (g_stasiunCount >= MAX_RECORDS) return;

    Stasiun s;
    memset(&s, 0, sizeof(s));

    safe_copy(s.id, sizeof(s.id), id);
    safe_copy(s.kode, sizeof(s.kode), kode);
    safe_copy(s.nama, sizeof(s.nama), nama);
    s.mdpl = mdpl;
    safe_copy(s.kota, sizeof(s.kota), kota);
    safe_copy(s.alamat, sizeof(s.alamat), alamat);
    safe_copy(s.status, sizeof(s.status), status);
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

void stasiun_update(int index, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat, const char* status) {
    if (index < 0 || index >= g_stasiunCount) return;
    if (!g_stasiun[index].active) return;

    if (kode) safe_copy(g_stasiun[index].kode, sizeof(g_stasiun[index].kode), kode);
    if (nama) safe_copy(g_stasiun[index].nama, sizeof(g_stasiun[index].nama), nama);
    if (kota) safe_copy(g_stasiun[index].kota, sizeof(g_stasiun[index].kota), kota);
    if (alamat) safe_copy(g_stasiun[index].alamat, sizeof(g_stasiun[index].alamat), alamat);
    if (status) safe_copy(g_stasiun[index].status, sizeof(g_stasiun[index].status), status);
    g_stasiun[index].mdpl = mdpl;

    stasiun_save();
}

void stasiun_delete(int index) {
    if (index >= 0 && index < g_stasiunCount) {
        g_stasiun[index].active = 0;
        safe_copy(g_stasiun[index].status, sizeof(g_stasiun[index].status), "Nonaktif");
        stasiun_save();
    }
}
