#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include <windows.h>

#include "modul_kereta.h"
#include "globals.h"

/* ===================== FILE STORAGE ===================== */
#define FILE_KERETA "kereta.dat"

/* ===================== UI EXTERN (dari ui_app.c) ===================== */
/* ui_app.c punya fungsi ini (tidak ada di header), jadi kita deklarasi extern */
extern void cls();
extern void gotoXY(int x, int y);
extern int  get_screen_width();
extern int  get_screen_height();
extern void draw_layout_base(int w, int h, const char* section_title);

/* ===================== PRIVATE HELPERS ===================== */

static void kereta_save() {
    FILE *f = fopen(FILE_KERETA, "wb");
    if (f) {
        fwrite(g_kereta, sizeof(Kereta), g_keretaCount, f);
        fclose(f);
    }
}

/* Return jumlah data aktif */
static int kereta_count_active() {
    int c = 0;
    for (int i = 0; i < g_keretaCount; i++) if (g_kereta[i].active) c++;
    return c;
}

/* Map index aktif (0..n-1) ke index asli g_kereta */
static int kereta_map_active_index(int activeIndex) {
    int c = 0;
    for (int i = 0; i < g_keretaCount; i++) {
        if (!g_kereta[i].active) continue;
        if (c == activeIndex) return i;
        c++;
    }
    return -1;
}

/* Cari activeIndex berdasarkan kode */
static int kereta_find_active_index_by_kode(const char* kode) {
    int c = 0;
    for (int i = 0; i < g_keretaCount; i++) {
        if (!g_kereta[i].active) continue;
        if (strcmp(g_kereta[i].kode, kode) == 0) return c;
        c++;
    }
    return -1;
}

/* Read key extended: Arrow/PgUp/PgDn */
static int read_key_ex() {
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        int ext = _getch();
        return 1000 + ext;
    }
    return ch;
}
static int is_pgup(int k)  { return k == 1073; } /* 1000+73 */
static int is_pgdn(int k)  { return k == 1081; } /* 1000+81 */
static int is_up(int k)    { return k == 1072; } /* 1000+72 */
static int is_down(int k)  { return k == 1080; } /* 1000+80 */

/* input line dengan ESC cancel.
   - allow_empty=1 -> boleh kosong (return kosong)
   - allow_empty=0 -> jika kosong, diulang
   return: 1 jika ESC, 0 jika ENTER
*/
static int input_text_at(int x, int y, char* out, int maxlen, int allow_empty) {
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

    out[0] = '\0';
    int i = 0;

    gotoXY(x, y);
    while (1) {
        int k = read_key_ex();

        if (k == 27) { /* ESC */
            out[0] = '\0';
            ci.bVisible = FALSE;
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
            return 1;
        }
        if (k == 13) { /* ENTER */
            if (!allow_empty && i == 0) {
                Beep(500, 150);
                continue;
            }
            out[i] = '\0';
            break;
        }
        if (k == 8) { /* BACKSPACE */
            if (i > 0) {
                i--;
                printf("\b \b");
            }
            continue;
        }

        /* ignore extended keys */
        if (k >= 1000) continue;

        if (k >= 32 && k <= 126 && i < maxlen - 1) {
            out[i++] = (char)k;
            putchar((char)k);
        }
    }

    ci.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    return 0;
}

static int input_int_at(int x, int y, int* outVal, int allow_empty, int* was_empty) {
    char buf[32];
    int esc = input_text_at(x, y, buf, (int)sizeof(buf), allow_empty);
    if (esc) return 1;

    if (buf[0] == '\0') {
        if (was_empty) *was_empty = 1;
        return 0;
    }
    if (was_empty) *was_empty = 0;

    *outVal = atoi(buf);
    return 0;
}

/* ===================== DATA LAYER API ===================== */

