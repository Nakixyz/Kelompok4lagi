#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <string.h>
#include "ui_app.h"
#include "globals.h"

// Include Modul Logic
#include "modul_akun.h"
#include "modul_karyawan.h"
#include "modul_stasiun.h"
#include "modul_kereta.h"

/* --- Helper Dasar --- */
void cls() { system("cls"); }

void gotoXY(int x, int y) {
    if (x < 0) x = 0; if (y < 0) y = 0;
    if (x > 250) x = 250; if (y > 80) y = 80;
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

int get_screen_width() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

int get_screen_height() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void input_password_masked(char *buffer, int max_len) {
    int i = 0;
    char ch;
    while (1) {
        ch = _getch();
        if (ch == 13) { buffer[i] = '\0'; break; }
        else if (ch == 8) { if (i > 0) { i--; printf("\b \b"); } }
        else if (ch == 27) { buffer[0] = 27; return; }
        else if (i < max_len - 1 && ch >= 32 && ch <= 126) {
            buffer[i] = ch; i++; printf("*");
        }
    }
}

/* --- GAMBAR LAYOUT DASAR --- */
void draw_layout_base(int w, int h, const char* section_title) {
    int split_x = w / 4;
    int header_h = 7;

    // Frame Utama
    for(int x=0; x<w; x++) { gotoXY(x, 0); printf("="); gotoXY(x, header_h); printf("="); gotoXY(x, h-2); printf("="); }
    for(int y=0; y<h-1; y++) { gotoXY(0, y); printf("||"); gotoXY(split_x, y); printf("||"); gotoXY(w-2, y); printf("||"); }

    // Header Title
    int content_w = w - split_x;
    int title_x = split_x + (content_w / 2) - 32;
    if (title_x < split_x + 2) title_x = split_x + 2;

    gotoXY(title_x, 1); printf(" _______ _             ______                 _            _       ");
    gotoXY(title_x, 2); printf("|__   __| |           |  ____|               | |          (_)      ");
    gotoXY(title_x, 3); printf("   | |  | |__   ___   | |__ ___  _   _ _ __| |_ _ __ __ _ _ _ __ ");
    gotoXY(title_x, 4); printf("   | |  | '_ \\ / _ \\  |  __/ _ \\| | | | '__| __| '__/ _` | | '_ \\");
    gotoXY(title_x, 5); printf("   | |  | | | |  __/  | | | (_) | |_| | |  | |_| | | (_| | | | | |");
    gotoXY(title_x, 6); printf("   |_|  |_| |_|\\___|  |_|  \\___/ \\__,_|_|   \\__|_|  \\__,_|_|_| |_|");

    // Sidebar Info
    gotoXY(5, 3); printf("- - Kelompok 4 - -");
    gotoXY(5, 5); printf("Role: %s", section_title);

    // Label Halaman (Ganti "Login >" jadi nama halaman)
    gotoXY(split_x + 2, header_h + 1); printf(" %s >", section_title);
}

/* ================== MENU CRUD KARYAWAN (CONTOH) ================== */
void view_karyawan() {
    while(1) {
        int w = get_screen_width(); if(w<=0) w=120;
        int h = get_screen_height(); if(h<=0) h=30;

        cls();
        draw_layout_base(w, h, "Kelola Karyawan");

        int split_x = w / 4;
        int center_x = split_x + ((w - split_x) / 2);
        int top = 10;

        // Tampilkan Tabel Sederhana
        gotoXY(split_x + 4, top); printf("%-4s %-10s %-25s %-15s %-10s", "No", "ID", "Nama", "Jabatan", "Status");
        gotoXY(split_x + 4, top+1); for(int i=0; i<w-split_x-10; i++) printf("-");

        int row = top + 2;
        int count = 0;
        for(int i=0; i<g_karyawanCount; i++) {
            if(g_karyawan[i].active) {
                if(count < 15) {
                    gotoXY(split_x + 4, row++);
                    printf("%-4d %-10s %-25s %-15s %-10s", count+1, g_karyawan[i].id, g_karyawan[i].nama, g_karyawan[i].jabatan, "Aktif");
                }
                count++;
            }
        }

        gotoXY(split_x + 4, h - 6); printf("[1] Tambah Data");
        gotoXY(split_x + 25, h - 6); printf("[2] Hapus Data");
        gotoXY(split_x + 4, h - 4); printf("[0] Kembali ke Dashboard");

        char ch = _getch();
        if(ch == '0') return; // Kembali ke dashboard

        // Logika Tambah/Hapus (Simple version)
        if (ch == '1') {
             // Logic create popup
             gotoXY(split_x + 4, h-3); printf(">> Fitur Tambah dipanggil..."); Sleep(500);
             // Panggil karyawan_create(...) di sini nanti
        }
        else if (ch == '2') {
             gotoXY(split_x + 4, h-3); printf(">> Fitur Hapus dipanggil..."); Sleep(500);
        }
    }
}

/* ===== Helper input untuk popup ===== */
static void trim_newline(char *s) { s[strcspn(s, "\n")] = '\0'; }

static void show_cursor(int visible) {
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    ci.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

static void input_at(int x, int y, const char *label, char *buf, int sz) {
    gotoXY(x, y); printf("%s", label);
    gotoXY(x + (int)strlen(label), y);
    fgets(buf, sz, stdin);
    trim_newline(buf);
}

static void generate_next_stasiun_id(char out[10]) {
    int maxNum = 0;

    // scan semua record (aktif/nonaktif) biar id selalu lanjut, tidak ngulang
    for (int i = 0; i < g_stasiunCount; i++) {
        int num = 0;
        if (sscanf(g_stasiun[i].id, "STS%d", &num) == 1) {
            if (num > maxNum) maxNum = num;
        }
    }

    snprintf(out, 10, "STS%03d", maxNum + 1);
}

// Ambil index array berdasarkan nomor tampil (No) di tabel (1 = data aktif pertama)
static int stasiun_find_index_by_no(int no) {
    if (no <= 0) return -1;

    int count = 0;
    for (int i = 0; i < g_stasiunCount; i++) {
        if (g_stasiun[i].active) {
            count++;
            if (count == no) return i;
        }
    }
    return -1;
}


/* ===== Popup: CREATE ===== */
static void popup_stasiun_create() {
    int w = get_screen_width(); if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;

    cls();
    draw_layout_base(w, h, "Tambah Stasiun");

    int split_x = w / 4;
    int x = split_x + 4;
    int y = 10;

    char id[10], kode[10], nama[50], kota[30], alamat[100];

    generate_next_stasiun_id(id);

    gotoXY(x, y++);
    printf("ID      : **********"); // tampil masked aja

    show_cursor(1);
    input_at(x, y++, "Kode    : ", kode, sizeof(kode));
    input_at(x, y++, "Nama    : ", nama, sizeof(nama));
    input_at(x, y++, "Kota    : ", kota, sizeof(kota));
    input_at(x, y++, "Alamat  : ", alamat, sizeof(alamat));
    show_cursor(0);

    if (kode[0] == '\0' || nama[0] == '\0') {
        gotoXY(x, y+1); printf(">> Gagal: Kode/Nama tidak boleh kosong.");
    } else {
        stasiun_create(id, kode, nama, kota, alamat);
        gotoXY(x, y+1); printf(">> Berhasil tambah stasiun.");
    }

    gotoXY(x, y+3); printf("Tekan tombol apa saja untuk kembali...");
    _getch();
}

/* ===== Popup: DETAIL (READ by id) ===== */
static void popup_stasiun_detail() {
    int w = get_screen_width(); if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;

    cls();
    draw_layout_base(w, h, "Detail Stasiun");

    int split_x = w / 4;
    int x = split_x + 4;
    int y = 10;

    char buf[16];
    show_cursor(1);
    input_at(x, y++, "Masukkan No Stasiun : ", buf, sizeof(buf));
    show_cursor(0);

    int no = atoi(buf);
    int idx = stasiun_find_index_by_no(no);

    if (idx == -1) {
        gotoXY(x, y+1); printf(">> Data tidak ditemukan.");
    } else {
        gotoXY(x, y+1); printf("ID     : %s", g_stasiun[idx].id);   // di detail boleh kelihatan
        gotoXY(x, y+2); printf("Kode   : %s", g_stasiun[idx].kode);
        gotoXY(x, y+3); printf("Nama   : %s", g_stasiun[idx].nama);
        gotoXY(x, y+4); printf("Kota   : %s", g_stasiun[idx].kota);
        gotoXY(x, y+5); printf("Alamat : %s", g_stasiun[idx].alamat);
    }

    gotoXY(x, y+7); printf("Tekan tombol apa saja untuk kembali...");
    _getch();
}


/* ===== Popup: UPDATE ===== */
static void popup_stasiun_update() {
    int w = get_screen_width(); if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;

    cls();
    draw_layout_base(w, h, "Update Stasiun");

    int split_x = w / 4;
    int x = split_x + 4;
    int y = 10;

    char id[10], kode[10], nama[50], kota[30], alamat[100];

    show_cursor(1);
    input_at(x, y++, "Masukkan ID Stasiun : ", id, sizeof(id));
    show_cursor(0);

    int idx = stasiun_find_index_by_id(id);
    if (idx == -1) {
        gotoXY(x, y+1); printf(">> Stasiun tidak ditemukan.");
        gotoXY(x, y+3); printf("Tekan tombol apa saja untuk kembali...");
        _getch();
        return;
    }

    gotoXY(x, y);   printf("Data lama:");
    gotoXY(x, y+1); printf("Kode   : %s", g_stasiun[idx].kode);
    gotoXY(x, y+2); printf("Nama   : %s", g_stasiun[idx].nama);
    gotoXY(x, y+3); printf("Kota   : %s", g_stasiun[idx].kota);
    gotoXY(x, y+4); printf("Alamat : %s", g_stasiun[idx].alamat);

    y += 6;
    gotoXY(x, y++); printf("Masukkan data baru (kosongkan untuk tetap):");

    show_cursor(1);
    input_at(x, y++, "Kode   : ", kode, sizeof(kode));
    input_at(x, y++, "Nama   : ", nama, sizeof(nama));
    input_at(x, y++, "Kota   : ", kota, sizeof(kota));
    input_at(x, y++, "Alamat : ", alamat, sizeof(alamat));
    show_cursor(0);

    /* kalau input kosong, pakai data lama */
    if (kode[0] == '\0') strncpy(kode, g_stasiun[idx].kode, sizeof(kode)-1), kode[sizeof(kode)-1]=0;
    if (nama[0] == '\0') strncpy(nama, g_stasiun[idx].nama, sizeof(nama)-1), nama[sizeof(nama)-1]=0;
    if (kota[0] == '\0') strncpy(kota, g_stasiun[idx].kota, sizeof(kota)-1), kota[sizeof(kota)-1]=0;
    if (alamat[0] == '\0') strncpy(alamat, g_stasiun[idx].alamat, sizeof(alamat)-1), alamat[sizeof(alamat)-1]=0;

    stasiun_update(id, kode, nama, kota, alamat);

    gotoXY(x, y+1); printf(">> Berhasil update stasiun.");
    gotoXY(x, y+3); printf("Tekan tombol apa saja untuk kembali...");
    _getch();
}

/* ===== Popup: DELETE ===== */
static void popup_stasiun_delete() {
    int w = get_screen_width(); if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;

    cls();
    draw_layout_base(w, h, "Hapus Stasiun");

    int split_x = w / 4;
    int x = split_x + 4;
    int y = 10;

    char id[10];

    show_cursor(1);
    input_at(x, y++, "Masukkan ID Stasiun : ", id, sizeof(id));
    show_cursor(0);

    int idx = stasiun_find_index_by_id(id);
    if (idx == -1) {
        gotoXY(x, y+1); printf(">> Stasiun tidak ditemukan.");
        gotoXY(x, y+3); printf("Tekan tombol apa saja untuk kembali...");
        _getch();
        return;
    }

    gotoXY(x, y);   printf("Data yang akan dihapus:");
    gotoXY(x, y+1); printf("ID   : %s", g_stasiun[idx].id);
    gotoXY(x, y+2); printf("Nama : %s", g_stasiun[idx].nama);
    gotoXY(x, y+3); printf("Kota : %s", g_stasiun[idx].kota);

    gotoXY(x, y+5); printf("Yakin hapus? (Y/N): ");
    int c = _getch();
    if (c == 'y' || c == 'Y') {
        stasiun_delete_by_id(g_stasiun[idx].id);
        gotoXY(x, y+7); printf(">> Berhasil hapus (soft delete).");
    } else {
        gotoXY(x, y+7); printf(">> Dibatalkan.");
    }

    gotoXY(x, y+9); printf("Tekan tombol apa saja untuk kembali...");
    _getch();
}

static void mask_text(const char *src, char *dst, size_t dst_sz) {
    if (!src || !dst || dst_sz == 0) return;
    size_t n = strlen(src);
    if (n >= dst_sz) n = dst_sz - 1;
    for (size_t i = 0; i < n; i++) dst[i] = '*';
    dst[n] = '\0';
}

static void draw_box(int x, int y, int bw, int bh, const char *title) {
    // Top border
    gotoXY(x, y); printf("+");
    for (int i = 0; i < bw - 2; i++) printf("-");
    printf("+");

    // Title (opsional)
    if (title && title[0] != '\0' && bw > 4) {
        int tx = x + 2;
        gotoXY(tx, y);
        printf("[%s]", title);
    }

    // Sides
    for (int r = 1; r < bh - 1; r++) {
        gotoXY(x, y + r); printf("|");
        gotoXY(x + bw - 1, y + r); printf("|");
    }

    // Bottom border
    gotoXY(x, y + bh - 1); printf("+");
    for (int i = 0; i < bw - 2; i++) printf("-");
    printf("+");
}

// Lebar isi kolom (tanpa spasi padding)
#define COL_NO     4
#define COL_ID     10
#define COL_KODE   10
#define COL_NAMA   20
#define COL_KOTA   15
#define COL_ALAMAT 20

static void draw_table_hline(int x, int y) {
    gotoXY(x, y);
    printf("+");
    for (int i = 0; i < COL_NO + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_ID + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_KODE + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_NAMA + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_KOTA + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_ALAMAT + 2; i++) printf("-");
    printf("+");
}

static void print_table_header(int x, int y) {
    gotoXY(x, y);
    printf("| %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s |",
           COL_NO, COL_NO, "No",
           COL_ID, COL_ID, "ID",
           COL_KODE, COL_KODE, "Kode",
           COL_NAMA, COL_NAMA, "Nama",
           COL_KOTA, COL_KOTA, "Kota",
           COL_ALAMAT, COL_ALAMAT, "Alamat");
}

static WORD g_attr_normal = 0;

static void init_console_attr_once() {
    if (g_attr_normal != 0) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    g_attr_normal = csbi.wAttributes;  // simpan warna default yg lagi jalan
}

static void set_highlight(int on) {
    init_console_attr_once();

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    if (on) {
        // background putih, tulisan hitam
        WORD bg = (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY); // putih
        WORD fg = 0; // hitam
        SetConsoleTextAttribute(h, fg | bg);
    } else {
        // balik ke warna normal program
        SetConsoleTextAttribute(h, g_attr_normal);
    }
}




static void print_table_row(int x, int y, int no,
                            const char *id, const char *kode,
                            const char *nama, const char *kota,
                            const char *alamat,
                            int selected) {
    char no_str[16];
    if (no <= 0) strcpy(no_str, "");
    else snprintf(no_str, sizeof(no_str), "%d", no);

    char id_mask[COL_ID + 1];
    for (int i = 0; i < COL_ID; i++) id_mask[i] = '*';
    id_mask[COL_ID] = '\0';

    gotoXY(x, y);
    set_highlight(selected);

    printf("| %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s |",
           COL_NO, COL_NO, no_str,
           COL_ID, COL_ID, (no <= 0 ? "" : id_mask),
           COL_KODE, COL_KODE, (no <= 0 ? "" : (kode ? kode : "")),
           COL_NAMA, COL_NAMA, (no <= 0 ? "" : (nama ? nama : "")),
           COL_KOTA, COL_KOTA, (no <= 0 ? "" : (kota ? kota : "")),
           COL_ALAMAT, COL_ALAMAT, (no <= 0 ? "" : (alamat ? alamat : "")));

    set_highlight(0);
}

static void clear_rect(int x, int y, int w, int h) {
    for (int r = 0; r < h; r++) {
        gotoXY(x, y + r);
        for (int c = 0; c < w; c++) putchar(' ');
    }
}

static void print_msg(int x, int y, const char *msg) {
    gotoXY(x, y);
    printf("%s", msg);
}

/* input inline (tanpa pindah halaman)
   - ENTER = selesai
   - BACKSPACE = hapus
   - '0' saat buffer masih kosong = KEMBALI
   return 1 sukses, 0 batal/kembali */
static int input_inline_cancel(int x, int y, const char *label, char *buf, int sz) {
    gotoXY(x, y);
    printf("%s", label);
    int cx = x + (int)strlen(label);
    gotoXY(cx, y);

    int i = 0;
    while (1) {
        int ch = _getch();

        if (ch == 13) { // ENTER
            buf[i] = '\0';
            return 1;
        }
        if (ch == 8) { // BACKSPACE
            if (i > 0) {
                i--;
                gotoXY(cx + i, y); putchar(' ');
                gotoXY(cx + i, y);
            }
            continue;
        }

        if (ch == '0' && i == 0) { // kembali
            buf[0] = '\0';
            return 0;
        }

        if (ch >= 32 && ch <= 126 && i < sz - 1) {
            buf[i++] = (char)ch;
            putchar((char)ch);
        }
    }
}

static int count_stasiun_active() {
    int total = 0;
    for (int i = 0; i < g_stasiunCount; i++) {
        if (g_stasiun[i].active) total++;
    }
    return total;
}

static int calc_table_width() {
    // sesuai draw_table_hline() kamu: "+" + (COL+2 dashes + "+") tiap kolom
    int w = 1; // plus pertama
    w += (COL_NO + 2) + 1;
    w += (COL_ID + 2) + 1;
    w += (COL_KODE + 2) + 1;
    w += (COL_NAMA + 2) + 1;
    w += (COL_KOTA + 2) + 1;
    w += (COL_ALAMAT + 2) + 1;
    return w; // sudah termasuk '+' terakhir
}

// isi cache data per halaman: row_idx[r] = index asli g_stasiun, row_has[r]=1 kalau ada data
static void build_page_cache(int page, int per_page, int row_idx[], int row_has[]) {
    for (int r = 0; r < per_page; r++) { row_idx[r] = -1; row_has[r] = 0; }

    int start = page * per_page;
    int end   = start + per_page;

    int shown = 0;
    int filled = 0;

    for (int i = 0; i < g_stasiunCount; i++) {
        if (!g_stasiun[i].active) continue;

        if (shown < start) { shown++; continue; }
        if (shown >= end) break;

        row_idx[filled] = i;
        row_has[filled] = 1;
        filled++;
        shown++;
        if (filled >= per_page) break;
    }
}

static void repaint_one_row(int tx, int ty, int page, int per_page,
                            int r, int selected,
                            int row_idx[], int row_has[]) {
    int y = ty + 3 + r;
    int start = page * per_page;

    if (row_has[r]) {
        int i = row_idx[r];
        print_table_row(tx, y,
            start + r + 1,
            g_stasiun[i].id,
            g_stasiun[i].kode,
            g_stasiun[i].nama,
            g_stasiun[i].kota,
            g_stasiun[i].alamat,
            selected
        );
    } else {
        // kosong (no=0 => tidak tampil "0" karena print_table_row kamu sudah handle)
        print_table_row(tx, y, 0, "", "", "", "", "", selected);
    }
}

static void draw_table_full(int tx, int ty, int page, int per_page, int cursor,
                            int table_w, int row_idx[], int row_has[]) {
    // bersihkan area tabel saja (bukan seluruh layar)
    // tinggi tabel: top + header + line + 10 row + bottom line = 1 + 1 + 1 + 10 + 1 = 14
    clear_rect(tx, ty, table_w + 1, 14);

    draw_table_hline(tx, ty);
    print_table_header(tx, ty + 1);
    draw_table_hline(tx, ty + 2);

    for (int r = 0; r < per_page; r++) {
        repaint_one_row(tx, ty, page, per_page, r, (r == cursor), row_idx, row_has);
    }

    draw_table_hline(tx, ty + 3 + per_page);
}
static void draw_hline_box(int x, int y, int w) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w - 2; i++) putchar('-');
    putchar('+');
}

