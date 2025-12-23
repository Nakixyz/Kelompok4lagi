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
#include "modul_penumpang.h"
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

/* --- Console Color Helper (highlight selection) --- */
static WORD g_defaultAttr = 0;

static void init_console_attr() {
    if (g_defaultAttr != 0) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        g_defaultAttr = csbi.wAttributes;
    } else {
        g_defaultAttr = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

static void set_highlight(int on) {
    init_console_attr();
    if (on) {
        WORD attr = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
                     BACKGROUND_BLUE | BACKGROUND_GREEN);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
    } else {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), g_defaultAttr);
    }
}



/* Password input: TAB show/hide, spasi ditolak */
void input_password_masked(char *buffer, int max_len, int x, int y, int field_w) {
    int i = 0;
    int visible = 0;

    int limit = max_len - 1;
    if (field_w > 0 && field_w < limit) limit = field_w;

    buffer[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        // ENTER
        if (ch == 27) { buffer[0] = 27; return; }         // ESC

        if (ch == 9) {                                    // TAB toggle
            visible = !visible;

            gotoXY(x, y);
            for (int k = 0; k < field_w; k++) putchar(' ');
            gotoXY(x, y);
            for (int k = 0; k < i; k++) putchar(visible ? buffer[k] : '*');
            continue;
        }

        if (ch == 8) {                                    // BACKSPACE
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {                      // printable
            if (ch == ' ') { Beep(800, 40); continue; }   // tolak spasi
            if (i < limit) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar(visible ? (char)ch : '*');
            } else {
                Beep(800, 40);
            }
        }
    }
}

/* Input teks umum (allow_space=0 => spasi ditolak & tidak tampil) */
void input_text(char *buffer, int max_len, int allow_space) {
    int i = 0;
    buffer[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        // ENTER
        if (ch == 27) { buffer[0] = 27; return; }         // ESC

        if (ch == 8) {                                    // BACKSPACE
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (!allow_space && ch == ' ') {                  // tolak spasi
            Beep(800, 40);
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            if (i < max_len - 1) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                Beep(800, 40);
            }
        }
    }
}

/* Input angka saja (untuk No) */
void input_digits(char *buffer, int max_len) {
    int i = 0;
    buffer[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        // ENTER
        if (ch == 27) { buffer[0] = 27; return; }         // ESC

        if (ch == 8) {                                    // BACKSPACE
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch >= '0' && ch <= '9') {
            if (i < max_len - 1) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                Beep(800, 40);
            }
        } else {
            Beep(800, 40);
        }
    }
}

static void draw_popup_box(int x, int y, int w, int h, const char *title) {
    if (w < 10) w = 10;
    if (h < 5) h = 5;

    gotoXY(x, y);
    for (int i = 0; i < w; i++) putchar('=');

    for (int r = 1; r < h - 1; r++) {
        gotoXY(x, y + r); putchar('|');
        gotoXY(x + 1, y + r);
        for (int i = 0; i < w - 2; i++) putchar(' ');
        gotoXY(x + w - 1, y + r); putchar('|');
    }

    gotoXY(x, y + h - 1);
    for (int i = 0; i < w; i++) putchar('=');

    if (title && title[0]) {
        gotoXY(x + 2, y);
        printf("[ %s ]", title);
    }
}

static int is_blank(const char *s) {
    if (!s) return 1;
    while (*s) {
        if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') return 0;
        s++;
    }
    return 1;
}

static int contains_space(const char *s) {
    if (!s) return 0;
    while (*s) {
        if (*s == ' ') return 1;
        s++;
    }
    return 0;
}

