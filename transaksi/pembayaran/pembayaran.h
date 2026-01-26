#ifndef TRANSAKSI_PEMBAYARAN_H
#define TRANSAKSI_PEMBAYARAN_H

#include <stddef.h>

/* Pembayaran Tiket (CRD - tanpa Update).
   Primary key = ID transaksi (mis. TRX001) yang dibuat otomatis oleh sistem.
   Tanggal dibuat tetap disimpan di tgl_pembayaran (YYYYMMDDHHMMSS). */
void pembayaran_init(void);
int  pembayaran_find_index_by_id(const char *id_pembayaran);

/* Return 1 jika sukses, 0 jika gagal (err akan diisi). */
int  pembayaran_create_auto(char *out_id, size_t out_sz,
                            const char *tgl_dibuat,
                            const char *id_penumpang,
                            const char *id_jadwal,
                            int jumlah_tiket,
                            const char *metode_pembayaran,
                            const char *status_pembayaran,
                            const char *id_karyawan,
                            char *err, size_t err_sz);

void pembayaran_delete(int index);

#endif