static void draw_text_in_box(int x, int y, int w, const char *text) {
    int inner = w - 2;
    int len = (int)strlen(text);
    if (len > inner) len = inner;

    gotoXY(x, y); putchar('|');
    for (int i = 0; i < len; i++) putchar(text[i]);
    for (int i = len; i < inner; i++) putchar(' ');
    putchar('|');
}

static int draw_wrapped_block(int x, int y, int w, int max_lines, const char *text) {
    int inner = w - 2;
    int line = 0;

    const char *p = text;
    while (*p && line < max_lines) {
        while (*p == ' ') p++;              // skip spasi depan
        if (!*p) break;

        char buf[512];
        int cur = 0;

        while (*p) {
            // ambil 1 kata
            const char *ws = p;
            int wl = 0;
            while (*p && *p != ' ') { p++; wl++; }

            // kalau kata kepanjangan > inner, potong saja
            if (cur == 0 && wl > inner) {
                memcpy(buf, ws, inner);
                cur = inner;
                buf[cur] = '\0';
                while (*p == ' ') p++;
                break;
            }

            int need = wl + (cur > 0 ? 1 : 0);
            if (cur + need <= inner) {
                if (cur > 0) buf[cur++] = ' ';
                memcpy(buf + cur, ws, wl);
                cur += wl;
                buf[cur] = '\0';
                while (*p == ' ') p++;
            } else {
                // tidak muat, balikin pointer ke awal kata untuk baris berikutnya
                p = ws;
                break;
            }
        }

        draw_text_in_box(x, y + line, w, buf);
        line++;
    }

    return line; // jumlah baris yang dipakai
}