static int looks_like_email(const char *s) {
    if (!s || !*s) return 0;

    /* Tidak boleh ada whitespace */
    for (const char *p = s; *p; p++) {
        if (*p == ' ' || *p == '	' || *p == ' ' || *p == ' ')
            return 0;
    }

    const char *at = strchr(s, '@');
    if (!at || at == s) return 0;          /* harus ada '@' dan tidak di awal */
    if (strchr(at + 1, '@')) return 0;     /* hanya boleh 1 '@' */

    /* local-part: [A-Za-z0-9._+-]+, tidak boleh diawali/diakhiri '.' dan tidak boleh ".." */
    if (s[0] == '.' || *(at - 1) == '.') return 0;

    char prev = 0;
    for (const char *p = s; p < at; p++) {
        char c = *p;
        int ok = ((c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '.' || c == '_' || c == '-' || c == '+');
        if (!ok) return 0;
        if (c == '.' && prev == '.') return 0;
        prev = c;
    }

    const char *domain = at + 1;
    if (!*domain) return 0;

    /* domain harus mengandung '.' dan TLD minimal 2 char: contoh gmail.com / kai.id */
    const char *dot = strrchr(domain, '.');
    if (!dot || dot == domain) return 0;
    if (*(dot + 1) == '\0') return 0;
    if ((int)strlen(dot + 1) < 2) return 0;

    /* domain: [A-Za-z0-9.-]+, tidak boleh diawali/diakhiri '.'/'-' dan tidak boleh ".." */
    if (domain[0] == '.' || domain[0] == '-') return 0;

    prev = 0;
    for (const char *p = domain; *p; p++) {
        char c = *p;
        int ok = ((c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '.' || c == '-');
        if (!ok) return 0;
        if (c == '.' && prev == '.') return 0;
        prev = c;
    }

    size_t dlen = strlen(domain);
    if (dlen == 0) return 0;
    if (domain[dlen - 1] == '.' || domain[dlen - 1] == '-') return 0;

    return 1;
}


static int karyawan_id_exists(const char *id) {
    if (!id || !*id) return 0;
    for (int i = 0; i < g_karyawanCount; i++) {
        if (strcmp(g_karyawan[i].id, id) == 0) return 1;
    }
    return 0;
}

/* mapping: No (tampilan) -> index asli array */
static int build_active_karyawan_indexes(int *out_idx, int max_out) {
    int n = 0;
    for (int i = 0; i < g_karyawanCount && n < max_out; i++) {
        if (g_karyawan[i].active) out_idx[n++] = i;
    }
    return n;
}


/* mapping: No (tampilan) -> index asli array (karyawan) [ALL: aktif + nonaktif] */
static int build_karyawan_indexes_all(int *out_idx, int max_out) {
    int n = 0;
    for (int i = 0; i < g_karyawanCount && n < max_out; i++) {
        out_idx[n++] = i; /* jangan filter active */
    }
    return n;
}


/* mapping: No (tampilan) -> index asli array (akun) */
static int build_active_account_indexes(int *out_idx, int max_out) {
    int n = 0;
    for (int i = 0; i < g_accountCount && n < max_out; i++) {
        if (g_accounts[i].active) out_idx[n++] = i;
    }
    return n;
}


/* --- GAMBAR LAYOUT DASAR --- */
void draw_layout_base(int w, int h, const char* section_title) {
    int split_x = w / 4;
    int header_h = 7;

    // Frame Utama
    for(int x=0; x<w; x++) {
        gotoXY(x, 0); printf("=");
        gotoXY(x, header_h); printf("=");
        gotoXY(x, h-2); printf("=");
    }
    for(int y=0; y<h-1; y++) {
        gotoXY(0, y); printf("||");
        gotoXY(split_x, y); printf("||");
        gotoXY(w-2, y); printf("||");
    }

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

    // Label Halaman
    gotoXY(split_x + 2, header_h + 1); printf(" %s >", section_title);
}

static const char* role_to_label(Role role);

/* ================== CRUD KARYAWAN ================== */
/* UI table-style seperti Kelola Akun:
   - UP/DOWN pilih baris
   - LEFT/RIGHT pindah halaman
   - ENTER detail
   - [A] tambah, [E] edit, [X] nonaktifkan (soft delete), [0] kembali
   - Data NONAKTIF tetap ditampilkan di tabel
*/
static void popup_center_clamped(int split_x, int content_w, int pop_w, int pop_h, int *out_x, int *out_y) {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;

    int x = split_x + (content_w - pop_w) / 2;
    int y = 8; // aman: di bawah header layout

    int min_x = split_x + 2;
    int max_x = (w - 3) - pop_w;       // sisakan border luar
    if (x < min_x) x = min_x;
    if (x > max_x) x = max_x;
    if (x < min_x) x = min_x;

    int min_y = 6;
    int max_y = (h - 3) - pop_h;
    if (y < min_y) y = min_y;
    if (y > max_y) y = max_y;
    if (y < min_y) y = min_y;

    *out_x = x;
    *out_y = y;
}

static void form_row(int pop_x, int pop_y, int row_y, int label_w, int input_w, const char *label) {
    int x_label = pop_x + 3;
    int x_input = x_label + label_w + 3; // label + " : "
    int y = pop_y + row_y;

    gotoXY(x_label, y);
    printf("%-*s : ", label_w, label);

    // bersihkan area input biar rapi
    for (int i = 0; i < input_w; i++) putchar(' ');
    gotoXY(x_input, y);
}


static void karyawan_print_hline(int x, int y,
                                int w_no, int w_id, int w_nama, int w_email, int w_jabatan, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_id + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_email + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_jabatan + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}

static void karyawan_popup_detail(int split_x, int content_w, const Karyawan *k) {
    int pop_w = 86, pop_h = 14;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 8;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Detail Karyawan");

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID      : %s", k->id);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Nama    : %s", k->nama);
    gotoXY(pop_x + 3, pop_y + 6);  printf("Email   : %s", k->email);
    gotoXY(pop_x + 3, pop_y + 8);  printf("Jabatan : %s", k->jabatan);
    gotoXY(pop_x + 3, pop_y + 10); printf("Status  : %s", k->active ? "Aktif" : "Nonaktif");

    gotoXY(pop_x + 3, pop_y + 12); printf("Tekan tombol apa saja untuk kembali...");
    _getch();
}


static void form_row_draw(int pop_x, int pop_y, int row_y,
                          int label_w, int input_w,
                          const char *label,
                          int *out_x_input, int *out_y_input) {
    int x_label = pop_x + 3;
    int x_input = x_label + label_w + 3; // label + " : "
    int y = pop_y + row_y;

    gotoXY(x_label, y);
    printf("%-*s : ", label_w, label);

    // clear area input supaya rapi
    for (int i = 0; i < input_w; i++) putchar(' ');

    if (out_x_input) *out_x_input = x_input;
    if (out_y_input) *out_y_input = y;
}

static void karyawan_popup_add(int split_x, int content_w) {
    const int pop_w = 86, pop_h = 18;
    int pop_x, pop_y;
    popup_center_clamped(split_x, content_w, pop_w, pop_h, &pop_x, &pop_y);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Karyawan");

    const int LABEL_W = 22;
    const int INPUT_W = 40;

    char new_id[16];
    char nama[64], email[64], jabatan_opt[8];

    // ==== RENDER FORM SEKALIGUS (TAMPIL LANGSUNG) ====
    int x_id, y_id, x_nama, y_nama, x_email, y_email, x_jab, y_jab;

    form_row_draw(pop_x, pop_y, 2, LABEL_W, INPUT_W, "ID", &x_id, &y_id);
    gotoXY(x_id, y_id); printf("(dibuat sistem)");

    form_row_draw(pop_x, pop_y, 4, LABEL_W, INPUT_W, "Nama", &x_nama, &y_nama);

    form_row_draw(pop_x, pop_y, 6, LABEL_W, INPUT_W, "Email", &x_email, &y_email);
    gotoXY(pop_x + 3, pop_y + 7);
    printf("  Format: nama@domain.com (contoh: user@gmail.com / user@kai.id)");

    form_row_draw(pop_x, pop_y, 9, LABEL_W, INPUT_W, "Jabatan", &x_jab, &y_jab);
    gotoXY(pop_x + 3, pop_y + 10);
    printf("  Pilih: 1=Data  2=Transaksi  3=Manager");

    // ==== INPUT (SETELAH FORM SUDAH KE-PRINT SEMUA) ====
    gotoXY(x_nama, y_nama);
    input_text(nama, 63, 1);
    if (nama[0] == 27) return;

    gotoXY(x_email, y_email);
    input_text(email, 63, 0);
    if (email[0] == 27) return;

    gotoXY(x_jab, y_jab);
    input_text(jabatan_opt, 7, 0);
    if (jabatan_opt[0] == 27) return;

    // ==== VALIDASI ====
    if (is_blank(nama) || is_blank(email) || is_blank(jabatan_opt)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 13);
        printf("Semua field wajib diisi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!looks_like_email(email)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 13);
        printf("Email tidak valid. Contoh: user@gmail.com / user@kai.id");
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Tekan tombol apa saja...");
        _getch();
        return;
    }

    const char *jabatan = NULL;
    if (strcmp(jabatan_opt, "1") == 0) jabatan = "Karyawan Data";
    else if (strcmp(jabatan_opt, "2") == 0) jabatan = "Karyawan Transaksi";
    else if (strcmp(jabatan_opt, "3") == 0) jabatan = "Manager";

    if (!jabatan) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 13);
        printf("Pilihan jabatan tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    karyawan_create_auto(new_id, sizeof(new_id), nama, email, jabatan);

    gotoXY(pop_x + 3, pop_y + 13);
    printf("Berhasil tambah karyawan. ID: %s", new_id);
    gotoXY(pop_x + 3, pop_y + 14);
    printf("Tekan tombol apa saja...");
    _getch();
}



static void karyawan_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 90, pop_h = 20;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 6;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Karyawan");

    gotoXY(pop_x + 3, pop_y + 2); printf("ID: %s", g_karyawan[idx].id);
    gotoXY(pop_x + 3, pop_y + 4); printf("Nama baru (Enter=tetap)    : ");
    gotoXY(pop_x + 3, pop_y + 6); printf("Email baru (Enter=tetap)   : ");
    gotoXY(pop_x + 3, pop_y + 8); printf("Jabatan baru [1=Data 2=Transaksi 3=Manager] (Enter=tetap) : ");

    char nama[64], email[64], jabatan_opt[8];
    nama[0] = '\0'; email[0] = '\0'; jabatan_opt[0] = '\0';

    gotoXY(pop_x + 34, pop_y + 4); input_text(nama, 63, 1);
    if (nama[0] == 27) return;

    gotoXY(pop_x + 34, pop_y + 6); input_text(email, 63, 0);
    if (email[0] == 27) return;

    gotoXY(pop_x + 70, pop_y + 8); input_text(jabatan_opt, 7, 0);
    if (jabatan_opt[0] == 27) return;

    const char *new_nama  = is_blank(nama)  ? g_karyawan[idx].nama  : nama;
    const char *new_email = is_blank(email) ? g_karyawan[idx].email : email;

    char jabatan_final[64];
    snprintf(jabatan_final, sizeof(jabatan_final), "%s", g_karyawan[idx].jabatan);

    if (!is_blank(jabatan_opt)) {
        if (strcmp(jabatan_opt, "1") == 0) snprintf(jabatan_final, sizeof(jabatan_final), "%s", "Karyawan Data");
        else if (strcmp(jabatan_opt, "2") == 0) snprintf(jabatan_final, sizeof(jabatan_final), "%s", "Karyawan Transaksi");
        else if (strcmp(jabatan_opt, "3") == 0) snprintf(jabatan_final, sizeof(jabatan_final), "%s", "Manager");
        else {
            Beep(500, 200);
            gotoXY(pop_x + 3, pop_y + 14);
            printf("Pilihan jabatan tidak valid. Tekan tombol apa saja...");
            _getch();
            return;
        }
    }

    if (!looks_like_email(new_email)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Email tidak valid. Contoh: user@gmail.com / user@kai.id. Tekan tombol apa saja...");
        _getch();
        return;
    }

    karyawan_update(idx, new_nama, new_email, jabatan_final);

    gotoXY(pop_x + 3, pop_y + 14);
    printf("Berhasil update. Tekan tombol apa saja...");
    _getch();
}

/* Soft delete: active=0, tapi tetap ditampilkan */
static void karyawan_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 78, pop_h = 14;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 9;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Karyawan");

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Ubah status karyawan %s (%s) menjadi NONAKTIF?", g_karyawan[idx].id, g_karyawan[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    printf("Konfirmasi [Y] Ya / [N] Tidak: ");

    int c = _getch();
    if (c == 'y' || c == 'Y') {
        karyawan_delete(idx);
        gotoXY(pop_x + 3, pop_y + 8);
        printf("Status berhasil jadi NONAKTIF. Tekan tombol apa saja...");
        _getch();
    }
}

