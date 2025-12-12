#include <stdio.h>
#include <string.h>
#include "modul_kereta.h"
#include "globals.h"

#define FILE_KERETA "kereta.dat"

static void kereta_save() {
    FILE *f = fopen(FILE_KERETA, "wb");
    if (f) {
        fwrite(g_kereta, sizeof(Kereta), g_keretaCount, f);
        fclose(f);
    }
}

void kereta_init() {
    FILE *f = fopen(FILE_KERETA, "rb");
    if (f) {
        g_keretaCount = fread(g_kereta, sizeof(Kereta), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_keretaCount = 0;
    }

    /* Seed 50 Data Dummy */
    if (g_keretaCount == 0) {
        const char *kelas_list[] = {"Eksekutif", "Bisnis", "Ekonomi"};

        for (int i = 1; i <= 50; i++) {
            Kereta k;
            snprintf(k.kode, sizeof(k.kode), "KA%03d", i);
            snprintf(k.nama, sizeof(k.nama), "Argo %02d", i);

            // Rotasi kelas
            strncpy(k.kelas, kelas_list[i % 3], sizeof(k.kelas));

            // Random gerbong antara 6-12
            k.gerbong = 6 + (i % 7);
            k.active = 1;

            g_kereta[g_keretaCount++] = k;
        }
        kereta_save();
    }
}

void kereta_create(const char* kode, const char* nama, const char* kelas, int gerbong) {
    if (g_keretaCount >= MAX_RECORDS) return;

    Kereta k;
    strncpy(k.kode, kode, sizeof(k.kode));
    strncpy(k.nama, nama, sizeof(k.nama));
    strncpy(k.kelas, kelas, sizeof(k.kelas));
    k.gerbong = gerbong;
    k.active = 1;

    g_kereta[g_keretaCount++] = k;
    kereta_save();
}

void kereta_delete(int index) {
    if (index >= 0 && index < g_keretaCount) {
        g_kereta[index].active = 0;
        kereta_save();
    }
}