static void draw_info_panel_wrap(int x, int y, int w, int body_lines,
                                 const char *t1, const char *t2, const char *t3) {
    // total tinggi = top + body_lines + bottom
    draw_hline_box(x, y, w);

    int line = 0;
    line += draw_wrapped_block(x, y + 1 + line, w, body_lines - line, t1);
    line += draw_wrapped_block(x, y + 1 + line, w, body_lines - line, t2);
    line += draw_wrapped_block(x, y + 1 + line, w, body_lines - line, t3);

    // sisa baris kosong biar kotaknya penuh
    for (; line < body_lines; line++) {
        draw_text_in_box(x, y + 1 + line, w, "");
    }

    draw_hline_box(x, y + 1 + body_lines, w);
}

void view_stasiun() {
    int page = 0;
    const int per_page = 10;
    int cursor = 0;

    int row_idx[10];
    int row_has[10];

    int tx = 0, ty = 9, table_w = 0;

    int need_full_layout = 1;   // cls + draw_layout_base
    int need_table_redraw = 1;  // redraw tabel full
    int need_footer_redraw = 1; // redraw footer/info
    int need_action_clear = 1;  // bersihkan area aksi

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_x1 = split_x + 2;
        int content_x2 = w - 3;
        int content_w  = content_x2 - content_x1 + 1;

        // hitung total & total_pages
        int total = count_stasiun_active();
        int total_pages = (total + per_page - 1) / per_page;
        if (total_pages < 1) total_pages = 1;

        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        if (cursor < 0) cursor = 0;
        if (cursor > per_page - 1) cursor = per_page - 1;

        // hitung posisi tabel tengah
        table_w = calc_table_width();
        tx = content_x1 + (content_w - table_w) / 2;
        if (tx < content_x1) tx = content_x1;
        ty = 9;

        // 1) gambar layout sekali (biar nggak kedip)
        if (need_full_layout) {
            cls();
            draw_layout_base(w, h, "Kelola Stasiun");
            need_full_layout = 0;
            need_table_redraw = 1;
            need_footer_redraw = 1;
            need_action_clear = 1;
        }

        // 2) build cache halaman & gambar tabel full jika perlu
        if (need_table_redraw) {
            build_page_cache(page, per_page, row_idx, row_has);
            draw_table_full(tx, ty, page, per_page, cursor, table_w, row_idx, row_has);
            need_table_redraw = 0;
            need_footer_redraw = 1;
            need_action_clear = 1;
        }

        // ===== INFO PANEL (AUTO WRAP, NGGAK KEPOTONG) =====
        int info_y = ty + 3 + per_page + 1;   // tepat di bawah garis tabel
        int panel_w = table_w;                // sama dengan lebar tabel
        int panel_x = tx;                     // sejajar dengan tabel

        const int panel_body_lines = 6;       // isi box (naikkan kalau mau lebih longgar)

        // bersihkan area panel: top + body + bottom = panel_body_lines + 2
        clear_rect(panel_x, info_y, panel_w, panel_body_lines + 2);

        char line1[256];
        snprintf(line1, sizeof(line1),
                 " Total Aktif: %d | Halaman: %d/%d | [<-] Sebelumnya [->] Berikutnya ",
                 total, page + 1, total_pages);

        const char *line2 =
        " [UP/DOWN] Pilih  [LEFT/RIGHT] Halaman  [ENTER] Detail  [E] Edit  [X] Hapus  [A] Tambah  [0] Kembali ";

        const char *line3 =
        " Aksi: pilih data dulu (UP/DOWN), lalu tekan tombol aksi. ";

        draw_info_panel_wrap(panel_x, info_y, panel_w, panel_body_lines, line1, line2, line3);

        // ===== AREA AKSI INLINE (di bawah panel) =====
        int ax = tx;
        int ay = info_y + (panel_body_lines + 2) + 1; // setelah box panel + 1 spasi
        int aw = table_w + 2;
        int ah = 10;

        clear_rect(ax, ay, aw, ah);

        int ch = _getch();

        // panah keyboard
        if (ch == 0 || ch == 224) {
            int k = _getch();

            int old_cursor = cursor;
            int old_page = page;

            if (k == 72) { // UP
                if (cursor > 0) cursor--;
            } else if (k == 80) { // DOWN
                if (cursor < per_page - 1) cursor++;
            } else if (k == 75) { // LEFT
                if (page > 0) { page--; cursor = 0; }
            } else if (k == 77) { // RIGHT
                if (page < total_pages - 1) { page++; cursor = 0; }
            }

            // ganti halaman => redraw full tabel (sekali)
            if (page != old_page) {
                need_table_redraw = 1;
                continue;
            }

            // cuma pindah cursor => repaint 2 baris saja (NO BLINK)
            if (cursor != old_cursor) {
                repaint_one_row(tx, ty, page, per_page, old_cursor, 0, row_idx, row_has);
                repaint_one_row(tx, ty, page, per_page, cursor, 1, row_idx, row_has);
            }

            continue;
        }

        if (ch == '0') return;

        // ========= INLINE TAMBAH =========
        if (ch == 'a' || ch == 'A') {
            clear_rect(ax, ay, aw, ah);

            gotoXY(ax, ay); printf("Tambah Stasiun (Tekan 0 untuk kembali saat input kosong)");
            gotoXY(ax, ay + 1); printf("ID     : **********");

            char id[10], kode[10], nama[50], kota[30], alamat[100];
            generate_next_stasiun_id(id);

            show_cursor(1);
            if (!input_inline_cancel(ax, ay + 3, "Kode   : ", kode, sizeof(kode))) { show_cursor(0); need_action_clear=1; continue; }
            if (!input_inline_cancel(ax, ay + 4, "Nama   : ", nama, sizeof(nama))) { show_cursor(0); need_action_clear=1; continue; }
            if (!input_inline_cancel(ax, ay + 5, "Kota   : ", kota, sizeof(kota))) { show_cursor(0); need_action_clear=1; continue; }
            if (!input_inline_cancel(ax, ay + 6, "Alamat : ", alamat, sizeof(alamat))) { show_cursor(0); need_action_clear=1; continue; }
            show_cursor(0);

            if (kode[0] == '\0' || nama[0] == '\0') {
                gotoXY(ax, ay + 8); printf("Gagal: Kode/Nama tidak boleh kosong. Tekan apa saja...");
                _getch();
            } else {
                stasiun_create(id, kode, nama, kota, alamat);
                gotoXY(ax, ay + 8); printf("Berhasil tambah data. Tekan apa saja...");
                _getch();
                need_table_redraw = 1; // refresh data
            }
            continue;
        }

        // kalau baris kosong, aksi selain tambah diabaikan
        if (!row_has[cursor]) {
            continue;
        }

        int idx = row_idx[cursor];
        if (idx < 0 || idx >= g_stasiunCount) continue;

        // ========= DETAIL INLINE =========
        if (ch == 13) {
            clear_rect(ax, ay, aw, ah);

            gotoXY(ax, ay);     printf("Detail Stasiun (Tekan apa saja untuk kembali)");
            gotoXY(ax, ay + 2); printf("ID     : %s", g_stasiun[idx].id);
            gotoXY(ax, ay + 3); printf("Kode   : %s", g_stasiun[idx].kode);
            gotoXY(ax, ay + 4); printf("Nama   : %s", g_stasiun[idx].nama);
            gotoXY(ax, ay + 5); printf("Kota   : %s", g_stasiun[idx].kota);
            gotoXY(ax, ay + 6); printf("Alamat : %s", g_stasiun[idx].alamat);

            _getch();
            need_action_clear = 1;
            continue;
        }

        // ========= HAPUS INLINE =========
        if (ch == 'x' || ch == 'X') {
            clear_rect(ax, ay, aw, ah);

            gotoXY(ax, ay);     printf("Hapus Stasiun? (Y/N)");
            gotoXY(ax, ay + 2); printf("ID   : %s", g_stasiun[idx].id);
            gotoXY(ax, ay + 3); printf("Nama : %s", g_stasiun[idx].nama);

            int c2 = _getch();
            if (c2 == 'y' || c2 == 'Y') {
                stasiun_delete_by_id(g_stasiun[idx].id);
                gotoXY(ax, ay + 5); printf("Berhasil hapus. Tekan apa saja...");
                _getch();

                // setelah delete, bisa jadi halaman terakhir berkurang
                need_table_redraw = 1;
            } else {
                need_action_clear = 1;
            }
            continue;
        }

        // ========= EDIT INLINE =========
        if (ch == 'e' || ch == 'E') {
            clear_rect(ax, ay, aw, ah);

            gotoXY(ax, ay); printf("Edit Stasiun (kosongkan untuk tetap)  |  (Tekan 0 untuk kembali saat input kosong)");
            gotoXY(ax, ay + 1); printf("ID : %s", g_stasiun[idx].id);

            char kode[10], nama[50], kota[30], alamat[100];
            kode[0]=nama[0]=kota[0]=alamat[0]='\0';

            show_cursor(1);
            if (!input_inline_cancel(ax, ay + 3, "Kode Baru   : ", kode, sizeof(kode))) { show_cursor(0); need_action_clear=1; continue; }
            if (!input_inline_cancel(ax, ay + 4, "Nama Baru   : ", nama, sizeof(nama))) { show_cursor(0); need_action_clear=1; continue; }
            if (!input_inline_cancel(ax, ay + 5, "Kota Baru   : ", kota, sizeof(kota))) { show_cursor(0); need_action_clear=1; continue; }
            if (!input_inline_cancel(ax, ay + 6, "Alamat Baru : ", alamat, sizeof(alamat))) { show_cursor(0); need_action_clear=1; continue; }
            show_cursor(0);

            if (kode[0] == '\0') strncpy(kode, g_stasiun[idx].kode, sizeof(kode)-1), kode[sizeof(kode)-1]=0;
            if (nama[0] == '\0') strncpy(nama, g_stasiun[idx].nama, sizeof(nama)-1), nama[sizeof(nama)-1]=0;
            if (kota[0] == '\0') strncpy(kota, g_stasiun[idx].kota, sizeof(kota)-1), kota[sizeof(kota)-1]=0;
            if (alamat[0] == '\0') strncpy(alamat, g_stasiun[idx].alamat, sizeof(alamat)-1), alamat[sizeof(alamat)-1]=0;

            stasiun_update(g_stasiun[idx].id, kode, nama, kota, alamat);

            gotoXY(ax, ay + 8); printf("Berhasil update. Tekan apa saja...");
            _getch();

            need_table_redraw = 1;
            continue;
        }
    }
}


