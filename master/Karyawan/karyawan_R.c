#include <stdio.h>
#include <string.h>
#include "karyawan.h"
#include "karyawan_shared.h"

/* Simpan seluruh array ke file */
void karyawan_save(void) {
    FILE *f = fopen(FILE_KARYAWAN, "wb");
    if (f) {
        fwrite(g_karyawan, sizeof(Karyawan), (size_t)g_karyawanCount, f);
        fclose(f);
    }
}

/* Load data (tanpa seed dummy) */
void karyawan_init(void) {
    FILE *f = fopen(FILE_KARYAWAN, "rb");
    if (f) {
        g_karyawanCount = (int)fread(g_karyawan, sizeof(Karyawan), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_karyawanCount = 0;
    }
}
