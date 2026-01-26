#include "modul_laporan.h"
#include "globals.h"

#include <stdio.h>
#include <string.h>

/* ==========================================================
   Helper internal (private)
   ========================================================== */

static int in_range(int d, int start, int end) {
    return (d >= start && d <= end);
}

static int find_jadwal_index_by_id(const char *id_jadwal) {
    for (int i = 0; i < g_jadwalCount; i++) {
        if (g_jadwal[i].active && strcmp(g_jadwal[i].id_jadwal, id_jadwal) == 0)
            return i;
    }
    return -1;
}

static int find_or_add_item(LaporanList *list, const char *label) {
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].label, label) == 0) return i;
    }

    if (list->count >= MAX_RECORDS) return -1;

    strncpy(list->items[list->count].label, label, sizeof(list->items[list->count].label) - 1);
    list->items[list->count].label[sizeof(list->items[list->count].label) - 1] = '\0';

    list->items[list->count].jumlah_transaksi = 0;
    list->items[list->count].jumlah_tiket = 0;
    list->items[list->count].pendapatan = 0;

    list->count++;
    return list->count - 1;
}

/* ==========================================================
   Public functions
   ========================================================== */

int laporan_validasi_tanggal(const char *ddmmyyyy) {
    if (!ddmmyyyy) return 0;
    if (strlen(ddmmyyyy) != 10) return 0; /* DD-MM-YYYY */

    /* validasi bentuk dasar: dd-mm-yyyy */
    if (ddmmyyyy[2] != '-' || ddmmyyyy[5] != '-') return 0;

    int dd = 0, mm = 0, yy = 0;
    if (sscanf(ddmmyyyy, "%d-%d-%d", &dd, &mm, &yy) != 3) return 0;

    if (yy < 1900 || yy > 2100) return 0;
    if (mm < 1 || mm > 12) return 0;
    if (dd < 1 || dd > 31) return 0;

    /* NOTE:
       Kalau mau lebih akurat:
       - cek jumlah hari per bulan
       - leap year untuk Feb
    */
    return 1;
}

int laporan_tanggal_to_int(const char *ddmmyyyy) {
    int dd = 0, mm = 0, yy = 0;
    if (!laporan_validasi_tanggal(ddmmyyyy)) return -1;

    sscanf(ddmmyyyy, "%d-%d-%d", &dd, &mm, &yy);
    return (yy * 10000) + (mm * 100) + dd; /* YYYYMMDD */
}

void laporan_ringkasan_periode(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanRingkasan *out
) {
    if (!out) return;

    out->total_transaksi = 0;
    out->total_tiket = 0;
    out->total_pendapatan = 0;

    int start = laporan_tanggal_to_int(tgl_awal_ddmmyyyy);
    int end   = laporan_tanggal_to_int(tgl_akhir_ddmmyyyy);
    if (start < 0 || end < 0) return;

    for (int i = 0; i < g_pembayaranCount; i++) {
        PembayaranTiket *p = &g_pembayaran[i];
        if (!p->active) continue;

        /* contoh filter (opsional): hanya LUNAS */
        /* if (strcmp(p->status_pembayaran, "LUNAS") != 0) continue; */

        int d = laporan_tanggal_to_int(p->tgl_dibuat);
        if (d < 0) continue;

        if (in_range(d, start, end)) {
            out->total_transaksi++;
            out->total_tiket += p->jumlah_tiket;
            out->total_pendapatan += p->total_harga;
        }
    }
}

void laporan_per_metode_pembayaran(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanList *out
) {
    if (!out) return;
    out->count = 0;

    int start = laporan_tanggal_to_int(tgl_awal_ddmmyyyy);
    int end   = laporan_tanggal_to_int(tgl_akhir_ddmmyyyy);
    if (start < 0 || end < 0) return;

    for (int i = 0; i < g_pembayaranCount; i++) {
        PembayaranTiket *p = &g_pembayaran[i];
        if (!p->active) continue;

        int d = laporan_tanggal_to_int(p->tgl_dibuat);
        if (d < 0 || !in_range(d, start, end)) continue;

        int idx = find_or_add_item(out, p->metode_pembayaran);
        if (idx < 0) continue;

        out->items[idx].jumlah_transaksi++;
        out->items[idx].jumlah_tiket += p->jumlah_tiket;
        out->items[idx].pendapatan += p->total_harga;
    }
}