/* ================== DASHBOARD MENU UTAMA ================== */
void dashboard_main(char* username) {
    while(1) {
        int w = get_screen_width(); if(w<=0) w=120;
        int h = get_screen_height(); if(h<=0) h=30;

        cls();
        draw_layout_base(w, h, "Dashboard");

        int split_x = w / 4;
        int content_w = w - split_x;
        int center_x = split_x + (content_w / 2);
        int center_y = h / 2;

        // Selamat Datang
        char welcome[100];
        snprintf(welcome, 100, "Selamat Datang, %s!", username);
        gotoXY(center_x - (strlen(welcome)/2), center_y - 8);
        printf("%s", welcome);

        // Menu Options (Kotak)
        int box_w = 40;
        int box_x = center_x - (box_w / 2);
        int start_y = center_y - 4;

        char* menus[] = {
            "[1] Kelola Akun",
            "[2] Kelola Karyawan",
            "[3] Kelola Stasiun",
            "[4] Kelola Kereta",
            "[0] Logout / Keluar"
        };

        for(int i=0; i<5; i++) {
            gotoXY(box_x, start_y + (i*2));
            printf("%s", menus[i]);
        }

        gotoXY(center_x - 10, start_y + 12); printf("Pilih Menu [0-4] : ");

        int ch = _getch();
        if (ch == '0') {
            return; // Logout (Kembali ke Login Screen)
        }
        else if (ch == '1') {
        }
        else if (ch == '2') {
            view_karyawan(); // Masuk menu karyawan
        }
        else if (ch == '3') {
            view_stasiun();
        }
        else if (ch == '4') {
        }
    }
}

