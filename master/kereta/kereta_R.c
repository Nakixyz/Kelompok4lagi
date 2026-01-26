#include <stdio.h>
#include <string.h>
#include "kereta.h"
#include "kereta_shared.h"

/* Versi lama untuk migrasi */
typedef struct {
    char kode[10];
    char nama[50];
    char kelas[20];
    int gerbong;
    int active;
} KeretaV1;

static long file_size(FILE *f) {
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, cur, SEEK_SET);
    return sz;
}

void kereta_save(void) {
    FILE *f = fopen(FILE_KERETA, "wb");
    if (f) {
        fwrite(g_kereta, sizeof(Kereta), (size_t)g_keretaCount, f);
        fclose(f);
    }
}

void kereta_init(void) {
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
                kereta_safe_copy(k.kode, sizeof(k.kode), tmp[i].kode);
                kereta_safe_copy(k.nama, sizeof(k.nama), tmp[i].nama);
                kereta_safe_copy(k.kelas, sizeof(k.kelas), tmp[i].kelas);
                k.gerbong = tmp[i].gerbong;
                k.kapasitas = tmp[i].gerbong * 80; /* default */
                kereta_safe_copy(k.status, sizeof(k.status), "Aktif"); /* status operasional; soft delete pakai k.active */
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

    /* NORMALIZE_RECORDS: pastikan kelas & status operasional selalu sesuai aturan terbaru */
    {
        int changed = 0;
        for (int i = 0; i < g_keretaCount; i++) {
            char norm[32];

            if (!kereta_normalize_kelas(norm, sizeof(norm), g_kereta[i].kelas)) {
                kereta_safe_copy(g_kereta[i].kelas, sizeof(g_kereta[i].kelas), "Ekonomi");
                changed = 1;
            } else if (strcmp(g_kereta[i].kelas, norm) != 0) {
                kereta_safe_copy(g_kereta[i].kelas, sizeof(g_kereta[i].kelas), norm);
                changed = 1;
            }

            /* status lama "Nonaktif" tidak dipakai lagi; nonaktif gunakan field active */
            if (strcmp(g_kereta[i].status, "Nonaktif") == 0) {
                kereta_safe_copy(g_kereta[i].status, sizeof(g_kereta[i].status), "Aktif");
                changed = 1;
            } else if (!kereta_normalize_status_operasional(norm, sizeof(norm), g_kereta[i].status)) {
                kereta_safe_copy(g_kereta[i].status, sizeof(g_kereta[i].status), "Aktif");
                changed = 1;
            } else if (strcmp(g_kereta[i].status, norm) != 0) {
                kereta_safe_copy(g_kereta[i].status, sizeof(g_kereta[i].status), norm);
                changed = 1;
            }

            /* pastikan active hanya 0/1 */
            if (g_kereta[i].active != 0 && g_kereta[i].active != 1) {
                g_kereta[i].active = 1;
                changed = 1;
            }
        }
        if (changed) kereta_save();
    }

    /* Tidak ada seed dummy. Data kosong tetap dibiarkan kosong. */
}
