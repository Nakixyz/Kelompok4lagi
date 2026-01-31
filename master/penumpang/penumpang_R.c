#include <stdio.h>
#include <string.h>
#include "penumpang.h"
#include "penumpang_shared.h"

void penumpang_save(void) {
    FILE *f = fopen(FILE_PENUMPANG, "wb");
    if (f) {
        fwrite(g_penumpang, sizeof(Penumpang), (size_t)g_penumpangCount, f);
        fclose(f);
    }
}

void penumpang_init(void) {
    FILE *f = fopen(FILE_PENUMPANG, "rb");
    if (f) {
        g_penumpangCount = (int)fread(g_penumpang, sizeof(Penumpang), MAX_RECORDS, f);
        /* Migrasi ringan: ubah format lama 'L'/'P' menjadi teks penuh */
        for (int i = 0; i < g_penumpangCount; i++) {
            char *jk = g_penumpang[i].jenis_kelamin;
            if (!jk) continue;
            if ((jk[0] == 'L' || jk[0] == 'l') && jk[1] == '\0') {
                snprintf(jk, sizeof(g_penumpang[i].jenis_kelamin), "%s", "Laki laki");
            } else if ((jk[0] == 'P' || jk[0] == 'p') && jk[1] == '\0') {
                snprintf(jk, sizeof(g_penumpang[i].jenis_kelamin), "%s", "Perempuan");
            }
        }
        fclose(f);
    } else {
        g_penumpangCount = 0;
    }
}