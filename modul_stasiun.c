#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "modul_stasiun.h"
#include "globals.h"

#define FILE_STASIUN "stasiuns.dat"

/* kalau cls() ada di file UI, biar modul ini tetap bisa compile */
extern void cls();

/* helper: hapus newline dari fgets */
static void trim_newline(char *s) {
    if (!s) return;
    s[strcspn(s, "\n")] = '\0';
}

/* helper: baca input string aman + tidak loncat */
static void read_line(const char *label, char *buf, size_t sz) {
    printf("%s", label);
    if (fgets(buf, (int)sz, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    trim_newline(buf);
}

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
        g_stasiunCount = (int)fread(g_stasiun, sizeof(Stasiun), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_stasiunCount = 0;
    }

    /* Seed 50 Data Dummy */
    if (g_stasiunCount == 0) {
        for (int i = 1; i <= 50 && g_stasiunCount < MAX_RECORDS; i++) {
            Stasiun s;
            memset(&s, 0, sizeof(Stasiun));

            snprintf(s.id, sizeof(s.id), "STS%03d", i);
            snprintf(s.kode, sizeof(s.kode), "KD%03d", i);
            snprintf(s.nama, sizeof(s.nama), "Stasiun Kota %02d", i);

            char kotaChar = 'A' + (i % 5);
            snprintf(s.kota, sizeof(s.kota), "Kota %c", kotaChar);

            snprintf(s.alamat, sizeof(s.alamat), "Alamat Stasiun Kota %02d", i);

            s.active = 1;
            g_stasiun[g_stasiunCount++] = s;
        }
        stasiun_save();
    }
}

/* ==========================
   UI: CREATE (tampilan)
   ========================== */
void stasiunCreate() {
    char id[10], kode[10], nama[50], kota[30], alamat[100];

    cls();
    printf("============== TAMBAH DATA STASIUN ==============\n");

    read_line("ID Stasiun     : ", id, sizeof(id));
    read_line("Kode Stasiun   : ", kode, sizeof(kode));
    read_line("Nama Stasiun   : ", nama, sizeof(nama));
    read_line("Kota           : ", kota, sizeof(kota));
    read_line("Alamat         : ", alamat, sizeof(alamat));

    /* validasi simple */
    if (id[0] == '\0' || kode[0] == '\0' || nama[0] == '\0') {
        printf("\n>> Gagal: ID/Kode/Nama tidak boleh kosong.\n");
        printf("Tekan ENTER untuk kembali...");
        getchar();
        return;
    }

    if (stasiun_find_index_by_id(id) != -1) {
        printf("\n>> Gagal: ID %s sudah ada.\n", id);
        printf("Tekan ENTER untuk kembali...");
        getchar();
        return;
    }

    stasiun_create(id, kode, nama, kota, alamat);

    printf("\n>> Data stasiun berhasil ditambahkan.\n");
    printf("Tekan ENTER untuk kembali...");
    getchar();
}

/* ==========================
   LOGIC: CREATE (PDM)
   ========================== */
void stasiun_create(const char* id, const char* kode, const char* nama,
                    const char* kota, const char* alamat) {
    if (g_stasiunCount >= MAX_RECORDS) return;

    Stasiun s;
    memset(&s, 0, sizeof(Stasiun));

    strncpy(s.id, id, sizeof(s.id) - 1);
    strncpy(s.kode, kode, sizeof(s.kode) - 1);
    strncpy(s.nama, nama, sizeof(s.nama) - 1);
    strncpy(s.kota, kota, sizeof(s.kota) - 1);
    strncpy(s.alamat, alamat, sizeof(s.alamat) - 1);

    s.active = 1;

    g_stasiun[g_stasiunCount++] = s;
    stasiun_save();
}

/* ==========================
   READ (tampilan tabel)
   ========================== */
void stasiunRead() {
    printf("==========================================================================\n");
    printf("%-4s %-10s %-10s %-20s %-15s %-20s\n",
           "No", "ID", "Kode", "Nama Stasiun", "Kota", "Alamat");
    printf("==========================================================================\n");

    int no = 1;
    int found = 0;

    for (int i = 0; i < g_stasiunCount; i++) {
        if (g_stasiun[i].active) {
            printf("%-4d %-10.10s %-10.10s %-20.20s %-15.15s %-20.20s\n",
                   no++,
                   g_stasiun[i].id,
                   g_stasiun[i].kode,
                   g_stasiun[i].nama,
                   g_stasiun[i].kota,
                   g_stasiun[i].alamat);
            found = 1;
        }
    }

    if (!found) {
        printf("Data stasiun kosong.\n");
    }
}

void stasiun_read_by_id(const char* id) {
    int index = stasiun_find_index_by_id(id);
    if (index == -1) {
        printf("Stasiun tidak ditemukan.\n");
        return;
    }

    printf("ID STASIUN  : %s\n", g_stasiun[index].id);
    printf("KODE        : %s\n", g_stasiun[index].kode);
    printf("NAMA        : %s\n", g_stasiun[index].nama);
    printf("KOTA        : %s\n", g_stasiun[index].kota);
    printf("ALAMAT      : %s\n", g_stasiun[index].alamat);
}

/* ==========================
   UI: UPDATE
   ========================== */
void stasiunUpdate() {
    char id[10], kode[10], nama[50], kota[30], alamat[100];

    cls();
    printf("============== UPDATE DATA STASIUN ==============\n");

    read_line("Masukkan ID Stasiun yang akan diupdate : ", id, sizeof(id));

    int index = stasiun_find_index_by_id(id);
    if (index == -1) {
        printf("\n>> Stasiun tidak ditemukan.\n");
        printf("Tekan ENTER untuk kembali...");
        getchar();
        return;
    }

    printf("\nData Lama:\n");
    printf("Kode   : %s\n", g_stasiun[index].kode);
    printf("Nama   : %s\n", g_stasiun[index].nama);
    printf("Kota   : %s\n", g_stasiun[index].kota);
    printf("Alamat : %s\n", g_stasiun[index].alamat);

    printf("\nMasukkan Data Baru\n");
    read_line("Kode Stasiun   : ", kode, sizeof(kode));
    read_line("Nama Stasiun   : ", nama, sizeof(nama));
    read_line("Kota           : ", kota, sizeof(kota));
    read_line("Alamat         : ", alamat, sizeof(alamat));

    if (kode[0] == '\0' || nama[0] == '\0') {
        printf("\n>> Gagal: Kode/Nama tidak boleh kosong.\n");
        printf("Tekan ENTER untuk kembali...");
        getchar();
        return;
    }

    stasiun_update(id, kode, nama, kota, alamat);

    printf("\n>> Data stasiun berhasil diupdate.\n");
    printf("Tekan ENTER untuk kembali...");
    getchar();
}

/* ==========================
   LOGIC: UPDATE (PDM)
   ========================== */
void stasiun_update(const char* id, const char* kode, const char* nama,
                    const char* kota, const char* alamat) {
    int index = stasiun_find_index_by_id(id);
    if (index == -1) return;

    /* aman: -1 dan pasti null-terminated */
    strncpy(g_stasiun[index].kode, kode, sizeof(g_stasiun[index].kode) - 1);
    g_stasiun[index].kode[sizeof(g_stasiun[index].kode) - 1] = '\0';

    strncpy(g_stasiun[index].nama, nama, sizeof(g_stasiun[index].nama) - 1);
    g_stasiun[index].nama[sizeof(g_stasiun[index].nama) - 1] = '\0';

    strncpy(g_stasiun[index].kota, kota, sizeof(g_stasiun[index].kota) - 1);
    g_stasiun[index].kota[sizeof(g_stasiun[index].kota) - 1] = '\0';

    strncpy(g_stasiun[index].alamat, alamat, sizeof(g_stasiun[index].alamat) - 1);
    g_stasiun[index].alamat[sizeof(g_stasiun[index].alamat) - 1] = '\0';

    stasiun_save();
}

/* ==========================
   UI: DELETE
   ========================== */
void stasiunDelete() {
    char id[10];
    char confirm;

    cls();
    printf("============== HAPUS DATA STASIUN ==============\n");

    read_line("Masukkan ID Stasiun yang akan dihapus : ", id, sizeof(id));

    int index = stasiun_find_index_by_id(id);
    if (index == -1) {
        printf("\n>> Stasiun tidak ditemukan.\n");
        printf("Tekan ENTER untuk kembali...");
        getchar();
        return;
    }

    printf("\nData yang akan dihapus:\n");
    printf("ID     : %s\n", g_stasiun[index].id);
    printf("Nama   : %s\n", g_stasiun[index].nama);
    printf("Kota   : %s\n", g_stasiun[index].kota);
    printf("Alamat : %s\n", g_stasiun[index].alamat);

    printf("\nYakin hapus data ini? (y/n): ");
    confirm = getchar();
    getchar(); /* buang newline */

    if (confirm == 'y' || confirm == 'Y') {
        stasiun_delete_by_id(id);
        printf("\n>> Data stasiun berhasil dihapus (soft delete).\n");
    } else {
        printf("\n>> Penghapusan dibatalkan.\n");
    }

    printf("Tekan ENTER untuk kembali...");
    getchar();
}

/* ==========================
   LOGIC: DELETE
   ========================== */
void stasiun_delete_by_id(const char* id) {
    int index = stasiun_find_index_by_id(id);
    if (index == -1) return;

    g_stasiun[index].active = 0;
    stasiun_save();
}

/* ==========================
   HELPER: FIND BY ID (PK)
   ========================== */
int stasiun_find_index_by_id(const char* id) {
    for (int i = 0; i < g_stasiunCount; i++) {
        if (g_stasiun[i].active && strcmp(g_stasiun[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}
