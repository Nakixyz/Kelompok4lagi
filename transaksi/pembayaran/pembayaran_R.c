#include <stdio.h>
#include <string.h>
#include "pembayaran.h"
#include "pembayaran_shared.h"

/* forward (didefinisikan di pembayaran_C.c) */
int pembayaran_create_auto(char *out_id, size_t out_sz,
                           const char *tgl_dibuat,
                           const char *id_penumpang,
                           const char *id_jadwal,
                           int jumlah_tiket,
                           const char *metode_pembayaran,
                           const char *status_pembayaran,
                           const char *id_karyawan,
                           char *err, size_t err_sz);

void pembayaran_save(void) {
    FILE *f = fopen(FILE_PEMBAYARAN, "wb");
    if (!f) return;
    fwrite(g_pembayaran, sizeof(PembayaranTiket), (size_t)g_pembayaranCount, f);
    fclose(f);
}

static int penumpang_find_index_by_id(const char *id) {
    if (!id) return -1;
    for (int i = 0; i < g_penumpangCount; i++) {
        if (strcmp(g_penumpang[i].id, id) == 0) return i;
    }
    return -1;
}


void pembayaran_init(void) {
    FILE *f = fopen(FILE_PEMBAYARAN, "rb");
    if (f) {
        g_pembayaranCount = (int)fread(g_pembayaran, sizeof(PembayaranTiket), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_pembayaranCount = 0;
    }

    
    /* MIGRASI DATA LAMA:
       Beberapa versi sebelumnya menyimpan file tanpa field 'active' atau nilainya selalu 0,
       sehingga record tidak tampil karena UI memfilter active==1.
       Jika setelah load tidak ada satupun record yang active==1, kita anggap data lama,
       lalu set active=1 untuk record yang ID-nya tidak kosong. */
    if (g_pembayaranCount > 0) {
        int any_active = 0;
        for (int i = 0; i < g_pembayaranCount; i++) {
            if (g_pembayaran[i].active == 1) { any_active = 1; break; }
        }
        if (!any_active) {
            for (int i = 0; i < g_pembayaranCount; i++) {
                if (g_pembayaran[i].id_pembayaran[0] != '\0') {
                    g_pembayaran[i].active = 1;
                }
            }
        }
    }

/* Migrasi kompatibilitas: data lama masih mungkin menyimpan status "MENUNGGU" */
    for (int i = 0; i < g_pembayaranCount; i++) {
        if (strcmp(g_pembayaran[i].status_pembayaran, "MENUNGGU") == 0) {
            strncpy(g_pembayaran[i].status_pembayaran, "MENUNGGU", sizeof(g_pembayaran[i].status_pembayaran)-1);
            g_pembayaran[i].status_pembayaran[sizeof(g_pembayaran[i].status_pembayaran)-1] = '\0';
        }
    }

}

int pembayaran_find_index_by_id(const char *id_pembayaran) {
    if (!id_pembayaran) return -1;
    for (int i = 0; i < g_pembayaranCount; i++) {
        if (strcmp(g_pembayaran[i].id_pembayaran, id_pembayaran) == 0) return i;
    }
    return -1;
}

/* expose penumpang lookup for create */
int pembayaran__penumpang_idx(const char *id) { return penumpang_find_index_by_id(id); }
