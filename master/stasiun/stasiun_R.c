#include <stdio.h>
#include <string.h>
#include "stasiun.h"
#include "stasiun_shared.h"

/* Versi lama (sebelum penambahan field) untuk migrasi */
typedef struct {
    char id[10];
    char nama[50];
    char kota[50];
    int active;
} StasiunV1;

static long file_size(FILE *f) {
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, cur, SEEK_SET);
    return sz;
}

void stasiun_save(void) {
    FILE *f = fopen(FILE_STASIUN, "wb");
    if (f) {
        fwrite(g_stasiun, sizeof(Stasiun), (size_t)g_stasiunCount, f);
        fclose(f);
    }
}

void stasiun_init(void) {
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
                stasiun_safe_copy(s.id, sizeof(s.id), tmp[i].id);
                stasiun_safe_copy(s.nama, sizeof(s.nama), tmp[i].nama);
                stasiun_safe_copy(s.kota, sizeof(s.kota), tmp[i].kota);
                /* field baru: isi default */
                stasiun_safe_copy(s.kode, sizeof(s.kode), "");
                s.mdpl = 0;
                stasiun_safe_copy(s.alamat, sizeof(s.alamat), "");
                stasiun_safe_copy(s.status, sizeof(s.status), tmp[i].active ? "Aktif" : "Nonaktif");
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

    /* Tidak ada seed dummy. Data kosong tetap dibiarkan kosong. */
}
