#include <stdio.h>
#include "penumpang.h"
#include "penumpang_shared.h"

void penumpang_update(int index, const char* nama, const char* email,
                      const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin) {
    if (index < 0 || index >= g_penumpangCount) return;
    if (!g_penumpang[index].active) return;

    if (nama) penumpang_safe_copy(g_penumpang[index].nama, sizeof(g_penumpang[index].nama), nama);
    if (email) penumpang_safe_copy(g_penumpang[index].email, sizeof(g_penumpang[index].email), email);
    if (no_telp) penumpang_safe_copy(g_penumpang[index].no_telp, sizeof(g_penumpang[index].no_telp), no_telp);
    if (tanggal_lahir) penumpang_safe_copy(g_penumpang[index].tanggal_lahir, sizeof(g_penumpang[index].tanggal_lahir), tanggal_lahir);
    if (jenis_kelamin) penumpang_safe_copy(g_penumpang[index].jenis_kelamin, sizeof(g_penumpang[index].jenis_kelamin), jenis_kelamin);

    penumpang_save();
}
