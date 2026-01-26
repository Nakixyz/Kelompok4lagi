#include <stdio.h>
#include <string.h>
#include "kereta.h"
#include "kereta_shared.h"

void kereta_update(int index, const char* nama, const char* kelas,
                   int kapasitas, int gerbong, const char* status) {
    if (index < 0 || index >= g_keretaCount) return;
    if (!g_kereta[index].active) return;

    if (nama) kereta_safe_copy(g_kereta[index].nama, sizeof(g_kereta[index].nama), nama);

    if (kelas) {
        char kn[32];
        if (kereta_normalize_kelas(kn, sizeof(kn), kelas)) kereta_safe_copy(g_kereta[index].kelas, sizeof(g_kereta[index].kelas), kn);
    }

    if (status) {
        char sn[32];
        if (kereta_normalize_status_operasional(sn, sizeof(sn), status)) kereta_safe_copy(g_kereta[index].status, sizeof(g_kereta[index].status), sn);
    }

    g_kereta[index].kapasitas = kapasitas;
    g_kereta[index].gerbong = gerbong;

    kereta_save();
}
