#include <stdio.h>
#include "karyawan.h"
#include "karyawan_shared.h"

/* Soft delete */
void karyawan_delete(int index) {
    if (index >= 0 && index < g_karyawanCount) {
        g_karyawan[index].active = 0;
        karyawan_save();
    }
}