void view_karyawan() {
    const int ROWS_PER_PAGE = 10;
    int page = 0;
    int selected = 0;

    const int W_NO = 3;
    const int W_ID = 8;
    const int W_NAMA = 18;
    const int W_EMAIL = 22;
    const int W_JAB = 14;
    const int W_STATUS = 10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_w = w - split_x;

        int idx_map[MAX_RECORDS];
        int total = build_karyawan_indexes_all(idx_map, MAX_RECORDS);

        int aktif = 0, nonaktif = 0;
        for (int i = 0; i < total; i++) {
            if (g_karyawan[idx_map[i]].active) aktif++;
            else nonaktif++;
        }

        int total_pages = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int rows_on_page = end - start;
        if (rows_on_page < 0) rows_on_page = 0;
        if (rows_on_page == 0) selected = 0;
        if (selected >= rows_on_page) selected = (rows_on_page > 0) ? rows_on_page - 1 : 0;
        if (selected < 0) selected = 0;

        cls();
        draw_layout_base(w, h, "Kelola Karyawan");

        int table_w = 1 + (W_NO + 3) + (W_ID + 3) + (W_NAMA + 3) + (W_EMAIL + 3) + (W_JAB + 3) + (W_STATUS + 3);
        int table_x = split_x + (content_w - table_w) / 2;
        if (table_x < split_x + 2) table_x = split_x + 2;

        int table_y = 10;

        karyawan_print_hline(table_x, table_y, W_NO, W_ID, W_NAMA, W_EMAIL, W_JAB, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_ID, "ID", W_NAMA, "Nama", W_EMAIL, "Email", W_JAB, "Jabatan", W_STATUS, "Status");
        karyawan_print_hline(table_x, table_y + 2, W_NO, W_ID, W_NAMA, W_EMAIL, W_JAB, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            if (r < rows_on_page) {
                int n = start + r;
                int idx = idx_map[n];

                const char *status = g_karyawan[idx].active ? "Aktif" : "Nonaktif";

                set_highlight(r == selected);
                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*s |",
                       W_NO, n + 1,
                       W_ID, W_ID, g_karyawan[idx].id,
                       W_NAMA, W_NAMA, g_karyawan[idx].nama,
                       W_EMAIL, W_EMAIL, g_karyawan[idx].email,
                       W_JAB, W_JAB, g_karyawan[idx].jabatan,
                       W_STATUS, status);
                set_highlight(0);
            } else {
                gotoXY(table_x, y);
                printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
                       W_NO, "", W_ID, "", W_NAMA, "", W_EMAIL, "", W_JAB, "", W_STATUS, "");
            }
        }

        karyawan_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_ID, W_NAMA, W_EMAIL, W_JAB, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        gotoXY(table_x, footer_y);
        printf("Total: %d (Aktif: %d | Nonaktif: %d) | Halaman: %d/%d | [<-] Sebelumnya [->] Berikutnya",
               total, aktif, nonaktif, page + 1, total_pages);

        gotoXY(table_x, footer_y + 1);
        printf("[UP/DOWN] Pilih | [LEFT/RIGHT] Halaman | [ENTER] Detail | [E] Edit | [X] Nonaktifkan");

        gotoXY(table_x, footer_y + 2);
        printf("[A] Tambah | [0] Kembali | Aksi: pilih data dulu, lalu tekan tombol aksi.");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { /* UP */
                if (rows_on_page > 0 && selected > 0) selected--;
                else Beep(800, 60);
                continue;
            }
            if (ext == 80) { /* DOWN */
                if (rows_on_page > 0 && selected + 1 < rows_on_page) selected++;
                else Beep(800, 60);
                continue;
            }
            if (ext == 75) { /* LEFT */
                if (page > 0) { page--; selected = 0; }
                else Beep(800, 60);
                continue;
            }
            if (ext == 77) { /* RIGHT */
                if (page + 1 < total_pages) { page++; selected = 0; }
                else Beep(800, 60);
                continue;
            }
        }

        if (ch == 'a' || ch == 'A') {
            karyawan_popup_add(split_x, content_w);
            continue;
        }

        if (ch == 13) { /* ENTER detail */
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idx_map[start + selected];
            karyawan_popup_detail(split_x, content_w, &g_karyawan[idx]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idx_map[start + selected];

            if (!g_karyawan[idx].active) {
                Beep(800, 60);
                continue;
            }

            karyawan_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idx_map[start + selected];

            if (!g_karyawan[idx].active) {
                Beep(800, 60);
                continue; /* sudah nonaktif */
            }

            karyawan_popup_delete_soft(split_x, content_w, idx);
            continue;
        }
    }
}


/* ================== CRUD AKUN (SUPERADMIN) ================== */

/* Table style seperti screenshot:
   - UP/DOWN pilih row
   - LEFT/RIGHT pindah halaman
   - ENTER detail
   - [A] tambah, [E] edit, [X] hapus, [0] kembali
*/

static void akun_print_hline(int x, int y, int w_no, int w_user, int w_role, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_user + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_role + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}

static void akun_popup_detail(int split_x, int content_w, const Account *a) {
    int pop_w = 74, pop_h = 12;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 9;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Detail Akun");

    gotoXY(pop_x + 3, pop_y + 2); printf("Username : %s", a->username);
    gotoXY(pop_x + 3, pop_y + 4); printf("Role     : %s", role_to_label(a->role));
    gotoXY(pop_x + 3, pop_y + 6); printf("Status   : %s", a->active ? "Aktif" : "Nonaktif");
    gotoXY(pop_x + 3, pop_y + 8); printf("Password : %s", "********");

    gotoXY(pop_x + 3, pop_y + 10); printf("Tekan tombol apa saja untuk kembali...");
    _getch();
}

static void akun_popup_add(int split_x, int content_w) {
    int pop_w = 78, pop_h = 18;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 7;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Akun Karyawan");

    char username[64], password[64], rolebuf[8];

    gotoXY(pop_x + 3, pop_y + 2);  printf("Username (tanpa spasi)                  : ");
    gotoXY(pop_x + 3, pop_y + 4);  printf("Password (TAB show/hide, tanpa spasi)   : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("Role [1=Data 2=Transaksi 3=Manager]     : ");

    gotoXY(pop_x + 48, pop_y + 2);
    input_text(username, 63, 0);
    if (username[0] == 27) return;

    gotoXY(pop_x + 48, pop_y + 4);
    input_password_masked(password, 63, pop_x + 48, pop_y + 4, 22);
    if (password[0] == 27) return;

    gotoXY(pop_x + 48, pop_y + 6);
    input_text(rolebuf, 7, 0);
    if (rolebuf[0] == 27) return;

    if (is_blank(username) || is_blank(password) || is_blank(rolebuf)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 12);
        printf("Semua field wajib diisi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    Role role;
    if (strcmp(rolebuf, "1") == 0) role = ROLE_DATA;
    else if (strcmp(rolebuf, "2") == 0) role = ROLE_TRANSAKSI;
    else if (strcmp(rolebuf, "3") == 0) role = ROLE_MANAGER;
    else {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 12);
        printf("Role tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    int ok = akun_create(username, password, role);

    gotoXY(pop_x + 3, pop_y + 12);
    if (ok) printf("Berhasil menambah akun. Tekan tombol apa saja...");
    else    printf("Gagal menambah akun (duplikat / input tidak valid). Tekan tombol apa saja...");

    _getch();
}

static void akun_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 82, pop_h = 22;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 5;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Akun");

    int is_admin = (strcmp(g_accounts[idx].username, "admin") == 0);

    gotoXY(pop_x + 3, pop_y + 2); printf("Username: %s", g_accounts[idx].username);
    gotoXY(pop_x + 3, pop_y + 3); printf("Role    : %s", role_to_label(g_accounts[idx].role));

    char new_user[64] = "";
    char new_pass[64] = "";
    char rolebuf[8]   = "";

    int line = pop_y + 6;

    if (!is_admin) {
        gotoXY(pop_x + 3, line); printf("Username baru (Enter=tetap, tanpa spasi)        : ");
        gotoXY(pop_x + 62, line); input_text(new_user, 63, 0);
        if (new_user[0] == 27) return;
        line += 2;
    } else {
        gotoXY(pop_x + 3, line); printf("Catatan: akun admin hanya bisa ganti password.");
        line += 2;
    }

    gotoXY(pop_x + 3, line); printf("Password baru (Enter=tetap, TAB show/hide)      : ");
    gotoXY(pop_x + 62, line); input_password_masked(new_pass, 63, pop_x + 62, line, 18);
    if (new_pass[0] == 27) return;
    line += 2;

    if (!is_admin) {
        gotoXY(pop_x + 3, line);
        printf("Role baru [1=Data 2=Transaksi 3=Manager] (Enter=tetap): ");
        gotoXY(pop_x + 70, line);
        input_text(rolebuf, 7, 0);
        if (rolebuf[0] == 27) return;
        line += 2;
    }

    int any_change = 0;
    int ok_all = 1;

    if (!is_admin && !is_blank(new_user)) {
        if (!akun_update_username(idx, new_user)) ok_all = 0;
        else any_change = 1;
    }

    if (!is_blank(new_pass)) {
        akun_change_password(idx, new_pass);
        any_change = 1;
    }

    if (!is_admin && !is_blank(rolebuf)) {
        Role r;
        if (strcmp(rolebuf, "1") == 0) r = ROLE_DATA;
        else if (strcmp(rolebuf, "2") == 0) r = ROLE_TRANSAKSI;
        else if (strcmp(rolebuf, "3") == 0) r = ROLE_MANAGER;
        else r = (Role)-1;

        if (r == (Role)-1) ok_all = 0;
        else {
            if (!akun_update_role(idx, r)) ok_all = 0;
            else any_change = 1;
        }
    }

    gotoXY(pop_x + 3, pop_y + pop_h - 3);
    if (!any_change) printf("Tidak ada perubahan. Tekan tombol apa saja...");
    else if (ok_all) printf("Berhasil update akun. Tekan tombol apa saja...");
    else             printf("Update gagal (input tidak valid/duplikat/role salah). Tekan tombol apa saja...");

    _getch();
}

static void akun_popup_delete(int split_x, int content_w, int idx) {
    int pop_w = 74, pop_h = 14;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 9;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Hapus Akun");

    if (strcmp(g_accounts[idx].username, "admin") == 0) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 3);
        printf("Akun admin tidak bisa dihapus. Tekan tombol apa saja...");
        _getch();
        return;
    }

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Hapus akun: %s (%s)?", g_accounts[idx].username, role_to_label(g_accounts[idx].role));

    gotoXY(pop_x + 3, pop_y + 6);
    printf("Konfirmasi [Y] Ya / [N] Tidak: ");

    int c = _getch();
    if (c == 'y' || c == 'Y') {
        akun_delete(idx);
        gotoXY(pop_x + 3, pop_y + 8);
        printf("Berhasil dihapus. Tekan tombol apa saja...");
        _getch();
    }
}

