#include <stdio.h>
#include "jadwal.h"
#include "jadwal_shared.h"

void jadwal_update(int index,
                   const char *id_kereta,
                   const char *id_stasiun_asal,
                   const char *id_stasiun_tujuan,
                   const char *waktu_berangkat,
                   const char *waktu_tiba,
                   int harga_tiket,
                   int kuota_kursi) {
    if (index < 0 || index >= g_jadwalCount) return;

    jadwal_safe_copy(g_jadwal[index].id_kereta, sizeof(g_jadwal[index].id_kereta), id_kereta);
    jadwal_safe_copy(g_jadwal[index].id_stasiun_asal, sizeof(g_jadwal[index].id_stasiun_asal), id_stasiun_asal);
    jadwal_safe_copy(g_jadwal[index].id_stasiun_tujuan, sizeof(g_jadwal[index].id_stasiun_tujuan), id_stasiun_tujuan);
    jadwal_safe_copy(g_jadwal[index].waktu_berangkat, sizeof(g_jadwal[index].waktu_berangkat), waktu_berangkat);
    jadwal_safe_copy(g_jadwal[index].waktu_tiba, sizeof(g_jadwal[index].waktu_tiba), waktu_tiba);
    g_jadwal[index].harga_tiket = harga_tiket;
    g_jadwal[index].kuota_kursi = kuota_kursi;

    jadwal_save();
}
