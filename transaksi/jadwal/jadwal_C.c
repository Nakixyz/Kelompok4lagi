#include <stdio.h>
#include <string.h>
#include "jadwal.h"
#include "jadwal_shared.h"

/* internal helper dari jadwal_R.c */
int  jadwal__get_next_number(void);
void jadwal__format_id(int n, char *out, size_t out_sz);

void jadwal_create_auto(char *out_id, size_t out_sz,
                        const char *tgl_dibuat,
                        const char *id_kereta,
                        const char *id_stasiun_asal,
                        const char *id_stasiun_tujuan,
                        const char *waktu_berangkat,
                        const char *waktu_tiba,
                        int harga_tiket,
                        int kuota_kursi,
                        const char *id_karyawan) {
    if (g_jadwalCount >= MAX_RECORDS) return;

    int next_n = jadwal__get_next_number();
    char id_new[20];
    jadwal__format_id(next_n, id_new, sizeof(id_new));
    if (out_id && out_sz > 0) snprintf(out_id, out_sz, "%s", id_new);

    JadwalTiket j;
    memset(&j, 0, sizeof(j));
    jadwal_safe_copy(j.id_jadwal, sizeof(j.id_jadwal), id_new);

    // FIX: simpan tanggal dibuat (sebelumnya terabaikan)
    jadwal_safe_copy(j.tgl_dibuat, sizeof(j.tgl_dibuat), tgl_dibuat);
    jadwal_safe_copy(j.id_kereta, sizeof(j.id_kereta), id_kereta);
    jadwal_safe_copy(j.id_stasiun_asal, sizeof(j.id_stasiun_asal), id_stasiun_asal);
    jadwal_safe_copy(j.id_stasiun_tujuan, sizeof(j.id_stasiun_tujuan), id_stasiun_tujuan);
    jadwal_safe_copy(j.waktu_berangkat, sizeof(j.waktu_berangkat), waktu_berangkat);
    jadwal_safe_copy(j.waktu_tiba, sizeof(j.waktu_tiba), waktu_tiba);
    j.harga_tiket = harga_tiket;
    j.kuota_kursi = kuota_kursi;
    jadwal_safe_copy(j.id_karyawan, sizeof(j.id_karyawan), id_karyawan);
    j.active = 1;

    g_jadwal[g_jadwalCount++] = j;
    jadwal_save();
}
