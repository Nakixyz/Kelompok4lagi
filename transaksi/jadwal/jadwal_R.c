#include <stdio.h>
#include <string.h>
#include "jadwal.h"
#include "jadwal_shared.h"

static int parse_jdw_number(const char *id) {
    if (!id) return -1;
    if (strncmp(id, "JDW", 3) != 0) return -1;
    int n = 0;
    if (sscanf(id + 3, "%d", &n) != 1) return -1;
    return n;
}

static int jadwal_get_max_number(void) {
    int max_n = 0;
    for (int i = 0; i < g_jadwalCount; i++) {
        int n = parse_jdw_number(g_jadwal[i].id_jadwal);
        if (n > max_n) max_n = n;
    }
    return max_n;
}

static void format_jdw_id(int n, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    snprintf(out, out_sz, "JDW%03d", n);
}

void jadwal_save(void) {
    FILE *f = fopen(FILE_JADWAL, "wb");
    if (!f) return;
    fwrite(g_jadwal, sizeof(JadwalTiket), (size_t)g_jadwalCount, f);
    fclose(f);
}

void jadwal_save_public(void) { jadwal_save(); }

void jadwal_init(void) {
    FILE *f = fopen(FILE_JADWAL, "rb");
    if (f) {
        g_jadwalCount = (int)fread(g_jadwal, sizeof(JadwalTiket), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_jadwalCount = 0;
    }

    
    /* MIGRASI DATA LAMA:
       Beberapa versi sebelumnya menyimpan file tanpa field 'active' atau nilainya selalu 0,
       sehingga record tidak tampil karena UI memfilter active==1.
       Jika setelah load tidak ada satupun record yang active==1, kita anggap data lama,
       lalu set active=1 untuk record yang ID-nya tidak kosong. */
    if (g_jadwalCount > 0) {
        int any_active = 0;
        for (int i = 0; i < g_jadwalCount; i++) {
            if (g_jadwal[i].active == 1) { any_active = 1; break; }
        }
        if (!any_active) {
            for (int i = 0; i < g_jadwalCount; i++) {
                if (g_jadwal[i].id_jadwal[0] != '\0') {
                    g_jadwal[i].active = 1;
                }
            }
        }
    }

}

int jadwal_find_index_by_id(const char *id_jadwal) {
    if (!id_jadwal) return -1;
    for (int i = 0; i < g_jadwalCount; i++) {
        if (strcmp(g_jadwal[i].id_jadwal, id_jadwal) == 0) return i;
    }
    return -1;
}

/* expose for C file */
int jadwal__get_next_number(void) { return jadwal_get_max_number() + 1; }
void jadwal__format_id(int n, char *out, size_t out_sz) { format_jdw_id(n, out, out_sz); }