static void view_akun() {
    const int ROWS_PER_PAGE = 10;
    int page = 0;
    int selected = 0;

    const int W_NO = 3;
    const int W_USER = 18;
    const int W_ROLE = 18;
    const int W_STATUS = 10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_w = w - split_x;

        int active_idx[MAX_RECORDS];
        int total = build_active_account_indexes(active_idx, MAX_RECORDS);

        int total_pages = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int rows_on_page = end - start;
        if (rows_on_page < 0) rows_on_page = 0;
        if (rows_on_page == 0) selected = 0;
        if (selected >= rows_on_page) selected = (rows_on_page > 0) ? rows_on_page - 1 : 0;
        if (selected < 0) selected = 0;

        cls();
        draw_layout_base(w, h, "Kelola Akun");

        int table_w = 1 + (W_NO + 3) + (W_USER + 3) + (W_ROLE + 3) + (W_STATUS + 3);
        int table_x = split_x + (content_w - table_w) / 2;
        if (table_x < split_x + 2) table_x = split_x + 2;

        int table_y = 10;

        akun_print_hline(table_x, table_y, W_NO, W_USER, W_ROLE, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s |", W_NO, "No", W_USER, "Username", W_ROLE, "Role", W_STATUS, "Status");
        akun_print_hline(table_x, table_y + 2, W_NO, W_USER, W_ROLE, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            if (r < rows_on_page) {
                int n = start + r;
                int idx = active_idx[n];

                const char *status = g_accounts[idx].active ? "Aktif" : "Nonaktif";

                set_highlight(r == selected);
                gotoXY(table_x, y);
                printf("| %*d | %-*s | %-*s | %-*s |",
                       W_NO, n + 1,
                       W_USER, g_accounts[idx].username,
                       W_ROLE, role_to_label(g_accounts[idx].role),
                       W_STATUS, status);
                set_highlight(0);
            } else {
                gotoXY(table_x, y);
                printf("| %-*s | %-*s | %-*s | %-*s |",
                       W_NO, "", W_USER, "", W_ROLE, "", W_STATUS, "");
            }
        }

        akun_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_USER, W_ROLE, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        gotoXY(table_x, footer_y);
        printf("Total Aktif: %d | Halaman: %d/%d | [<-] Sebelumnya [->] Berikutnya", total, page + 1, total_pages);

        gotoXY(table_x, footer_y + 1);
        printf("[UP/DOWN] Pilih | [LEFT/RIGHT] Halaman | [ENTER] Detail | [E] Edit | [X] Hapus");

        gotoXY(table_x, footer_y + 2);
        printf("[A] Tambah | [0] Kembali | Aksi: pilih data dulu, lalu tekan tombol aksi.");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) {
                if (rows_on_page > 0 && selected > 0) selected--;
                else Beep(800, 60);
                continue;
            }
            if (ext == 80) {
                if (rows_on_page > 0 && selected + 1 < rows_on_page) selected++;
                else Beep(800, 60);
                continue;
            }
            if (ext == 75) {
                if (page > 0) { page--; selected = 0; }
                else Beep(800, 60);
                continue;
            }
            if (ext == 77) {
                if (page + 1 < total_pages) { page++; selected = 0; }
                else Beep(800, 60);
                continue;
            }
        }

        if (ch == 'a' || ch == 'A') {
            akun_popup_add(split_x, content_w);
            continue;
        }

        if (ch == 13) {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = active_idx[start + selected];
            akun_popup_detail(split_x, content_w, &g_accounts[idx]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = active_idx[start + selected];
            akun_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = active_idx[start + selected];
            akun_popup_delete(split_x, content_w, idx);
            continue;
        }
    }
}

/* ================== CRUD DATA (KARYAWAN DATA) ================== */

static int is_digits_only(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) {
        if (*p < '0' || *p > '9') return 0;
    }
    return 1;
}

/* format: DD-MM-YYYY (10 char), contoh: 01-01-2000 */
static int looks_like_date_ddmmyyyy(const char *s) {
    if (!s) return 0;
    if (strlen(s) != 10) return 0;
    if (s[2] != '-' || s[5] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}

/* ---------- PENUMPANG ---------- */

static void penumpang_print_hline(int x, int y,
                                  int w_no, int w_id, int w_nama, int w_email,
                                  int w_telp, int w_lahir, int w_jk, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_id + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_email + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_telp + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_lahir + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_jk + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}

static void penumpang_popup_detail(int split_x, int content_w, const Penumpang *p) {
    int pop_w = 82, pop_h = 16;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 8;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Detail Penumpang");

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID             : %s", p->id);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Nama           : %s", p->nama);
    gotoXY(pop_x + 3, pop_y + 6);  printf("Email          : %s", p->email);
    gotoXY(pop_x + 3, pop_y + 8);  printf("No. Telp       : %s", p->no_telp);
    gotoXY(pop_x + 3, pop_y + 10); printf("Tanggal Lahir  : %s", p->tanggal_lahir);
    gotoXY(pop_x + 3, pop_y + 12); printf("Jenis Kelamin  : %s", p->jenis_kelamin);
    gotoXY(pop_x + 3, pop_y + 14); printf("Status         : %s", p->active ? "Aktif" : "Nonaktif");

    _getch();
}

