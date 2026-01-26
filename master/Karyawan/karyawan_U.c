#include <stdio.h>
#include "karyawan.h"
#include "karyawan_shared.h"

/* Edit data */
void karyawan_update(int index, const char* nama, const char* email, const char* jabatan) {
    if (index < 0 || index >= g_karyawanCount) return;
    if (!g_karyawan[index].active) return;

    if (nama)    karyawan_safe_copy(g_karyawan[index].nama, sizeof(g_karyawan[index].nama), nama);
    if (email)   karyawan_safe_copy(g_karyawan[index].email, sizeof(g_karyawan[index].email), email);
    if (jabatan) karyawan_safe_copy(g_karyawan[index].jabatan, sizeof(g_karyawan[index].jabatan), jabatan);

    karyawan_save();
}
