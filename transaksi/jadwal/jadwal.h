#ifndef TRANSAKSI_JADWAL_H
#define TRANSAKSI_JADWAL_H

#include <stddef.h>

void jadwal_init(void);
int  jadwal_find_index_by_id(const char *id_jadwal);

/* Wrapper publik agar modul lain bisa mem-persist perubahan tanpa "hack" update. */
void jadwal_save_public(void);

void jadwal_create_auto(char *out_id, size_t out_sz,
                        const char *tgl_dibuat,
                        const char *id_kereta,
                        const char *id_stasiun_asal,
                        const char *id_stasiun_tujuan,
                        const char *waktu_berangkat,
                        const char *waktu_tiba,
                        int harga_tiket,
                        int kuota_kursi,
                        const char *id_karyawan);

void jadwal_update(int index,
                   const char *id_kereta,
                   const char *id_stasiun_asal,
                   const char *id_stasiun_tujuan,
                   const char *waktu_berangkat,
                   const char *waktu_tiba,
                   int harga_tiket,
                   int kuota_kursi);

void jadwal_delete(int index);

#endif