void laporan_per_kereta(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanList *out
) {
    if (!out) return;
    out->count = 0;

    int start = laporan_tanggal_to_int(tgl_awal_ddmmyyyy);
    int end   = laporan_tanggal_to_int(tgl_akhir_ddmmyyyy);
    if (start < 0 || end < 0) return;

    for (int i = 0; i < g_pembayaranCount; i++) {
        PembayaranTiket *p = &g_pembayaran[i];
        if (!p->active) continue;

        int d = laporan_tanggal_to_int(p->tgl_dibuat);
        if (d < 0 || !in_range(d, start, end)) continue;

        int jidx = find_jadwal_index_by_id(p->id_jadwal);
        if (jidx < 0) continue;

        /* label = kode kereta dari jadwal */
        const char *kodeKereta = g_jadwal[jidx].id_kereta;

        int idx = find_or_add_item(out, kodeKereta);
        if (idx < 0) continue;

        out->items[idx].jumlah_transaksi++;
        out->items[idx].jumlah_tiket += p->jumlah_tiket;
        out->items[idx].pendapatan += p->total_harga;
    }
}

void laporan_per_rute(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanList *out
) {
    if (!out) return;
    out->count = 0;

    int start = laporan_tanggal_to_int(tgl_awal_ddmmyyyy);
    int end   = laporan_tanggal_to_int(tgl_akhir_ddmmyyyy);
    if (start < 0 || end < 0) return;

    for (int i = 0; i < g_pembayaranCount; i++) {
        PembayaranTiket *p = &g_pembayaran[i];
        if (!p->active) continue;

        int d = laporan_tanggal_to_int(p->tgl_dibuat);
        if (d < 0 || !in_range(d, start, end)) continue;

        int jidx = find_jadwal_index_by_id(p->id_jadwal);
        if (jidx < 0) continue;

        char label[64];
        snprintf(label, sizeof(label), "%s->%s",
                 g_jadwal[jidx].id_stasiun_asal,
                 g_jadwal[jidx].id_stasiun_tujuan);

        int idx = find_or_add_item(out, label);
        if (idx < 0) continue;

        out->items[idx].jumlah_transaksi++;
        out->items[idx].jumlah_tiket += p->jumlah_tiket;
        out->items[idx].pendapatan += p->total_harga;
    }
}

int laporan_export_txt(
    const char *filepath,
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy
) {
    if (!filepath) return 0;

    FILE *f = fopen(filepath, "w");
    if (!f) return 0;

    LaporanRingkasan ringkas;
    LaporanList metode, kereta, rute;

    laporan_ringkasan_periode(tgl_awal_ddmmyyyy, tgl_akhir_ddmmyyyy, &ringkas);
    laporan_per_metode_pembayaran(tgl_awal_ddmmyyyy, tgl_akhir_ddmmyyyy, &metode);
    laporan_per_kereta(tgl_awal_ddmmyyyy, tgl_akhir_ddmmyyyy, &kereta);
    laporan_per_rute(tgl_awal_ddmmyyyy, tgl_akhir_ddmmyyyy, &rute);

    fprintf(f, "=== LAPORAN PERIODE %s s/d %s ===\n\n", tgl_awal_ddmmyyyy, tgl_akhir_ddmmyyyy);
    fprintf(f, "Ringkasan:\n");
    fprintf(f, "- Total transaksi : %d\n", ringkas.total_transaksi);
    fprintf(f, "- Total tiket     : %d\n", ringkas.total_tiket);
    fprintf(f, "- Total pendapatan: %ld\n\n", ringkas.total_pendapatan);

    fprintf(f, "Pendapatan per Metode:\n");
    for (int i = 0; i < metode.count; i++) {
        fprintf(f, "- %s | trx=%d | tiket=%d | pendapatan=%ld\n",
                metode.items[i].label,
                metode.items[i].jumlah_transaksi,
                metode.items[i].jumlah_tiket,
                metode.items[i].pendapatan);
    }

    fprintf(f, "\nPendapatan per Kereta:\n");
    for (int i = 0; i < kereta.count; i++) {
        fprintf(f, "- %s | trx=%d | tiket=%d | pendapatan=%ld\n",
                kereta.items[i].label,
                kereta.items[i].jumlah_transaksi,
                kereta.items[i].jumlah_tiket,
                kereta.items[i].pendapatan);
    }

    fprintf(f, "\nPendapatan per Rute:\n");
    for (int i = 0; i < rute.count; i++) {
        fprintf(f, "- %s | trx=%d | tiket=%d | pendapatan=%ld\n",
                rute.items[i].label,
                rute.items[i].jumlah_transaksi,
                rute.items[i].jumlah_tiket,
                rute.items[i].pendapatan);
    }

    fclose(f);
    return 1;
}