void kereta_init() {
    FILE *f = fopen(FILE_KERETA, "rb");
    if (f) {
        g_keretaCount = (int)fread(g_kereta, sizeof(Kereta), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_keretaCount = 0;
    }

    /* Seed 50 Dummy jika kosong */
    if (g_keretaCount == 0) {
        const char *kelas_list[] = {"Eksekutif", "Bisnis", "Ekonomi"};
        for (int i = 1; i <= 50; i++) {
            Kereta k;
            snprintf(k.kode, sizeof(k.kode), "KA%03d", i);
            snprintf(k.nama, sizeof(k.nama), "Argo %02d", i);
            strncpy(k.kelas, kelas_list[i % 3], sizeof(k.kelas) - 1);
            k.kelas[sizeof(k.kelas) - 1] = '\0';

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
    memset(&k, 0, sizeof(k));
    strncpy(k.kode, kode, sizeof(k.kode) - 1);
    strncpy(k.nama, nama, sizeof(k.nama) - 1);
    strncpy(k.kelas, kelas, sizeof(k.kelas) - 1);
    k.gerbong = gerbong;
    k.active = 1;

    g_kereta[g_keretaCount++] = k;
    kereta_save();
}

void kereta_update(int index, const char* nama, const char* kelas, int gerbong) {
    if (index < 0 || index >= g_keretaCount) return;
    if (!g_kereta[index].active) return;

    if (nama && nama[0]) {
        strncpy(g_kereta[index].nama, nama, sizeof(g_kereta[index].nama) - 1);
        g_kereta[index].nama[sizeof(g_kereta[index].nama) - 1] = '\0';
    }
    if (kelas && kelas[0]) {
        strncpy(g_kereta[index].kelas, kelas, sizeof(g_kereta[index].kelas) - 1);
        g_kereta[index].kelas[sizeof(g_kereta[index].kelas) - 1] = '\0';
    }
    if (gerbong > 0) g_kereta[index].gerbong = gerbong;

    kereta_save();
}

void kereta_delete(int index) {
    if (index >= 0 && index < g_keretaCount) {
        g_kereta[index].active = 0; /* soft delete */
        kereta_save();
    }
}

/* ===================== UI (MASTER KERETA) ===================== */

static int compute_page_size(int h) {
    /* Layout ui_app:
       header_h = 7, tabel mulai sekitar y=10.
       Kita sisakan area bawah buat menu + info.
    */
    int top = 10;
    int reserved_bottom = 8; /* menu & footer */
    int avail = (h - reserved_bottom) - (top + 3);

    if (avail < 5) avail = 5;
    if (avail > 32) avail = 32; /* sesuai konsep kamu: page ideal 32 data */
    return avail;
}

static void draw_table_kereta(int w, int h, int cursor_active_idx, int focus_mode, const char* mode_title) {
    /* focus_mode:
       0 = normal list
       1 = selector highlight (bracket)
    */
    int split_x = w / 4;
    int top = 10;
    int pageSize = compute_page_size(h);
    int totalActive = kereta_count_active();
    if (totalActive <= 0) totalActive = 0;

    int totalPages = (totalActive + pageSize - 1) / pageSize;
    if (totalPages < 1) totalPages = 1;

    if (cursor_active_idx < 0) cursor_active_idx = 0;
    if (cursor_active_idx > totalActive - 1) cursor_active_idx = totalActive - 1;
    if (totalActive == 0) cursor_active_idx = 0;

    int page = (totalActive == 0) ? 0 : (cursor_active_idx / pageSize);
    int startActive = page * pageSize;

    /* Header tabel */
    gotoXY(split_x + 4, top);
    printf("%-7s | %-34s | %-10s | %-7s", "Kode", "Nama Kereta", "Kelas", "Gerbong");
    gotoXY(split_x + 4, top + 1);
    for (int i = 0; i < w - split_x - 10; i++) printf("-");

    /* Rows */
    int rowY = top + 2;

    for (int r = 0; r < pageSize; r++) {
        int activeIdx = startActive + r;
        if (activeIdx >= totalActive) break;

        int realIdx = kereta_map_active_index(activeIdx);
        if (realIdx < 0) break;

        char line[256];
        snprintf(line, sizeof(line), "%-7s | %-34s | %-10s | %-7d",
                 g_kereta[realIdx].kode,
                 g_kereta[realIdx].nama,
                 g_kereta[realIdx].kelas,
                 g_kereta[realIdx].gerbong);

        gotoXY(split_x + 4, rowY + r);

        if (focus_mode && activeIdx == cursor_active_idx) {
            /* highlight style konsep kamu */
            printf("[%s]", line);
        } else {
            printf(" %s ", line);
        }
    }

    /* Footer info */
    int infoY = h - 6;
    gotoXY(split_x + 4, infoY);
    for (int i = 0; i < w - split_x - 10; i++) printf("-");

    gotoXY(split_x + 4, infoY + 1);
    if (focus_mode) {
        printf("Mode: %s  |  Page %d/%d  |  Data %d/%d",
               mode_title,
               page + 1, totalPages,
               (totalActive == 0 ? 0 : cursor_active_idx + 1),
               totalActive);
        gotoXY(split_x + 4, infoY + 2);
        printf("PgUp/PgDn / Arrow untuk pindah, ENTER pilih, ESC batal.");
    } else {
        printf("Total Data Aktif: %d  |  Page %d/%d", totalActive, page + 1, totalPages);
    }
}

/* selector: return activeIndex terpilih, -1 jika batal */
static int select_kereta_active_index(const char* title) {
    int cursor = 0;
    int totalActive = kereta_count_active();
    if (totalActive <= 0) return -1;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        cls();
        draw_layout_base(w, h, "Kelola Kereta");
        draw_table_kereta(w, h, cursor, 1, title);

        int k = read_key_ex();
        if (k == 27) return -1;       /* ESC */
        if (k == 13) return cursor;   /* ENTER */

        if (is_up(k) || is_pgup(k)) {
            if (cursor > 0) cursor--;
        } else if (is_down(k) || is_pgdn(k)) {
            if (cursor < totalActive - 1) cursor++;
        } else {
            /* ignore tombol lain (termasuk '[' dan ']') supaya tidak error */
        }
    }
}

static void popup_message(int w, int h, const char* msg) {
    int split_x = w / 4;
    gotoXY(split_x + 4, h - 3);
    printf("%s", msg);
    Sleep(900);
}

static void form_tambah_kereta() {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;
    int split_x = w / 4;

    cls();
    draw_layout_base(w, h, "Kelola Kereta");

    gotoXY(split_x + 4, 10); printf("Tambah Kereta");
    gotoXY(split_x + 4, 11); for (int i = 0; i < w - split_x - 10; i++) printf("-");

    char kode[16], nama[64], kelas[32];
    int gerbong = 0;

    gotoXY(split_x + 4, 13); printf("Kode   (contoh KA001)  : ");
    if (input_text_at(split_x + 32, 13, kode, (int)sizeof(kode), 0)) return;

    gotoXY(split_x + 4, 14); printf("Nama Kereta            : ");
    if (input_text_at(split_x + 32, 14, nama, (int)sizeof(nama), 0)) return;

    gotoXY(split_x + 4, 15); printf("Kelas (Eksekutif/Bisnis/Ekonomi): ");
    if (input_text_at(split_x + 44, 15, kelas, (int)sizeof(kelas), 0)) return;

    gotoXY(split_x + 4, 16); printf("Gerbong (angka)        : ");
    int dummyEmpty = 0;
    if (input_int_at(split_x + 32, 16, &gerbong, 0, &dummyEmpty)) return;

    kereta_create(kode, nama, kelas, gerbong);
    popup_message(w, h, ">> Berhasil tambah kereta.");
}

static void form_detail_kereta(int realIdx) {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;
    int split_x = w / 4;

    cls();
    draw_layout_base(w, h, "Kelola Kereta");

    gotoXY(split_x + 4, 10); printf("Detail Kereta");
    gotoXY(split_x + 4, 11); for (int i = 0; i < w - split_x - 10; i++) printf("-");

    Kereta* k = &g_kereta[realIdx];
    gotoXY(split_x + 4, 13); printf("Kode   : %s", k->kode);
    gotoXY(split_x + 4, 14); printf("Nama   : %s", k->nama);
    gotoXY(split_x + 4, 15); printf("Kelas  : %s", k->kelas);
    gotoXY(split_x + 4, 16); printf("Gerbong: %d", k->gerbong);

    gotoXY(split_x + 4, h - 4); printf("[Tekan tombol apa saja untuk kembali]");
    _getch();
}

static void form_hapus_kereta(int realIdx) {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;
    int split_x = w / 4;

    cls();
    draw_layout_base(w, h, "Kelola Kereta");

    Kereta* k = &g_kereta[realIdx];

    gotoXY(split_x + 4, 10); printf("Hapus Kereta");
    gotoXY(split_x + 4, 11); for (int i = 0; i < w - split_x - 10; i++) printf("-");

    gotoXY(split_x + 4, 13);
    printf("Yakin hapus %s (%s)? [Y/N] : ", k->kode, k->nama);

    int ch = toupper(_getch());
    if (ch == 'Y') {
        kereta_delete(realIdx);
        popup_message(w, h, ">> Berhasil hapus (soft delete).");
    } else {
        popup_message(w, h, ">> Batal hapus.");
    }
}

static void form_ubah_kereta(int realIdx) {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;
    int split_x = w / 4;

    Kereta before = g_kereta[realIdx];

    /* Tampilkan UI list (dengan highlight sudah selesai di selector),
       lalu tampilkan form edit tapi data tetap terlihat di atas (konsep kamu) */
    cls();
    draw_layout_base(w, h, "Kelola Kereta");

    /* tampilkan tabel biasa (tanpa selector), tapi kita posisikan cursor di record tsb */
    int activeIdx = kereta_find_active_index_by_kode(before.kode);
    if (activeIdx < 0) activeIdx = 0;
    draw_table_kereta(w, h, activeIdx, 0, ""); /* list tetap tampil */

    int formY = h - 10;
    if (formY < 14) formY = 14;

    gotoXY(split_x + 4, formY);     printf("Ubah Kereta (kosongkan = tetap, ESC = batal)");
    gotoXY(split_x + 4, formY + 1); for (int i = 0; i < w - split_x - 10; i++) printf("-");

    gotoXY(split_x + 4, formY + 3); printf("Kode   : %s", before.kode);

    char nama[64] = {0};
    char kelas[32] = {0};
    int  gerbong = 0;
    int  emptyInt = 0;

    gotoXY(split_x + 4, formY + 4); printf("Nama   : ");
    if (input_text_at(split_x + 13, formY + 4, nama, (int)sizeof(nama), 1)) return;

    gotoXY(split_x + 4, formY + 5); printf("Kelas  : ");
    if (input_text_at(split_x + 13, formY + 5, kelas, (int)sizeof(kelas), 1)) return;

    gotoXY(split_x + 4, formY + 6); printf("Gerbong: ");
    if (input_int_at(split_x + 13, formY + 6, &gerbong, 1, &emptyInt)) return;

    gotoXY(split_x + 4, formY + 8);
    printf("Nilai lama -> Nama:%s | Kelas:%s | Gerbong:%d", before.nama, before.kelas, before.gerbong);

    /* Apply perubahan */
    int newGerbong = (emptyInt ? 0 : gerbong);
    kereta_update(realIdx, nama, kelas, newGerbong);

    popup_message(w, h, ">> Berhasil ubah data.");
}

static void form_cari_kereta() {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;
    int split_x = w / 4;

    cls();
    draw_layout_base(w, h, "Kelola Kereta");

    gotoXY(split_x + 4, 10); printf("Cari Kereta (ketik Kode, ENTER. ESC batal)");
    gotoXY(split_x + 4, 11); for (int i = 0; i < w - split_x - 10; i++) printf("-");

    char kode[16];
    gotoXY(split_x + 4, 13); printf("Kode : ");
    if (input_text_at(split_x + 11, 13, kode, (int)sizeof(kode), 0)) return;

    int activeIdx = kereta_find_active_index_by_kode(kode);
    if (activeIdx < 0) {
        popup_message(w, h, ">> Data tidak ditemukan.");
        return;
    }

    /* tampilkan list dengan highlight di hasil */
    while (1) {
        cls();
        draw_layout_base(w, h, "Kelola Kereta");
        draw_table_kereta(w, h, activeIdx, 1, "Hasil Pencarian");

        gotoXY(split_x + 4, h - 4);
        printf("[ENTER] Detail  |  [U] Ubah  |  [H] Hapus  |  [ESC] Kembali");

        int k = read_key_ex();
        if (k == 27) return;
        if (k >= 0 && k < 1000) k = toupper(k);

        int realIdx = kereta_map_active_index(activeIdx);
        if (realIdx < 0) return;

        if (k == 13) form_detail_kereta(realIdx);
        else if (k == 'U') form_ubah_kereta(realIdx);
        else if (k == 'H') form_hapus_kereta(realIdx);
    }
}

void view_kereta() {
    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        cls();
        draw_layout_base(w, h, "Kelola Kereta");

        /* tabel biasa */
        draw_table_kereta(w, h, 0, 0, "");

        int split_x = w / 4;
        gotoXY(split_x + 4, h - 4);
        printf("[1] Tambah  [2] Ubah  [3] Hapus  [4] Detail  [5] Cari  [0] Kembali");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == '1') {
            form_tambah_kereta();
        }
        else if (ch == '2') {
            int pick = select_kereta_active_index("Ubah Kereta");
            if (pick >= 0) {
                int realIdx = kereta_map_active_index(pick);
                if (realIdx >= 0) form_ubah_kereta(realIdx);
            }
        }
        else if (ch == '3') {
            int pick = select_kereta_active_index("Hapus Kereta");
            if (pick >= 0) {
                int realIdx = kereta_map_active_index(pick);
                if (realIdx >= 0) form_hapus_kereta(realIdx);
            }
        }
        else if (ch == '4') {
            int pick = select_kereta_active_index("Detail Kereta");
            if (pick >= 0) {
                int realIdx = kereta_map_active_index(pick);
                if (realIdx >= 0) form_detail_kereta(realIdx);
            }
        }
        else if (ch == '5') {
            form_cari_kereta();
        }
        else {
            /* ignore */
        }
    }
}
