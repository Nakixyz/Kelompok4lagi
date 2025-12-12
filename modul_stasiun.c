#include <stdio.h>
#include <string.h>
#include "modul_stasiun.h"
#include "globals.h"

#define FILE_STASIUN "stasiun.dat"

static void stasiun_save() {
    FILE *f = fopen(FILE_STASIUN, "wb");
    if (f) {
        fwrite(g_stasiun, sizeof(Stasiun), g_stasiunCount, f);
        fclose(f);
    }
}

void stasiun_init() {
    FILE *f = fopen(FILE_STASIUN, "rb");
    if (f) {
        g_stasiunCount = fread(g_stasiun, sizeof(Stasiun), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_stasiunCount = 0;
    }

    /* Seed 50 Data Dummy */
    if (g_stasiunCount == 0) {
        for (int i = 1; i <= 50; i++) {
            Stasiun s;
            snprintf(s.id, sizeof(s.id), "STS%03d", i);
            snprintf(s.nama, sizeof(s.nama), "Stasiun Kota %02d", i);

            // Kota A, Kota B, Kota C bergantian
            char kotaChar = 'A' + (i % 5);
            snprintf(s.kota, sizeof(s.kota), "Kota %c", kotaChar);

            s.active = 1;
            g_stasiun[g_stasiunCount++] = s;
        }
        stasiun_save();
    }
}

void stasiun_create(const char* id, const char* nama, const char* kota) {
    if (g_stasiunCount >= MAX_RECORDS) return;

    Stasiun s;
    strncpy(s.id, id, sizeof(s.id));
    strncpy(s.nama, nama, sizeof(s.nama));
    strncpy(s.kota, kota, sizeof(s.kota));
    s.active = 1;

    g_stasiun[g_stasiunCount++] = s;
    stasiun_save();
}

void stasiun_delete(int index) {
    if (index >= 0 && index < g_stasiunCount) {
        g_stasiun[index].active = 0;
        stasiun_save();
    }
}