static void penumpang_popup_add(int split_x, int content_w) {
    int pop_w = 88, pop_h = 20;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 7;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Penumpang");

    char nama[64], email[64], telp[24], lahir[16], jk[8];

    gotoXY(pop_x + 3, pop_y + 2);  printf("Nama (boleh spasi)                 : ");
    gotoXY(pop_x + 3, pop_y + 4);  printf("Email (tanpa spasi)                : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("No. Telp (angka saja)              : ");
    gotoXY(pop_x + 3, pop_y + 8);  printf("Tanggal Lahir (DD-MM-YYYY)         : ");
    gotoXY(pop_x + 3, pop_y + 10); printf("Jenis Kelamin [L/P]                : ");

    gotoXY(pop_x + 43, pop_y + 2);  input_text(nama, 63, 1);  if (nama[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 4);  input_text(email, 63, 0); if (email[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 6);  input_digits(telp, 23);   if (telp[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 8);  input_text(lahir, 15, 0); if (lahir[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 10); input_text(jk, 7, 0);     if (jk[0] == 27) return;

    if (is_blank(nama) || is_blank(email) || is_blank(telp) || is_blank(lahir) || is_blank(jk)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Semua field wajib diisi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!looks_like_email(email)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Email tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!looks_like_date_ddmmyyyy(lahir)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Tanggal lahir harus format DD-MM-YYYY. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!(jk[0] == 'L' || jk[0] == 'l' || jk[0] == 'P' || jk[0] == 'p') || jk[1] != '\0') {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Jenis kelamin harus L atau P. Tekan tombol apa saja...");
        _getch();
        return;
    }

    char id_new[16];
    penumpang_create_auto(id_new, sizeof(id_new), nama, email, telp, lahir, (jk[0] == 'l') ? "L" : (jk[0] == 'p') ? "P" : jk);

    gotoXY(pop_x + 3, pop_y + 14);
    printf("Berhasil tambah. ID: %s. Tekan tombol apa saja...", id_new);
    _getch();
}

static void penumpang_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 92, pop_h = 22;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 6;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Penumpang (kosongkan untuk tetap)");

    char nama[64], email[64], telp[24], lahir[16], jk[8];

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID (tidak bisa diubah)             : %s", g_penumpang[idx].id);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Nama (boleh spasi)                 : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("Email (tanpa spasi)                : ");
    gotoXY(pop_x + 3, pop_y + 8);  printf("No. Telp (angka saja)              : ");
    gotoXY(pop_x + 3, pop_y + 10); printf("Tanggal Lahir (DD-MM-YYYY)         : ");
    gotoXY(pop_x + 3, pop_y + 12); printf("Jenis Kelamin [L/P]                : ");

    gotoXY(pop_x + 43, pop_y + 4);  input_text(nama, 63, 1);  if (nama[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 6);  input_text(email, 63, 0); if (email[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 8);  input_text(telp, 23, 0);  if (telp[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 10); input_text(lahir, 15, 0); if (lahir[0] == 27) return;
    gotoXY(pop_x + 43, pop_y + 12); input_text(jk, 7, 0);     if (jk[0] == 27) return;

    const char *new_nama = is_blank(nama) ? g_penumpang[idx].nama : nama;
    const char *new_email = is_blank(email) ? g_penumpang[idx].email : email;
    const char *new_telp = is_blank(telp) ? g_penumpang[idx].no_telp : telp;
    const char *new_lahir = is_blank(lahir) ? g_penumpang[idx].tanggal_lahir : lahir;

    char jk_final[16];
    snprintf(jk_final, sizeof(jk_final), "%s", g_penumpang[idx].jenis_kelamin);
    if (!is_blank(jk)) {
        if (!(jk[0] == 'L' || jk[0] == 'l' || jk[0] == 'P' || jk[0] == 'p') || jk[1] != '\0') {
            Beep(500, 200);
            gotoXY(pop_x + 3, pop_y + 16);
            printf("Jenis kelamin harus L atau P. Tekan tombol apa saja...");
            _getch();
            return;
        }
        snprintf(jk_final, sizeof(jk_final), "%c", (jk[0] == 'l') ? 'L' : (jk[0] == 'p') ? 'P' : jk[0]);
    }

    if (!looks_like_email(new_email)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 16);
        printf("Email tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!looks_like_date_ddmmyyyy(new_lahir)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 16);
        printf("Tanggal lahir harus format DD-MM-YYYY. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!is_digits_only(new_telp)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 16);
        printf("No. telp harus angka saja. Tekan tombol apa saja...");
        _getch();
        return;
    }

    penumpang_update(idx, new_nama, new_email, new_telp, new_lahir, jk_final);

    gotoXY(pop_x + 3, pop_y + 16);
    printf("Berhasil update. Tekan tombol apa saja...");
    _getch();
}

static void penumpang_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 78, pop_h = 14;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 9;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Penumpang");

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Ubah status penumpang %s (%s) menjadi NONAKTIF?", g_penumpang[idx].id, g_penumpang[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    printf("Konfirmasi [Y] Ya / [N] Tidak: ");

    int c = _getch();
    if (c == 'y' || c == 'Y') {
        penumpang_delete(idx);
        gotoXY(pop_x + 3, pop_y + 8);
        printf("Status berhasil jadi NONAKTIF. Tekan tombol apa saja...");
        _getch();
    }
}

static void view_penumpang() {
    const int ROWS_PER_PAGE = 10;
    int page = 0;
    int selected = 0;

    const int W_NO = 3;
    const int W_ID = 8;
    const int W_NAMA = 18;
    const int W_EMAIL = 22;
    const int W_TELP = 12;
    const int W_LAHIR = 10;
    const int W_JK = 2;
    const int W_STATUS = 10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_w = w - split_x;

        int total = g_penumpangCount;

        int aktif = 0, nonaktif = 0;
        for (int i = 0; i < total; i++) {
            if (g_penumpang[i].active) aktif++; else nonaktif++;
        }

        int total_pages = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int rows_on_page = end - start;
        if (rows_on_page < 0) rows_on_page = 0;
        if (rows_on_page == 0) selected = 0;
        if (selected >= rows_on_page) selected = (rows_on_page > 0) ? rows_on_page - 1 : 0;
        if (selected < 0) selected = 0;

        cls();
        draw_layout_base(w, h, "Kelola Penumpang");

        int table_w = 1 + (W_NO + 3) + (W_ID + 3) + (W_NAMA + 3) + (W_EMAIL + 3) + (W_TELP + 3) + (W_LAHIR + 3) + (W_JK + 3) + (W_STATUS + 3);
        int table_x = split_x + (content_w - table_w) / 2;
        if (table_x < split_x + 2) table_x = split_x + 2;

        int table_y = 10;

        penumpang_print_hline(table_x, table_y, W_NO, W_ID, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_ID, "ID", W_NAMA, "Nama", W_EMAIL, "Email", W_TELP, "Telp", W_LAHIR, "Lahir", W_JK, "JK", W_STATUS, "Status");
        penumpang_print_hline(table_x, table_y + 2, W_NO, W_ID, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            if (r < rows_on_page) {
                int n = start + r;
                const char *status = g_penumpang[n].active ? "Aktif" : "Nonaktif";

                set_highlight(r == selected);
                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*s |",
                       W_NO, n + 1,
                       W_ID, W_ID, g_penumpang[n].id,
                       W_NAMA, W_NAMA, g_penumpang[n].nama,
                       W_EMAIL, W_EMAIL, g_penumpang[n].email,
                       W_TELP, W_TELP, g_penumpang[n].no_telp,
                       W_LAHIR, W_LAHIR, g_penumpang[n].tanggal_lahir,
                       W_JK, W_JK, g_penumpang[n].jenis_kelamin,
                       W_STATUS, status);
                set_highlight(0);
            } else {
                gotoXY(table_x, y);
                printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
                       W_NO, "", W_ID, "", W_NAMA, "", W_EMAIL, "", W_TELP, "", W_LAHIR, "", W_JK, "", W_STATUS, "");
            }
        }

        penumpang_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_ID, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        gotoXY(table_x, footer_y);
        printf("Total: %d (Aktif: %d | Nonaktif: %d) | Halaman: %d/%d | [<-] Sebelumnya [->] Berikutnya",
               total, aktif, nonaktif, page + 1, total_pages);

        gotoXY(table_x, footer_y + 1);
        printf("[UP/DOWN] Pilih | [LEFT/RIGHT] Halaman | [ENTER] Detail | [E] Edit | [X] Nonaktifkan");

        gotoXY(table_x, footer_y + 2);
        printf("[A] Tambah | [0] Kembali | Aksi: pilih data dulu, lalu tekan tombol aksi.");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { if (rows_on_page > 0 && selected > 0) selected--; else Beep(800, 60); continue; }
            if (ext == 80) { if (rows_on_page > 0 && selected + 1 < rows_on_page) selected++; else Beep(800, 60); continue; }
            if (ext == 75) { if (page > 0) { page--; selected = 0; } else Beep(800, 60); continue; }
            if (ext == 77) { if (page + 1 < total_pages) { page++; selected = 0; } else Beep(800, 60); continue; }
        }

        if (ch == 'a' || ch == 'A') { penumpang_popup_add(split_x, content_w); continue; }

        if (ch == 13) {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            penumpang_popup_detail(split_x, content_w, &g_penumpang[start + selected]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = start + selected;
            if (!g_penumpang[idx].active) { Beep(800, 60); continue; }
            penumpang_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = start + selected;
            if (!g_penumpang[idx].active) { Beep(800, 60); continue; }
            penumpang_popup_delete_soft(split_x, content_w, idx);
            continue;
        }
    }
}

/* ---------- STASIUN ---------- */

static void stasiun_print_hline(int x, int y,
                                int w_no, int w_id, int w_kode, int w_nama,
                                int w_kota, int w_mdpl, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_id + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kode + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kota + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_mdpl + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}

static void stasiun_popup_detail(int split_x, int content_w, const Stasiun *s) {
    int pop_w = 92, pop_h = 18;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 8;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Detail Stasiun");

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID      : %s", s->id);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Kode    : %s", s->kode);
    gotoXY(pop_x + 3, pop_y + 6);  printf("Nama    : %s", s->nama);
    gotoXY(pop_x + 3, pop_y + 8);  printf("MDPL    : %d", s->mdpl);
    gotoXY(pop_x + 3, pop_y + 10); printf("Kota    : %s", s->kota);
    gotoXY(pop_x + 3, pop_y + 12); printf("Alamat  : %s", s->alamat);
    gotoXY(pop_x + 3, pop_y + 14); printf("Status  : %s", s->active ? s->status : "Nonaktif");

    _getch();
}

static void stasiun_popup_add(int split_x, int content_w) {
    int pop_w = 96, pop_h = 22;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 6;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Stasiun");

    char kode[16], nama[64], kota[64], alamat[96], mdpl_buf[16], status[20];

    gotoXY(pop_x + 3, pop_y + 2);  printf("Kode Stasiun (tanpa spasi)          : ");
    gotoXY(pop_x + 3, pop_y + 4);  printf("Nama Stasiun (boleh spasi)          : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("MDPL (angka)                        : ");
    gotoXY(pop_x + 3, pop_y + 8);  printf("Kota (boleh spasi)                  : ");
    gotoXY(pop_x + 3, pop_y + 10); printf("Alamat (boleh spasi)                : ");
    gotoXY(pop_x + 3, pop_y + 12); printf("Status (mis. Aktif / Maintenance)   : ");

    gotoXY(pop_x + 47, pop_y + 2);  input_text(kode, 15, 0);    if (kode[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 4);  input_text(nama, 63, 1);    if (nama[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 6);  input_digits(mdpl_buf, 15); if (mdpl_buf[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 8);  input_text(kota, 63, 1);    if (kota[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 10); input_text(alamat, 95, 1);  if (alamat[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 12); input_text(status, 19, 1);  if (status[0] == 27) return;

    if (is_blank(kode) || is_blank(nama) || is_blank(mdpl_buf) || is_blank(kota) || is_blank(alamat)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 16);
        printf("Field wajib: kode, nama, mdpl, kota, alamat. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (contains_space(kode)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 16);
        printf("Kode tidak boleh mengandung spasi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    int mdpl = atoi(mdpl_buf);
    if (mdpl < 0) mdpl = 0;

    if (is_blank(status)) snprintf(status, sizeof(status), "Aktif");

    char id_new[16];
    stasiun_create_auto(id_new, sizeof(id_new), kode, nama, mdpl, kota, alamat, status);

    gotoXY(pop_x + 3, pop_y + 16);
    printf("Berhasil tambah. ID: %s. Tekan tombol apa saja...", id_new);
    _getch();
}

static void stasiun_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 98, pop_h = 24;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 5;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Stasiun (kosongkan untuk tetap)");

    char kode[16], nama[64], kota[64], alamat[96], mdpl_buf[16], status[20];

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID (tidak bisa diubah)              : %s", g_stasiun[idx].id);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Kode Stasiun (tanpa spasi)          : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("Nama Stasiun (boleh spasi)          : ");
    gotoXY(pop_x + 3, pop_y + 8);  printf("MDPL (angka)                        : ");
    gotoXY(pop_x + 3, pop_y + 10); printf("Kota (boleh spasi)                  : ");
    gotoXY(pop_x + 3, pop_y + 12); printf("Alamat (boleh spasi)                : ");
    gotoXY(pop_x + 3, pop_y + 14); printf("Status                               : ");

    gotoXY(pop_x + 47, pop_y + 4);  input_text(kode, 15, 0);   if (kode[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 6);  input_text(nama, 63, 1);   if (nama[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 8);  input_text(mdpl_buf, 15, 0); if (mdpl_buf[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 10); input_text(kota, 63, 1);   if (kota[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 12); input_text(alamat, 95, 1); if (alamat[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 14); input_text(status, 19, 1); if (status[0] == 27) return;

    const char *new_kode = is_blank(kode) ? g_stasiun[idx].kode : kode;
    const char *new_nama = is_blank(nama) ? g_stasiun[idx].nama : nama;
    const char *new_kota = is_blank(kota) ? g_stasiun[idx].kota : kota;
    const char *new_alamat = is_blank(alamat) ? g_stasiun[idx].alamat : alamat;
    const char *new_status = is_blank(status) ? g_stasiun[idx].status : status;

    int mdpl = g_stasiun[idx].mdpl;
    if (!is_blank(mdpl_buf)) {
        if (!is_digits_only(mdpl_buf)) {
            Beep(500, 200);
            gotoXY(pop_x + 3, pop_y + 18);
            printf("MDPL harus angka. Tekan tombol apa saja...");
            _getch();
            return;
        }
        mdpl = atoi(mdpl_buf);
        if (mdpl < 0) mdpl = 0;
    }

    if (contains_space(new_kode)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 18);
        printf("Kode tidak boleh mengandung spasi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    stasiun_update(idx, new_kode, new_nama, mdpl, new_kota, new_alamat, new_status);

    gotoXY(pop_x + 3, pop_y + 18);
    printf("Berhasil update. Tekan tombol apa saja...");
    _getch();
}

static void stasiun_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 80, pop_h = 14;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 9;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Stasiun");

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Ubah status stasiun %s (%s) menjadi NONAKTIF?", g_stasiun[idx].id, g_stasiun[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    printf("Konfirmasi [Y] Ya / [N] Tidak: ");

    int c = _getch();
    if (c == 'y' || c == 'Y') {
        stasiun_delete(idx);
        gotoXY(pop_x + 3, pop_y + 8);
        printf("Status berhasil jadi NONAKTIF. Tekan tombol apa saja...");
        _getch();
    }
}

static void view_stasiun() {
    const int ROWS_PER_PAGE = 10;
    int page = 0;
    int selected = 0;

    const int W_NO = 3;
    const int W_ID = 8;
    const int W_KODE = 8;
    const int W_NAMA = 18;
    const int W_KOTA = 14;
    const int W_MDPL = 6;
    const int W_STATUS = 12;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_w = w - split_x;

        int total = g_stasiunCount;

        int aktif = 0, nonaktif = 0;
        for (int i = 0; i < total; i++) {
            if (g_stasiun[i].active) aktif++; else nonaktif++;
        }

        int total_pages = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int rows_on_page = end - start;
        if (rows_on_page < 0) rows_on_page = 0;
        if (rows_on_page == 0) selected = 0;
        if (selected >= rows_on_page) selected = (rows_on_page > 0) ? rows_on_page - 1 : 0;
        if (selected < 0) selected = 0;

        cls();
        draw_layout_base(w, h, "Kelola Stasiun");

        int table_w = 1 + (W_NO + 3) + (W_ID + 3) + (W_KODE + 3) + (W_NAMA + 3) + (W_KOTA + 3) + (W_MDPL + 3) + (W_STATUS + 3);
        int table_x = split_x + (content_w - table_w) / 2;
        if (table_x < split_x + 2) table_x = split_x + 2;

        int table_y = 10;

        stasiun_print_hline(table_x, table_y, W_NO, W_ID, W_KODE, W_NAMA, W_KOTA, W_MDPL, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_ID, "ID", W_KODE, "Kode", W_NAMA, "Nama", W_KOTA, "Kota", W_MDPL, "MDPL", W_STATUS, "Status");
        stasiun_print_hline(table_x, table_y + 2, W_NO, W_ID, W_KODE, W_NAMA, W_KOTA, W_MDPL, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            if (r < rows_on_page) {
                int n = start + r;
                const char *status = g_stasiun[n].active ? g_stasiun[n].status : "Nonaktif";

                set_highlight(r == selected);
                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %*d | %-*.*s |",
                       W_NO, n + 1,
                       W_ID, W_ID, g_stasiun[n].id,
                       W_KODE, W_KODE, g_stasiun[n].kode,
                       W_NAMA, W_NAMA, g_stasiun[n].nama,
                       W_KOTA, W_KOTA, g_stasiun[n].kota,
                       W_MDPL, g_stasiun[n].mdpl,
                       W_STATUS, W_STATUS, status);
                set_highlight(0);
            } else {
                gotoXY(table_x, y);
                printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
                       W_NO, "", W_ID, "", W_KODE, "", W_NAMA, "", W_KOTA, "", W_MDPL, "", W_STATUS, "");
            }
        }

        stasiun_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_ID, W_KODE, W_NAMA, W_KOTA, W_MDPL, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        gotoXY(table_x, footer_y);
        printf("Total: %d (Aktif: %d | Nonaktif: %d) | Halaman: %d/%d | [<-] Sebelumnya [->] Berikutnya",
               total, aktif, nonaktif, page + 1, total_pages);

        gotoXY(table_x, footer_y + 1);
        printf("[UP/DOWN] Pilih | [LEFT/RIGHT] Halaman | [ENTER] Detail | [E] Edit | [X] Nonaktifkan");

        gotoXY(table_x, footer_y + 2);
        printf("[A] Tambah | [0] Kembali | Aksi: pilih data dulu, lalu tekan tombol aksi.");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { if (rows_on_page > 0 && selected > 0) selected--; else Beep(800, 60); continue; }
            if (ext == 80) { if (rows_on_page > 0 && selected + 1 < rows_on_page) selected++; else Beep(800, 60); continue; }
            if (ext == 75) { if (page > 0) { page--; selected = 0; } else Beep(800, 60); continue; }
            if (ext == 77) { if (page + 1 < total_pages) { page++; selected = 0; } else Beep(800, 60); continue; }
        }

        if (ch == 'a' || ch == 'A') { stasiun_popup_add(split_x, content_w); continue; }

        if (ch == 13) {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            stasiun_popup_detail(split_x, content_w, &g_stasiun[start + selected]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = start + selected;
            if (!g_stasiun[idx].active) { Beep(800, 60); continue; }
            stasiun_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = start + selected;
            if (!g_stasiun[idx].active) { Beep(800, 60); continue; }
            stasiun_popup_delete_soft(split_x, content_w, idx);
            continue;
        }
    }
}

/* ---------- KERETA ---------- */

static void kereta_print_hline(int x, int y,
                               int w_no, int w_id, int w_nama, int w_kelas,
                               int w_kap, int w_gerb, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_id + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kelas + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kap + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_gerb + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}

static void kereta_popup_detail(int split_x, int content_w, const Kereta *k) {
    int pop_w = 90, pop_h = 18;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 8;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Detail Kereta");

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID Kereta   : %s", k->kode);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Nama        : %s", k->nama);
    gotoXY(pop_x + 3, pop_y + 6);  printf("Kelas       : %s", k->kelas);
    gotoXY(pop_x + 3, pop_y + 8);  printf("Kapasitas   : %d", k->kapasitas);
    gotoXY(pop_x + 3, pop_y + 10); printf("Gerbong     : %d", k->gerbong);
    gotoXY(pop_x + 3, pop_y + 12); printf("Status      : %s", k->active ? k->status : "Nonaktif");
    gotoXY(pop_x + 3, pop_y + 14); printf("Aktif?      : %s", k->active ? "Ya" : "Tidak");

    _getch();
}

static void kereta_popup_add(int split_x, int content_w) {
    int pop_w = 94, pop_h = 22;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 6;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Kereta");

    char nama[64], kelas[32], kap_buf[16], gerb_buf[16], status[20];

    gotoXY(pop_x + 3, pop_y + 2);  printf("Nama Kereta (boleh spasi)           : ");
    gotoXY(pop_x + 3, pop_y + 4);  printf("Kelas Kereta (boleh spasi)          : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("Kapasitas (angka)                   : ");
    gotoXY(pop_x + 3, pop_y + 8);  printf("Jumlah Gerbong (angka)              : ");
    gotoXY(pop_x + 3, pop_y + 10); printf("Status (mis. Aktif / Maintenance)   : ");

    gotoXY(pop_x + 47, pop_y + 2);  input_text(nama, 63, 1);     if (nama[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 4);  input_text(kelas, 31, 1);    if (kelas[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 6);  input_digits(kap_buf, 15);   if (kap_buf[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 8);  input_digits(gerb_buf, 15);  if (gerb_buf[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 10); input_text(status, 19, 1);   if (status[0] == 27) return;

    if (is_blank(nama) || is_blank(kelas) || is_blank(kap_buf) || is_blank(gerb_buf)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Field wajib: nama, kelas, kapasitas, gerbong. Tekan tombol apa saja...");
        _getch();
        return;
    }

    int kap = atoi(kap_buf);
    int gerb = atoi(gerb_buf);
    if (kap <= 0 || gerb <= 0) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 14);
        printf("Kapasitas & gerbong harus > 0. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (is_blank(status)) snprintf(status, sizeof(status), "Aktif");

    char id_new[16];
    kereta_create_auto(id_new, sizeof(id_new), nama, kelas, kap, gerb, status);

    gotoXY(pop_x + 3, pop_y + 14);
    printf("Berhasil tambah. ID: %s. Tekan tombol apa saja...", id_new);
    _getch();
}

static void kereta_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 96, pop_h = 24;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 5;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Kereta (kosongkan untuk tetap)");

    char nama[64], kelas[32], kap_buf[16], gerb_buf[16], status[20];

    gotoXY(pop_x + 3, pop_y + 2);  printf("ID Kereta (tidak bisa diubah)       : %s", g_kereta[idx].kode);
    gotoXY(pop_x + 3, pop_y + 4);  printf("Nama Kereta (boleh spasi)           : ");
    gotoXY(pop_x + 3, pop_y + 6);  printf("Kelas Kereta (boleh spasi)          : ");
    gotoXY(pop_x + 3, pop_y + 8);  printf("Kapasitas (angka)                   : ");
    gotoXY(pop_x + 3, pop_y + 10); printf("Jumlah Gerbong (angka)              : ");
    gotoXY(pop_x + 3, pop_y + 12); printf("Status                               : ");

    gotoXY(pop_x + 47, pop_y + 4);  input_text(nama, 63, 1);      if (nama[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 6);  input_text(kelas, 31, 1);     if (kelas[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 8);  input_text(kap_buf, 15, 0);   if (kap_buf[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 10); input_text(gerb_buf, 15, 0);  if (gerb_buf[0] == 27) return;
    gotoXY(pop_x + 47, pop_y + 12); input_text(status, 19, 1);    if (status[0] == 27) return;

    const char *new_nama = is_blank(nama) ? g_kereta[idx].nama : nama;
    const char *new_kelas = is_blank(kelas) ? g_kereta[idx].kelas : kelas;
    const char *new_status = is_blank(status) ? g_kereta[idx].status : status;

    int kap = g_kereta[idx].kapasitas;
    if (!is_blank(kap_buf)) {
        if (!is_digits_only(kap_buf)) {
            Beep(500, 200);
            gotoXY(pop_x + 3, pop_y + 16);
            printf("Kapasitas harus angka. Tekan tombol apa saja...");
            _getch();
            return;
        }
        kap = atoi(kap_buf);
    }

    int gerb = g_kereta[idx].gerbong;
    if (!is_blank(gerb_buf)) {
        if (!is_digits_only(gerb_buf)) {
            Beep(500, 200);
            gotoXY(pop_x + 3, pop_y + 16);
            printf("Gerbong harus angka. Tekan tombol apa saja...");
            _getch();
            return;
        }
        gerb = atoi(gerb_buf);
    }

    if (kap <= 0 || gerb <= 0) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 16);
        printf("Kapasitas & gerbong harus > 0. Tekan tombol apa saja...");
        _getch();
        return;
    }

    kereta_update(idx, new_nama, new_kelas, kap, gerb, new_status);

    gotoXY(pop_x + 3, pop_y + 16);
    printf("Berhasil update. Tekan tombol apa saja...");
    _getch();
}

static void kereta_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 80, pop_h = 14;
    int pop_x = split_x + (content_w - pop_w) / 2;
    int pop_y = 9;
    if (pop_x < split_x + 2) pop_x = split_x + 2;

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Kereta");

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Ubah status kereta %s (%s) menjadi NONAKTIF?", g_kereta[idx].kode, g_kereta[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    printf("Konfirmasi [Y] Ya / [N] Tidak: ");

    int c = _getch();
    if (c == 'y' || c == 'Y') {
        kereta_delete(idx);
        gotoXY(pop_x + 3, pop_y + 8);
        printf("Status berhasil jadi NONAKTIF. Tekan tombol apa saja...");
        _getch();
    }
}

static void view_kereta() {
    const int ROWS_PER_PAGE = 10;
    int page = 0;
    int selected = 0;

    const int W_NO = 3;
    const int W_ID = 8;
    const int W_NAMA = 18;
    const int W_KELAS = 12;
    const int W_KAP = 9;
    const int W_GERB = 7;
    const int W_STATUS = 12;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_w = w - split_x;

        int total = g_keretaCount;

        int aktif = 0, nonaktif = 0;
        for (int i = 0; i < total; i++) {
            if (g_kereta[i].active) aktif++; else nonaktif++;
        }

        int total_pages = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int rows_on_page = end - start;
        if (rows_on_page < 0) rows_on_page = 0;
        if (rows_on_page == 0) selected = 0;
        if (selected >= rows_on_page) selected = (rows_on_page > 0) ? rows_on_page - 1 : 0;
        if (selected < 0) selected = 0;

        cls();
        draw_layout_base(w, h, "Kelola Kereta");

        int table_w = 1 + (W_NO + 3) + (W_ID + 3) + (W_NAMA + 3) + (W_KELAS + 3) + (W_KAP + 3) + (W_GERB + 3) + (W_STATUS + 3);
        int table_x = split_x + (content_w - table_w) / 2;
        if (table_x < split_x + 2) table_x = split_x + 2;

        int table_y = 10;

        kereta_print_hline(table_x, table_y, W_NO, W_ID, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_ID, "ID", W_NAMA, "Nama", W_KELAS, "Kelas", W_KAP, "Kapasitas", W_GERB, "Gerbong", W_STATUS, "Status");
        kereta_print_hline(table_x, table_y + 2, W_NO, W_ID, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            if (r < rows_on_page) {
                int n = start + r;
                const char *status = g_kereta[n].active ? g_kereta[n].status : "Nonaktif";

                set_highlight(r == selected);
                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %*d | %*d | %-*.*s |",
                       W_NO, n + 1,
                       W_ID, W_ID, g_kereta[n].kode,
                       W_NAMA, W_NAMA, g_kereta[n].nama,
                       W_KELAS, W_KELAS, g_kereta[n].kelas,
                       W_KAP, g_kereta[n].kapasitas,
                       W_GERB, g_kereta[n].gerbong,
                       W_STATUS, W_STATUS, status);
                set_highlight(0);
            } else {
                gotoXY(table_x, y);
                printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
                       W_NO, "", W_ID, "", W_NAMA, "", W_KELAS, "", W_KAP, "", W_GERB, "", W_STATUS, "");
            }
        }

        kereta_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_ID, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        gotoXY(table_x, footer_y);
        printf("Total: %d (Aktif: %d | Nonaktif: %d) | Halaman: %d/%d | [<-] Sebelumnya [->] Berikutnya",
               total, aktif, nonaktif, page + 1, total_pages);

        gotoXY(table_x, footer_y + 1);
        printf("[UP/DOWN] Pilih | [LEFT/RIGHT] Halaman | [ENTER] Detail | [E] Edit | [X] Nonaktifkan");

        gotoXY(table_x, footer_y + 2);
        printf("[A] Tambah | [0] Kembali | Aksi: pilih data dulu, lalu tekan tombol aksi.");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { if (rows_on_page > 0 && selected > 0) selected--; else Beep(800, 60); continue; }
            if (ext == 80) { if (rows_on_page > 0 && selected + 1 < rows_on_page) selected++; else Beep(800, 60); continue; }
            if (ext == 75) { if (page > 0) { page--; selected = 0; } else Beep(800, 60); continue; }
            if (ext == 77) { if (page + 1 < total_pages) { page++; selected = 0; } else Beep(800, 60); continue; }
        }

        if (ch == 'a' || ch == 'A') { kereta_popup_add(split_x, content_w); continue; }

        if (ch == 13) {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            kereta_popup_detail(split_x, content_w, &g_kereta[start + selected]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = start + selected;
            if (!g_kereta[idx].active) { Beep(800, 60); continue; }
            kereta_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = start + selected;
            if (!g_kereta[idx].active) { Beep(800, 60); continue; }
            kereta_popup_delete_soft(split_x, content_w, idx);
            continue;
        }
    }
}


static const char* role_to_label(Role role) {
    switch (role) {
        case ROLE_SUPERADMIN: return "Superadmin";
        case ROLE_DATA:       return "Karyawan Data";
        case ROLE_TRANSAKSI:  return "Karyawan Transaksi";
        case ROLE_MANAGER:    return "Manager";
        default:              return "Unknown";
    }
}

static int auth_find_account_index(const char* username, const char* password) {
    if (!username || !password) return -1;
    for (int i = 0; i < g_accountCount; i++) {
        if (!g_accounts[i].active) continue;
        if (strcmp(g_accounts[i].username, username) == 0 &&
            strcmp(g_accounts[i].password, password) == 0) {
            return i;
        }
    }
    return -1;
}

/* ================== DASHBOARD MENU UTAMA (ROLE-BASED) ================== */
static void dashboard_main(int account_index) {
    if (account_index < 0 || account_index >= g_accountCount) return;
    Account *me = &g_accounts[account_index];

    while(1) {
        int w = get_screen_width(); if(w<=0) w=120;
        int h = get_screen_height(); if(h<=0) h=30;

        cls();
        draw_layout_base(w, h, "Dashboard");
        /* Override sidebar role label */
        gotoXY(5, 5); printf("Role: %-20s", role_to_label(me->role));

        int split_x = w / 4;
        int content_w = w - split_x;
        int center_x = split_x + (content_w / 2);
        int center_y = h / 2;

        char welcome[100];
        snprintf(welcome, 100, "Selamat Datang, %s!", me->username);
        gotoXY(center_x - (int)(strlen(welcome)/2), center_y - 8);
        printf("%s", welcome);

        int box_w = 40;
        int box_x = center_x - (box_w / 2);
        int start_y = center_y - 4;

        /* Menu disesuaikan dengan role */
        int menu_count = 0;
        const char* menus[8];

        if (me->role == ROLE_SUPERADMIN) {
            menus[menu_count++] = "[1] Kelola Akun Karyawan";
            menus[menu_count++] = "[2] Kelola Data Karyawan";
            menus[menu_count++] = "[0] Logout / Keluar";
        } else if (me->role == ROLE_DATA) {
            menus[menu_count++] = "[1] Kelola Penumpang";
            menus[menu_count++] = "[2] Kelola Stasiun";
            menus[menu_count++] = "[3] Kelola Kereta";
            menus[menu_count++] = "[0] Logout / Keluar";
        } else if (me->role == ROLE_TRANSAKSI) {
            menus[menu_count++] = "[1] Transaksi Pembayaran Tiket";
            menus[menu_count++] = "[2] Kelola Jadwal";
            menus[menu_count++] = "[0] Logout / Keluar";
        } else if (me->role == ROLE_MANAGER) {
            menus[menu_count++] = "[1] Laporan";
            menus[menu_count++] = "[0] Logout / Keluar";
        } else {
            menus[menu_count++] = "[0] Logout / Keluar";
        }

        for(int i=0; i<menu_count; i++) {
            gotoXY(box_x, start_y + (i*2));
            printf("%s", menus[i]);
        }

        gotoXY(center_x - 12, start_y + (menu_count*2) + 2);
        printf("Pilih Menu : ");

        int ch = _getch();
        if (ch == '0') return;

        /* Aksi berdasarkan role */
        if (me->role == ROLE_SUPERADMIN) {
            if (ch == '2') {
                view_karyawan();
            } else if (ch == '1') {
                gotoXY(center_x - 20, start_y + (menu_count*2) + 4);
                view_akun();
            }
        } else if (me->role == ROLE_DATA) {
            if (ch == '1') {
                view_penumpang();
            } else if (ch == '2') {
                view_stasiun();
            } else if (ch == '3') {
                view_kereta();
            }
        } else if (me->role == ROLE_TRANSAKSI) {
            if (ch == '1' || ch == '2') {
                gotoXY(center_x - 26, start_y + (menu_count*2) + 4);
                printf(">> Menu Transaksi: CRUD belum dibuat (placeholder)");
                Sleep(900);
            }
        } else if (me->role == ROLE_MANAGER) {
            if (ch == '1') {
                gotoXY(center_x - 22, start_y + (menu_count*2) + 4);
                printf(">> Menu Laporan: CRUD belum dibuat (placeholder)");
                Sleep(900);
            }
        }
    }
}

/* ================== LOGIN SCREEN ================== */
void login_screen() {
    char user[64], pass[64];
    int w, h;

    while (1) {
        w = get_screen_width(); if(w<=0) w=120;
        h = get_screen_height(); if(h<=0) h=30;

        cls();
        draw_layout_base(w, h, "Login System");

        int split_x = w / 4;
        int center_x = split_x + ((w - split_x) / 2);
        int form_top = (h/2) - 5;

        int box_w = 50;
        int box_x = center_x - (box_w / 2);

        gotoXY(box_x, form_top); printf("Username :");
        gotoXY(box_x, form_top + 1); for(int i=0; i<box_w; i++) printf("-");

        gotoXY(box_x, form_top + 4); printf("Password :");
        gotoXY(box_x, form_top + 5); for(int i=0; i<box_w; i++) printf("-");

        gotoXY(split_x + 5, h - 4);
        printf("[ESC] Keluar Program  |  [TAB] Lihat/Sembunyikan Password  |  [ENTER] Login");

        gotoXY(box_x, form_top + 2);
        CONSOLE_CURSOR_INFO ci;
        GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
        ci.bVisible = TRUE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        input_text(user, 63, 0);            // username: tanpa spasi
        if (user[0] == 27) { cls(); exit(0); }

        gotoXY(box_x, form_top + 6);
        input_password_masked(pass, 63, box_x, form_top + 6, box_w);
        if (pass[0] == 27) { cls(); exit(0); }

        ci.bVisible = FALSE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        int acc_idx = auth_find_account_index(user, pass);
        if (acc_idx >= 0) {
            gotoXY(center_x - 10, form_top + 9); printf(">> LOGIN BERHASIL! <<");
            Sleep(800);
            dashboard_main(acc_idx);
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
    penumpang_init();
    stasiun_init();
    kereta_init();
}

void ui_run() {
    login_screen();
}
