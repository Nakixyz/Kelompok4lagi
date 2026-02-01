#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pembayaran.h"
#include "pembayaran_shared.h"

/* internal from pembayaran_R.c */
int pembayaran__penumpang_idx(const char *id);

static void now_datetime_pk(char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    if (!tmv) { snprintf(out, out_sz, "00000000000000"); return; }

    snprintf(out, out_sz, "%04d%02d%02d%02d%02d%02d",
             tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
             tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
}

static void now_date_ddmmyyyy(char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    if (!tmv) { out[0] = '\0'; return; }
    snprintf(out, out_sz, "%02d-%02d-%04d",
             tmv->tm_mday, tmv->tm_mon + 1, tmv->tm_year + 1900);
}

/* Generate ID transaksi pembayaran: TRX### (auto increment). */
static int trx_parse_num(const char *id) {
    if (!id) return -1;
    if (strncmp(id, "TRX", 3) != 0) return -1;
    const char *p = id + 3;
    if (!*p) return -1;
    int n = 0;
    while (*p) {
        if (*p < '0' || *p > '9') return -1;
        n = n * 10 + (*p - '0');
        p++;
    }
    return n;
}

static void pembayaran_next_id(char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;

    /* Cari nomor terkecil yang belum dipakai, mulai dari 1 (TRX001). */
    for (int cand = 1; cand <= 999; cand++) {
        int used = 0;
        for (int i = 0; i < g_pembayaranCount; i++) {
            int n = trx_parse_num(g_pembayaran[i].id_pembayaran);
            if (n == cand) { used = 1; break; }
        }
        if (!used) {
            snprintf(out, out_sz, "TRX%03d", cand);
            return;
        }
    }
    /* Fallback */
    snprintf(out, out_sz, "TRX%03d", g_pembayaranCount + 1);
}

int pembayaran_create_auto(char *out_id, size_t out_sz,
                           const char *tgl_dibuat,
                           const char *id_penumpang,
                           const char *id_jadwal,
                           int jumlah_tiket,
                           const char *metode_pembayaran,
                           const char *status_pembayaran,
                           const char *id_karyawan,
                           char *err, size_t err_sz) {
    if (err && err_sz > 0) err[0] = '\0';

    if (g_pembayaranCount >= MAX_RECORDS) {
        if (err && err_sz > 0) snprintf(err, err_sz, "Data pembayaran penuh.");
        return 0;
    }
    if (!id_penumpang || !id_jadwal || jumlah_tiket <= 0) {
        if (err && err_sz > 0) snprintf(err, err_sz, "Input tidak valid.");
        return 0;
    }

    int pidx = pembayaran__penumpang_idx(id_penumpang);
    if (pidx < 0 || !g_penumpang[pidx].active) {
        if (err && err_sz > 0) snprintf(err, err_sz, "Penumpang tidak ditemukan / nonaktif.");
        return 0;
    }

    int jidx = jadwal_find_index_by_id(id_jadwal);
    if (jidx < 0 || !g_jadwal[jidx].active) {
        if (err && err_sz > 0) snprintf(err, err_sz, "Jadwal tidak ditemukan / nonaktif.");
        return 0;
    }

    if (g_jadwal[jidx].kuota_kursi < jumlah_tiket) {
        if (err && err_sz > 0) snprintf(err, err_sz, "Kuota kursi tidak cukup. Sisa: %d", g_jadwal[jidx].kuota_kursi);
        return 0;
    }

    /* ID transaksi (PK) dibuat otomatis: TRX### */
    char idtrx[32];
    pembayaran_next_id(idtrx, sizeof(idtrx));

    /* tanggal transaksi tetap disimpan */
    char tglpk[32];
    now_datetime_pk(tglpk, sizeof(tglpk));

    /* guard unik (antisipasi file korup/duplikat) */
    int guard = 0;
    while (pembayaran_find_index_by_id(idtrx) >= 0 && guard < 1000) {
        int n = trx_parse_num(idtrx);
        if (n < 0) n = 0;
        snprintf(idtrx, sizeof(idtrx), "TRX%03d", n + 1);
        guard++;
    }

    if (out_id && out_sz > 0) snprintf(out_id, out_sz, "%s", idtrx);

    PembayaranTiket x;
    memset(&x, 0, sizeof(x));
    pembayaran_safe_copy(x.id_pembayaran, sizeof(x.id_pembayaran), idtrx);
    pembayaran_safe_copy(x.tgl_pembayaran, sizeof(x.tgl_pembayaran), tglpk);

    /* tgl_dibuat UI -> tetap simpan hanya tanggal hari ini (sesuai code lama) */
    {
        char tgl_only[16];
        (void)tgl_dibuat;
        now_date_ddmmyyyy(tgl_only, sizeof(tgl_only));
        pembayaran_safe_copy(x.tgl_dibuat, sizeof(x.tgl_dibuat), tgl_only);
    }

    pembayaran_safe_copy(x.id_penumpang, sizeof(x.id_penumpang), id_penumpang);
    pembayaran_safe_copy(x.id_jadwal, sizeof(x.id_jadwal), id_jadwal);

    x.jumlah_tiket = jumlah_tiket;
    x.harga_satuan = g_jadwal[jidx].harga_tiket;
    x.total_harga  = x.harga_satuan * x.jumlah_tiket;

    pembayaran_safe_copy(x.metode_pembayaran, sizeof(x.metode_pembayaran), metode_pembayaran ? metode_pembayaran : "TUNAI");
    pembayaran_safe_copy(x.status_pembayaran, sizeof(x.status_pembayaran), status_pembayaran ? status_pembayaran : "LUNAS");
    pembayaran_safe_copy(x.id_karyawan, sizeof(x.id_karyawan), id_karyawan);
    x.active = 1;

    /* update kuota */
    g_jadwal[jidx].kuota_kursi -= jumlah_tiket;

    g_pembayaran[g_pembayaranCount++] = x;

    /* simpan keduanya (pembayaran & jadwal) */
    pembayaran_save();
    jadwal_save_public();

    return 1;
}