/* ================== LOGIN SCREEN ================== */
void login_screen() {
    char user[64], pass[64];
    int w, h;
    int header_h = 7;

    while (1) {
        w = get_screen_width(); if(w<=0) w=120;
        h = get_screen_height(); if(h<=0) h=30;

        cls();
        draw_layout_base(w, h, "Login System"); // Pakai base layout yg sama

        int split_x = w / 4;
        int center_x = split_x + ((w - split_x) / 2);
        int form_top = (h/2) - 5;

        // Form Login
        int box_w = 50;
        int box_x = center_x - (box_w / 2);

        gotoXY(box_x, form_top); printf("Username :");
        gotoXY(box_x, form_top + 1); for(int i=0; i<box_w; i++) printf("-");

        gotoXY(box_x, form_top + 4); printf("Password :");
        gotoXY(box_x, form_top + 5); for(int i=0; i<box_w; i++) printf("-");

        gotoXY(split_x + 5, h - 4); printf("[ESC] Keluar Program  |  [ENTER] Login");

        // Input User
        gotoXY(box_x, form_top + 2);
        CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
        ci.bVisible = TRUE; SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        fflush(stdin);
        // Logic baca ESC saat input username
        user[0] = '\0';
        int idx = 0;
        while(1) {
            int c = _getch();
            if(c == 13) break; // Enter
            if(c == 27) { cls(); exit(0); } // ESC Exit
            if(c == 8) { if(idx>0) { idx--; printf("\b \b"); } }
            else if(idx < 63 && c >= 32 && c <= 126) { user[idx++] = c; printf("%c", c); }
        }
        user[idx] = '\0';

        // Input Pass
        gotoXY(box_x, form_top + 6);
        input_password_masked(pass, 63);
        if (pass[0] == 27) { cls(); exit(0); } // ESC Exit saat password

        ci.bVisible = FALSE; SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        // Validation
        int success = 0;
        if(strcmp(user, "admin") == 0 && strcmp(pass, "admin") == 0) success = 1;
        else {
            for(int i=0; i<g_accountCount; i++) {
                if(g_accounts[i].active && strcmp(g_accounts[i].username, user)==0 && strcmp(g_accounts[i].password, pass)==0) {
                    success = 1; break;
                }
            }
        }

        if (success) {
            gotoXY(center_x - 10, form_top + 9); printf(">> LOGIN BERHASIL! <<");
            Sleep(800);

            // INI PERBAIKANNYA: Panggil Dashboard, Jangan Break/Return!
            dashboard_main(user);

            // Kalau dashboard_main selesai (user pilih logout), loop ini ngulang login lagi
        } else {
            Beep(500, 300);
            gotoXY(center_x - 15, form_top + 9); printf(">> USERNAME / PASSWORD SALAH <<");
            Sleep(1500);
        }
    }
}

void ui_init() {
    akun_init();
    karyawan_init();
    stasiun_init();
    kereta_init();
}

void ui_run() {
    login_screen();
}