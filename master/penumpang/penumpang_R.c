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
        fclose(f);
    } else {
        g_penumpangCount = 0;
    }
}
