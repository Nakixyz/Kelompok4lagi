#include <stdio.h>
#include <string.h>
#include "modul_penumpang.h"
#include "globals.h"

#define FILE_PENUMPANG "penumpang.dat"
// MENYIMPAN FILE
static void penumpang_save() {
    FILE *f = fopen(FILE_PENUMPANG, "wb");
    if (f) {
        fwrite(g_penumpang, sizeof(g_penumpang), 1, f);
        fclose(f);
    }
}

void penumpang_init() {
    // SEED 50 DATA DUMMY PENUMPANG JIKA KOSONG
    FILE *f = fopen(FILE_PENUMPANG, "rb");
    if (f) {
        g_penumpangCount = (int)fread(g_penumpang, sizeof(Penumpang), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_penumpangCount = 0;
    }
    if (g_penumpangCount == 0) {
        for (int i = 0; i < 50 && g_penumpangCount < MAX_RECORDS; i++) {
            Penumpang p;
            snprintf(p.id, sizeof(p.id), "PN%03d", i);
            snprintf(p.nama, sizeof(p.nama), "Penumpang %02d", i);
            snprintf(p.email, sizeof(p.email), "user%02d@kai.id", i);
            snprintf(p.notelp, sizeof(p.notelp), "08123456789");
            snprintf(p.tgl_lahir, sizeof(p.tgl_lahir), "1994-2-6");
            snprintf(p.jk, sizeof(p.jk), (i % 2 == 0) ? "L" : "P");
            p.active = 1;

            g_penumpang[g_penumpangCount++] = p;
        }
        penumpang_save();
    }
}

// MEMBUAT DATA PENUMPANG
void penumpang_create(const char* id, const char* nama, const char* email, const char* notelp,
    const char* tgl_lahir, const char* jk) {
    if (g_penumpangCount >= MAX_RECORDS) return;

    Penumpang p;
    snprintf(p.id, sizeof(p.id), id);
    snprintf(p.nama, sizeof(p.nama), nama);
    snprintf(p.email, sizeof(p.email), email);
    snprintf(p.notelp, sizeof(p.notelp), notelp);
    snprintf(p.tgl_lahir, sizeof(p.tgl_lahir), tgl_lahir);
    snprintf(p.jk, sizeof(p.jk), jk);
    p.active = 1;

    g_penumpang[g_penumpangCount++] = p;
    penumpang_save();
}

// MENGHAPUS DATA PENUMPANG
void penumpang_delete(int index) {
    if (index >= 0 && index < g_penumpangCount) {
        g_penumpang[index].active = 0;
        penumpang_save();
    }
}