#ifndef MODUL_LAPORAN_H
#define MODUL_LAPORAN_H

/*
    Modul Laporan (READ ONLY)

    Tujuan:
    - Semua proses laporan dipisahkan dari ui_app.c
    - Laporan hanya membaca data global (g_pembayaran, g_jadwal, dll)
    - Tidak ada CRUD / transaksi di sini
*/

#include "types.h"

/* =========================
   Struct hasil laporan
   ========================= */

typedef struct {
    int total_transaksi;
    int total_tiket;
    long total_pendapatan;
} LaporanRingkasan;

typedef struct {
    char label[64];        /* contoh: "TUNAI", "QRIS", "STS01->STS02", "KA01" */
    int jumlah_transaksi;
    int jumlah_tiket;
    long pendapatan;
} LaporanItem;

typedef struct {
    LaporanItem items[MAX_RECORDS];
    int count;
} LaporanList;


/* =========================
   API laporan (READ ONLY)
   ========================= */

/* Validasi string tanggal format: DD-MM-YYYY */
int laporan_validasi_tanggal(const char *ddmmyyyy);

/* Konversi DD-MM-YYYY -> int yyyymmdd (buat compare periode) */
int laporan_tanggal_to_int(const char *ddmmyyyy);

/*
  Ringkasan total transaksi/pemasukan dalam rentang tanggal.
  Filter dari PembayaranTiket.tgl_dibuat (format DD-MM-YYYY).
*/
void laporan_ringkasan_periode(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanRingkasan *out
);

/*
  Laporan pendapatan per metode pembayaran (TUNAI/DEBIT/QRIS).
  Dibaca dari PembayaranTiket.metode_pembayaran
*/
void laporan_per_metode_pembayaran(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanList *out
);

/*
  Laporan pendapatan per kereta (berdasarkan JadwalTiket.id_kereta).
  Pembayaran -> Jadwal -> Kereta.kode
*/
void laporan_per_kereta(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanList *out
);

/*
  Laporan pendapatan per rute (asal->tujuan).
  Pembayaran -> Jadwal -> id_stasiun_asal + id_stasiun_tujuan
*/
void laporan_per_rute(
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy,
    LaporanList *out
);

/* Optional: export laporan ke file txt (READ data, write output laporan) */
int laporan_export_txt(
    const char *filepath,
    const char *tgl_awal_ddmmyyyy,
    const char *tgl_akhir_ddmmyyyy
);

#endif
