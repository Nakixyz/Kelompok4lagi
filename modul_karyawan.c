#include <stdio.h>
#include <string.h>
#include "modul_karyawan.h"
#include "globals.h"

#define FILE_KARYAWAN "karyawan.dat"

/* Fungsi Helper untuk Simpan ke File */
static void karyawan_save() {
    FILE *f = fopen(FILE_KARYAWAN, "wb");
    if (f) {
        fwrite(g_karyawan, sizeof(Karyawan), g_karyawanCount, f);
        fclose(f);
    }
}

void karyawan_init() {
    FILE *f = fopen(FILE_KARYAWAN, "rb");
    if (f) {
        g_karyawanCount = fread(g_karyawan, sizeof(Karyawan), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_karyawanCount = 0;
    }

    /* Seed 50 Data Dummy jika kosong */
    if (g_karyawanCount == 0) {
        for (int i = 1; i <= 50; i++) {
            Karyawan k;
            snprintf(k.id, sizeof(k.id), "PEG%03d", i);
            snprintf(k.nama, sizeof(k.nama), "Karyawan %02d", i);
            snprintf(k.email, sizeof(k.email), "user%02d@kai.id", i);

            if (i % 2 == 0) snprintf(k.jabatan, sizeof(k.jabatan), "Staff");
            else snprintf(k.jabatan, sizeof(k.jabatan), "Supervisor");

            k.active = 1;
            g_karyawan[g_karyawanCount++] = k;
        }
        karyawan_save();
    }
}

void karyawan_create(const char* id, const char* nama, const char* email, const char* jabatan) {
    if (g_karyawanCount >= MAX_RECORDS) return;

    Karyawan k;
    strncpy(k.id, id, sizeof(k.id));
    strncpy(k.nama, nama, sizeof(k.nama));
    strncpy(k.email, email, sizeof(k.email));
    strncpy(k.jabatan, jabatan, sizeof(k.jabatan));
    k.active = 1;

    g_karyawan[g_karyawanCount++] = k;
    karyawan_save();
}

void karyawan_delete(int index) {
    if (index >= 0 && index < g_karyawanCount) {
        g_karyawan[index].active = 0;
        karyawan_save();
    }
}
