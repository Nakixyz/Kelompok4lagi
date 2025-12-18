#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <windows.h>
#include <conio.h>

#include "modul_kereta.h"
#include "globals.h"
#include "types.h"

/* ======================= FILE SAVE/LOAD ======================= */
#define FILE_KERETA "kereta.dat"

static void kereta_save(void) {
    FILE *f = fopen(FILE_KERETA, "wb");
    if (f) {
        fwrite(g_kereta, sizeof(Kereta), g_keretaCount, f);
        fclose(f);
    }
}

static void kereta_load(void) {
    FILE *f = fopen(FILE_KERETA, "rb");
    if (f) {
        g_keretaCount = (int)fread(g_kereta, sizeof(Kereta), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_keretaCount = 0;
    }
}

/* ======================= KEY CODES (getch) ======================= */
#define KEY_ESC   27
#define KEY_ENTER 13
#define KEY_BS     8

#define EXT_0     0
#define EXT_224   224

#define K_UP      72
#define K_DOWN    80
#define K_LEFT    75
#define K_RIGHT   77
#define K_PGUP    73
#define K_PGDN    81
#define K_HOME    71
#define K_END     79

/* ======================= Helper Console (layout tidak diubah) ======================= */
static void k_cls(void) { system("cls"); }

static void k_gotoXY(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

static int k_screen_w(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return 120;
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

static int k_screen_h(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return 30;
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

static void k_draw_layout_base(int w, int h, const char* section_title) {
    int split_x = w / 4;
    int header_h = 7;

    for(int x=0; x<w; x++) {
        k_gotoXY(x, 0); printf("=");
        k_gotoXY(x, header_h); printf("=");
        k_gotoXY(x, h-2); printf("=");
    }
    for(int y=0; y<h-1; y++) {
        k_gotoXY(0, y); printf("||");
        k_gotoXY(split_x, y); printf("||");
        k_gotoXY(w-2, y); printf("||");
    }

    int content_w = w - split_x;
    int title_x = split_x + (content_w / 2) - 32;
    if (title_x < split_x + 2) title_x = split_x + 2;

    k_gotoXY(title_x, 1); printf(" _______ _             ______                 _            _       ");
    k_gotoXY(title_x, 2); printf("|__   __| |           |  ____|               | |          (_)      ");
    k_gotoXY(title_x, 3); printf("   | |  | |__   ___   | |__ ___  _   _ _ __| |_ _ __ __ _ _ _ __ ");
    k_gotoXY(title_x, 4); printf("   | |  | '_ \\ / _ \\  |  __/ _ \\| | | | '__| __| '__/ _` | | '_ \\");
    k_gotoXY(title_x, 5); printf("   | |  | | | |  __/  | | | (_) | |_| | |  | |_| | | (_| | | | | |");
    k_gotoXY(title_x, 6); printf("   |_|  |_| |_|\\___|  |_|  \\___/ \\__,_|_|   \\__|_|  \\__,_|_|_| |_|");

    k_gotoXY(5, 3); printf("- - Kelompok 4 - -");
    k_gotoXY(5, 5); printf("Role: %s", section_title);

    k_gotoXY(split_x + 2, header_h + 1);
    printf(" %s >", section_title);
}

/* ======================= Util string ======================= */
static void trim(char* s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) s[--n] = 0;
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

static int str_ieq(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

/* ======================= Validasi Nama (huruf + spasi, unik) ======================= */
static int is_alpha_space_only(const char* s) {
    if (!s || !s[0]) return 0;
    for (size_t i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!(isalpha(c) || c == ' ')) return 0;
    }
    return 1;
}

static int nama_exists_case_insensitive(const char* nama) {
    for (int i = 0; i < g_keretaCount; i++) {
        if (g_kereta[i].nama[0] && str_ieq(g_kereta[i].nama, nama)) return 1;
    }
    return 0;
}

/* ======================= Input (getch) dengan ESC cancel ======================= */
static int input_alpha_line_at(int x, int y, char* buf, int maxLen) {
    int len = 0;
    buf[0] = 0;

    k_gotoXY(x, y);
    for (int i=0; i<maxLen-1; i++) putchar(' ');
    k_gotoXY(x, y);

    while (1) {
        int ch = _getch();

        if (ch == KEY_ESC) { buf[0] = 0; return 0; }
        if (ch == KEY_ENTER) { buf[len] = 0; trim(buf); return 1; }

        if (ch == KEY_BS) {
            if (len > 0) {
                len--;
                buf[len] = 0;
                printf("\b \b");
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            if (len < maxLen-1) {
                char c = (char)ch;
                if (isalpha((unsigned char)c) || c == ' ') {
                    buf[len++] = c;
                    buf[len] = 0;
                    putchar(c);
                }
            }
        }
    }
}

static int input_digits_only_at(int x, int y, char* buf, int maxLen) {
    int len = 0;
    buf[0] = 0;

    k_gotoXY(x, y);
    for (int i=0; i<maxLen-1; i++) putchar(' ');
    k_gotoXY(x, y);

    while (1) {
        int ch = _getch();

        if (ch == KEY_ESC) { buf[0] = 0; return 0; }
        if (ch == KEY_ENTER) { buf[len] = 0; trim(buf); return 1; }

        if (ch == KEY_BS) {
            if (len > 0) {
                len--;
                buf[len] = 0;
                printf("\b \b");
            }
            continue;
        }

        if (isdigit((unsigned char)ch)) {
            if (len < maxLen-1) {
                buf[len++] = (char)ch;
                buf[len] = 0;
                putchar((char)ch);
            }
        }
    }
}

static int parse_int_strict(const char* s, int* out) {
    if (!s) return 0;
    char tmp[64];
    strncpy(tmp, s, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = 0;
    trim(tmp);
    if (tmp[0] == 0) return 0;

    for (size_t i = 0; tmp[i]; i++) {
        if (!isdigit((unsigned char)tmp[i])) return 0;
    }
    long v = strtol(tmp, NULL, 10);
    if (v <= 0 || v > 1000000) return 0;
    *out = (int)v;
    return 1;
}

/* ======================= Kode otomatis unik ======================= */
static int kode_exists(const char* kode) {
    for (int i = 0; i < g_keretaCount; i++) {
        if (strcmp(g_kereta[i].kode, kode) == 0) return 1;
    }
    return 0;
}

static void gen_kode_acak(char out[16]) {
    for (int tries = 0; tries < 5000; tries++) {
        int num = (rand() % 999) + 1;
        snprintf(out, 16, "KA%03d", num);
        if (!kode_exists(out)) return;
    }
    for (int num = 1; num <= 999; num++) {
        snprintf(out, 16, "KA%03d", num);
        if (!kode_exists(out)) return;
    }
    strcpy(out, "KA000");
}

/* ======================= Dummy Nama: "Argo <Daerah Indonesia acak>" ======================= */
static const char* g_daerah[] = {
    "Sampang","Cibinong","Pamekasan","Tuban","Wonosobo","Kebumen","Purwokerto","Cilacap",
    "Kudus","Jepara","Rembang","Blora","Banyuwangi","Lumajang","Jember","Situbondo",
    "Bondowoso","Probolinggo","Pasuruan","Nganjuk","Ngawi","Magetan","Madiun","Ponorogo",
    "Trenggalek","Tulungagung","Kediri","Batu","Malang","Mojokerto","Sidoarjo","Gresik",
    "Lamongan","Bojonegoro","Pacitan","Banjarnegara","Temanggung","Salatiga","Solo","Sragen",
    "Klaten","Wonogiri","Magelang","Muntilan","Ambarawa","Kaliwungu","Pekalongan","Batang",
    "Kendal","Semarang","Demak","Grobogan","Cepu","Kertajaya","Sukodono","Balongbendo",
    "Karanganyar","Colomadu","Kartasura","Banyumanik","Ngaliyan","Tembalang","Jatisari",
    "Cipayung","Cikarang","Cimahi","Cianjur","Cilegon","Serang","Pandeglang","Lebak",
    "Banyuasin","Ogan Ilir","Prabumulih","Lubuklinggau","Muara Enim","Pagaralam",
    "Bukittinggi","Payakumbuh","Sijunjung","Solok","Pariaman","Padangpanjang"
};

static void gen_nama_argo(char out[64]) {
    int daerahCount = (int)(sizeof(g_daerah)/sizeof(g_daerah[0]));
    for (int tries = 0; tries < 5000; tries++) {
        const char* d = g_daerah[rand() % daerahCount];
        snprintf(out, 64, "Argo %s", d);
        if (!nama_exists_case_insensitive(out)) return;
    }
    for (int i=1; i<=9999; i++) {
        snprintf(out, 64, "Argo Nusantara %d", i);
        if (!nama_exists_case_insensitive(out)) return;
    }
    strcpy(out, "Argo Nusantara");
}

/* ======================= Kelas selection (left/right/home/end, enter, esc) ======================= */
static int select_kelas_ui(int split_x, int w, int start_y, int defaultSel, char outKelas[16]) {
    const char* labels[3] = {"EKONOMI", "EKSEKUTIF", "BISNIS"};
    const char* normalized[3] = {"Ekonomi", "Eksekutif", "Bisnis"};

    int sel = defaultSel;
    if (sel < 0) sel = 0;
    if (sel > 2) sel = 2;

    while (1) {
        int x = split_x + 4;

        k_gotoXY(x, start_y);
        printf("KELAS YANG INGIN ANDA PILIH :");

        k_gotoXY(x, start_y + 2);
        printf("               ");

        for (int i=0; i<3; i++) {
            if (i == sel) printf("[  %-9s ]", labels[i]);
            else          printf("   %-9s  ", labels[i]);
            if (i != 2) printf("               ");
        }

        k_gotoXY(x, start_y + 4);
        printf("Gunakan <- / -> (Home/End) lalu ENTER.  [ESC] Keluar");

        int ch = _getch();
        if (ch == KEY_ESC) return 0;
        if (ch == KEY_ENTER) {
            strcpy(outKelas, normalized[sel]);
            return 1;
        }

        if (ch == EXT_0 || ch == EXT_224) {
            int ext = _getch();
            if (ext == K_LEFT || ext == K_HOME) { sel--; if (sel < 0) sel = 2; }
            else if (ext == K_RIGHT || ext == K_END) { sel++; if (sel > 2) sel = 0; }
        }
    }
}

/* ======================= Render table + paging ======================= */
static void render_table(int split_x, int w, int top_y, int page, int page_size) {
    int start = page * page_size;
    int end = start + page_size;
    if (end > g_keretaCount) end = g_keretaCount;

    int content_w = w - split_x;
    int x = split_x + 4;

    k_gotoXY(x, top_y);
    printf("%-7s | %-30s | %-10s | %-7s | %-11s", "Kode", "Nama Kereta", "Kelas", "Gerbong", "Status");
    k_gotoXY(x, top_y+1);
    for (int i=0; i<content_w-10; i++) putchar('-');

    int row = top_y + 2;
    for (int i = start; i < end; i++, row++) {
        const char* status = g_kereta[i].active ? "Aktif" : "Tidak Aktif";
        k_gotoXY(x, row);
        printf("%-7s | %-30s | %-10s | %-7d | %-11s",
               g_kereta[i].kode, g_kereta[i].nama, g_kereta[i].kelas, g_kereta[i].gerbong, status);
    }

    for (; row < top_y + 2 + page_size; row++) {
        k_gotoXY(x, row);
        for (int i=0; i<content_w-8; i++) putchar(' ');
    }
}

/* ======================= Selector untuk Ubah/Hapus/Detail ======================= */
static void render_table_selector(int split_x, int w, int top_y, int page, int page_size, int selected_global) {
    int start = page * page_size;
    int end = start + page_size;
    if (end > g_keretaCount) end = g_keretaCount;

    int content_w = w - split_x;
    int x = split_x + 4;

    k_gotoXY(x, top_y);
    printf("%-7s | %-30s | %-10s | %-7s | %-11s", "Kode", "Nama Kereta", "Kelas", "Gerbong", "Status");
    k_gotoXY(x, top_y+1);
    for (int i=0; i<content_w-10; i++) putchar('-');

    int row = top_y + 2;
    for (int i = start; i < end; i++, row++) {
        const char* status = g_kereta[i].active ? "Aktif" : "Tidak Aktif";
        char line[256];
        snprintf(line, sizeof(line), "%-7s | %-30s | %-10s | %-7d | %-11s",
                 g_kereta[i].kode, g_kereta[i].nama, g_kereta[i].kelas, g_kereta[i].gerbong, status);

        k_gotoXY(x, row);
        if (i == selected_global) printf("[%s]", line);
        else                      printf(" %s ", line);
    }

    for (; row < top_y + 2 + page_size; row++) {
        k_gotoXY(x, row);
        for (int i=0; i<content_w-8; i++) putchar(' ');
    }
}

static int selector_pick_index(const char* modeTitle) {
    if (g_keretaCount <= 0) return -1;

    int w = k_screen_w(); if (w<=0) w=120;
    int h = k_screen_h(); if (h<=0) h=30;
    int split_x = w/4;

    int header_h = 7;
    int top_y = header_h + 4;

    int max_rows = h - (top_y + 8);
    if (max_rows < 5) max_rows = 5;
    int page_size = max_rows;

    int total_pages = (g_keretaCount + page_size - 1) / page_size;
    if (total_pages < 1) total_pages = 1;

    int page = 0;
    int selected = 0;

    while (1) {
        if (page < 0) page = total_pages - 1;
        if (page >= total_pages) page = 0;

        int start = page * page_size;
        int end = start + page_size;
        if (end > g_keretaCount) end = g_keretaCount;

        if (selected < start) selected = start;
        if (selected >= end) selected = end - 1;
        if (selected < 0) selected = 0;

        k_cls();
        k_draw_layout_base(w, h, "Kelola Kereta");

        int x = split_x + 4;
        k_gotoXY(x, header_h + 2);
        printf("%s", modeTitle);

        render_table_selector(split_x, w, top_y, page, page_size, selected);

        k_gotoXY(x, h - 6);
        printf("[PgUp/PgDn/Up/Down] Pindah  |  [<- -> (Home/End)] Halaman  |  [ENTER] Pilih  |  [ESC] Keluar");
        k_gotoXY(x, h - 5);
        printf("Halaman %d/%d", page + 1, total_pages);

        int ch = _getch();
        if (ch == KEY_ESC) return -1;
        if (ch == KEY_ENTER) return selected;

        if (ch == EXT_0 || ch == EXT_224) {
            int ext = _getch();
            if (ext == K_UP) { if (selected > 0) selected--; }
            else if (ext == K_DOWN) { if (selected < g_keretaCount - 1) selected++; }
            else if (ext == K_PGUP) { selected -= page_size; if (selected < 0) selected = 0; }
            else if (ext == K_PGDN) { selected += page_size; if (selected > g_keretaCount - 1) selected = g_keretaCount - 1; }
            else if (ext == K_LEFT || ext == K_HOME) {
                page--; if (page < 0) page = total_pages - 1;
                selected = page * page_size;
                if (selected >= g_keretaCount) selected = g_keretaCount - 1;
            } else if (ext == K_RIGHT || ext == K_END) {
                page++; if (page >= total_pages) page = 0;
                selected = page * page_size;
                if (selected >= g_keretaCount) selected = g_keretaCount - 1;
            }
        }
    }
}

/* ======================= Detail Box ======================= */
static void show_detail_box(int split_x, int w, int y, const Kereta* k) {
    int x = split_x + 4;
    int box_w = (w - split_x) - 10;
    if (box_w < 50) box_w = 50;

    k_gotoXY(x, y);     putchar('+'); for (int i=0;i<box_w-2;i++) putchar('-'); putchar('+');
    for (int r=1; r<=4; r++) {
        k_gotoXY(x, y+r); putchar('|'); for (int i=0;i<box_w-2;i++) putchar(' '); putchar('|');
    }
    k_gotoXY(x, y+5);   putchar('+'); for (int i=0;i<box_w-2;i++) putchar('-'); putchar('+');

    const char* status = k->active ? "Aktif" : "Tidak Aktif";

    k_gotoXY(x+2, y+1); printf("Kode   : %s", k->kode);
    k_gotoXY(x+2, y+2); printf("Nama   : %s", k->nama);
    k_gotoXY(x+2, y+3); printf("Kelas  : %s", k->kelas);
    k_gotoXY(x+2, y+4); printf("Gerbong: %d | Status: %s", k->gerbong, status);
}

/* ======================= ACTIONS ======================= */
static void action_tambah(void) {
    int w = k_screen_w(); if (w<=0) w=120;
    int h = k_screen_h(); if (h<=0) h=30;
    int split_x = w/4;
    int header_h = 7;

    Kereta k;
    memset(&k, 0, sizeof(k));

    gen_kode_acak(k.kode);
    k.active = 1;

    char nama[64] = {0};
    char kelas_ok[16] = {0};
    char gerbong_in[32] = {0};

    while (1) {
        k_cls();
        k_draw_layout_base(w, h, "Kelola Kereta");
        int x = split_x + 4;
        int y = header_h + 3;

        k_gotoXY(x, y);     printf("Tambah Kereta");
        k_gotoXY(x, y+1);   for(int i=0;i<w-split_x-10;i++) printf("-");
        k_gotoXY(x, y+3);   printf("Kode  (otomatis)                : %s", k.kode);
        k_gotoXY(x, y+4);   printf("Nama Kereta                     : ");
        k_gotoXY(x, y+5);   printf("Kelas (Eksekutif/Bisnis/Ekonomi): ");
        k_gotoXY(x, y+6);   printf("Gerbong (angka)                 : ");
        k_gotoXY(x, y+7);   printf("Status (A=Aktif / N=Non)        : ");
        k_gotoXY(x, y+9);   printf("[ESC] Keluar");

        if (!input_alpha_line_at(x+34, y+4, nama, (int)sizeof(nama))) return;
        if (!is_alpha_space_only(nama)) {
            k_gotoXY(x, y+11); printf("Nama harus huruf (tanpa angka/simbol). Tekan tombol...");
            _getch(); continue;
        }
        if (nama_exists_case_insensitive(nama)) {
            k_gotoXY(x, y+11); printf("Nama kereta sudah ada. Harus berbeda. Tekan tombol...");
            _getch(); continue;
        }

        if (!select_kelas_ui(split_x, w, y+11, 0, kelas_ok)) return;
        k_gotoXY(x+34, y+5); printf("%-12s", kelas_ok);

        if (!input_digits_only_at(x+34, y+6, gerbong_in, (int)sizeof(gerbong_in))) return;
        if (!parse_int_strict(gerbong_in, &k.gerbong)) {
            k_gotoXY(x, y+17); printf("Gerbong harus angka > 0. Tekan tombol...");
            _getch(); continue;
        }

        k_gotoXY(x+34, y+7);
        printf(" ");
        k_gotoXY(x+34, y+7);
        int st = _getch();
        if (st == KEY_ESC) return;
        if (st == KEY_ENTER) st = 'A';
        st = toupper((unsigned char)st);
        putchar((char)st);
        k.active = (st == 'A') ? 1 : 0;

        strncpy(k.nama, nama, sizeof(k.nama)-1);
        strncpy(k.kelas, kelas_ok, sizeof(k.kelas)-1);

        if (g_keretaCount < MAX_RECORDS) {
            g_kereta[g_keretaCount++] = k;
            kereta_save();
            k_gotoXY(x, y+19); printf("Berhasil tambah. Tekan tombol...");
        } else {
            k_gotoXY(x, y+19); printf("Data penuh (MAX_RECORDS). Tekan tombol...");
        }
        _getch();
        return;
    }
}

static void action_detail(void) {
    int idx = selector_pick_index("Detail Kereta");
    if (idx < 0) return;

    int w = k_screen_w(); if (w<=0) w=120;
    int h = k_screen_h(); if (h<=0) h=30;
    int split_x = w/4;
    int header_h = 7;

    k_cls();
    k_draw_layout_base(w, h, "Kelola Kereta");
    int x = split_x + 4;
    int y = header_h + 3;

    k_gotoXY(x, y); printf("Detail Kereta");
    k_gotoXY(x, y+1); for(int i=0;i<w-split_x-10;i++) printf("-");
    show_detail_box(split_x, w, y+3, &g_kereta[idx]);

    k_gotoXY(x, y+10);
    printf("[ESC] Keluar  |  Tekan tombol apa saja untuk kembali...");
    int ch = _getch();
    if (ch == KEY_ESC) return;
}

/* [3] Hapus = ubah status Aktif -> Non-Aktif */
static void action_hapus_status(void) {
    int idx = selector_pick_index("Hapus Kereta");
    if (idx < 0) return;

    int w = k_screen_w(); if (w<=0) w=120;
    int h = k_screen_h(); if (h<=0) h=30;
    int split_x = w/4;
    int header_h = 7;

    k_cls();
    k_draw_layout_base(w, h, "Kelola Kereta");
    int x = split_x + 4;
    int y = header_h + 3;

    k_gotoXY(x, y); printf("Hapus Kereta");
    k_gotoXY(x, y+1); for(int i=0;i<w-split_x-10;i++) printf("-");
    show_detail_box(split_x, w, y+3, &g_kereta[idx]);

    if (g_kereta[idx].active == 0) {
        k_gotoXY(x, y+10);
        printf("Kereta ini sudah Non-Aktif");
        k_gotoXY(x, y+12);
        printf("[ESC] Keluar  |  Tekan tombol apa saja...");
        int ch = _getch();
        if (ch == KEY_ESC) return;
        return;
    }

    k_gotoXY(x, y+10);
    printf("Yakin ubah status menjadi Non-Aktif? (Y/N)  [ESC] Keluar: ");
    int ch = _getch();
    if (ch == KEY_ESC) return;

    ch = toupper((unsigned char)ch);
    if (ch == 'Y') {
        g_kereta[idx].active = 0;
        kereta_save();
        k_gotoXY(x, y+12); printf("Status berhasil diubah menjadi Non-Aktif. Tekan tombol...");
    } else {
        k_gotoXY(x, y+12); printf("Batal. Tekan tombol...");
    }
    _getch();
}

/* Ubah (kelas selector sama seperti tambah) */
static void action_ubah(void) {
    int idx = selector_pick_index("Ubah Kereta");
    if (idx < 0) return;

    int w = k_screen_w(); if (w<=0) w=120;
    int h = k_screen_h(); if (h<=0) h=30;
    int split_x = w/4;
    int header_h = 7;

    Kereta* k = &g_kereta[idx];

    char nama[64] = {0};
    char kelas_ok[16] = {0};
    char gerbong_in[32] = {0};

    while (1) {
        k_cls();
        k_draw_layout_base(w, h, "Kelola Kereta");
        int x = split_x + 4;
        int y = header_h + 3;

        k_gotoXY(x, y); printf("Ubah Kereta");
        k_gotoXY(x, y+1); for(int i=0;i<w-split_x-10;i++) printf("-");
        show_detail_box(split_x, w, y+3, k);

        int form_y = y + 10;
        k_gotoXY(x, form_y);     printf("Masukkan data baru (kosongkan = tetap):");
        k_gotoXY(x, form_y+2);   printf("Nama Kereta                     : ");
        k_gotoXY(x, form_y+3);   printf("Kelas (Eksekutif/Bisnis/Ekonomi): ");
        k_gotoXY(x, form_y+4);   printf("Gerbong (angka)                 : ");
        k_gotoXY(x, form_y+5);   printf("Status (A=Aktif / N=Non)        : ");
        k_gotoXY(x, form_y+7);   printf("[ESC] Keluar");

        if (!input_alpha_line_at(x+34, form_y+2, nama, (int)sizeof(nama))) return;

        if (nama[0]) {
            if (!is_alpha_space_only(nama)) {
                k_gotoXY(x, form_y+9); printf("Nama harus huruf. Tekan tombol...");
                _getch(); continue;
            }
            int dupe = 0;
            for (int i=0; i<g_keretaCount; i++) {
                if (i != idx && g_kereta[i].nama[0] && str_ieq(g_kereta[i].nama, nama)) { dupe = 1; break; }
            }
            if (dupe) {
                k_gotoXY(x, form_y+9); printf("Nama sudah dipakai. Tekan tombol...");
                _getch(); continue;
            }
        }

        int defaultSel = 0;
        if (str_ieq(k->kelas, "Ekonomi")) defaultSel = 0;
        else if (str_ieq(k->kelas, "Eksekutif")) defaultSel = 1;
        else if (str_ieq(k->kelas, "Bisnis")) defaultSel = 2;

        if (!select_kelas_ui(split_x, w, form_y+9, defaultSel, kelas_ok)) return;
        k_gotoXY(x+34, form_y+3); printf("%-12s", kelas_ok);

        if (!input_digits_only_at(x+34, form_y+4, gerbong_in, (int)sizeof(gerbong_in))) return;

        k_gotoXY(x+34, form_y+5);
        printf(" ");
        k_gotoXY(x+34, form_y+5);
        int st = _getch();
        if (st == KEY_ESC) return;
        if (st == KEY_ENTER) st = 0;
        else {
            st = toupper((unsigned char)st);
            putchar((char)st);
        }

        if (nama[0]) {
            strncpy(k->nama, nama, sizeof(k->nama)-1);
            k->nama[sizeof(k->nama)-1] = 0;
        }

        if (kelas_ok[0]) {
            strncpy(k->kelas, kelas_ok, sizeof(k->kelas)-1);
            k->kelas[sizeof(k->kelas)-1] = 0;
        }

        if (gerbong_in[0]) {
            int gb = 0;
            if (!parse_int_strict(gerbong_in, &gb)) {
                k_gotoXY(x, form_y+16); printf("Gerbong harus angka > 0. Tekan tombol...");
                _getch(); continue;
            }
            k->gerbong = gb;
        }

        if (st) {
            k->active = (st == 'A') ? 1 : 0;
        }

        kereta_save();
        k_gotoXY(x, form_y+16);
        printf("Berhasil ubah. Tekan tombol...");
        _getch();
        return;
    }
}

/* ======================= Public API ======================= */
void kereta_init(void) {
    static int seeded = 0;
    if (seeded) return;
    seeded = 1;

    srand((unsigned int)time(NULL));

    kereta_load();
    if (g_keretaCount > 0) return;

    /* Dummy 50 data (ID random, nama Argo daerah acak) */
    g_keretaCount = 0;
    for (int i = 0; i < 50 && g_keretaCount < MAX_RECORDS; i++) {
        Kereta k;
        memset(&k, 0, sizeof(k));

        gen_kode_acak(k.kode);
        gen_nama_argo(k.nama);

        int r = rand() % 3;
        if (r == 0) strcpy(k.kelas, "Ekonomi");
        else if (r == 1) strcpy(k.kelas, "Eksekutif");
        else strcpy(k.kelas, "Bisnis");

        k.gerbong = (rand() % 7) + 6;
        k.active = (rand() % 10 < 8) ? 1 : 0;

        g_kereta[g_keretaCount++] = k;
    }
    kereta_save();
}

/* ======================= MAIN VIEW ======================= */
void view_kereta(void) {
    int page = 0;

    while (1) {
        int w = k_screen_w(); if (w<=0) w=120;
        int h = k_screen_h(); if (h<=0) h=30;
        int split_x = w/4;
        int header_h = 7;

        int x = split_x + 4;
        int top_y = header_h + 4;

        int max_rows = h - (top_y + 8);
        if (max_rows < 5) max_rows = 5;
        int page_size = max_rows;

        int total_pages = (g_keretaCount + page_size - 1) / page_size;
        if (total_pages < 1) total_pages = 1;

        if (page < 0) page = total_pages - 1;
        if (page >= total_pages) page = 0;

        k_cls();
        k_draw_layout_base(w, h, "Kelola Kereta");

        render_table(split_x, w, top_y, page, page_size);

        k_gotoXY(x, h - 7);
        printf("Halaman %d/%d  |  Gunakan <- -> (Home/End) untuk pindah halaman", page + 1, total_pages);

        k_gotoXY(x, h - 6);
        printf("[1] Tambah   [2] Ubah   [3] Hapus   [4] Detail   [0] Kembali");

        k_gotoXY(x, h - 4);
        printf("Pilih Menu [0-4] : ");

        int ch = _getch();

        if (ch == EXT_0 || ch == EXT_224) {
            int ext = _getch();
            if (ext == K_LEFT || ext == K_HOME) {
                page--;
                if (page < 0) page = total_pages - 1;
                continue;
            }
            if (ext == K_RIGHT || ext == K_END) {
                page++;
                if (page >= total_pages) page = 0;
                continue;
            }
            continue;
        }

        if (ch == '0') return;
        if (ch == '1') action_tambah();
        else if (ch == '2') action_ubah();
        else if (ch == '3') action_hapus_status();
        else if (ch == '4') action_detail();

        total_pages = (g_keretaCount + page_size - 1) / page_size;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;
    }
}
    