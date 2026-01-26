#include <stdio.h>
#include "stasiun.h"
#include "stasiun_shared.h"

void stasiun_update(int index, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat, const char* status) {
    if (index < 0 || index >= g_stasiunCount) return;
    if (!g_stasiun[index].active) return;

    if (kode) stasiun_safe_copy(g_stasiun[index].kode, sizeof(g_stasiun[index].kode), kode);
    if (nama) stasiun_safe_copy(g_stasiun[index].nama, sizeof(g_stasiun[index].nama), nama);
    if (kota) stasiun_safe_copy(g_stasiun[index].kota, sizeof(g_stasiun[index].kota), kota);
    if (alamat) stasiun_safe_copy(g_stasiun[index].alamat, sizeof(g_stasiun[index].alamat), alamat);
    if (status) stasiun_safe_copy(g_stasiun[index].status, sizeof(g_stasiun[index].status), status);
    g_stasiun[index].mdpl = mdpl;

    stasiun_save();
}
