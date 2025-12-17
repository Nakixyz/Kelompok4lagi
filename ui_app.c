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
    if (contains_space(s)) return 0;
    const char *at = strchr(s, '@');
    if (!at || at == s) return 0;
    const char *dot = strrchr(s, '.');
    if (!dot || dot < at) return 0;
    if (*(dot + 1) == '\0') return 0;
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
void view_karyawan() {
    const int ROWS_PER_PAGE = 15;
    int page = 0;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = w / 4;
        int content_w = w - split_x;

        int active_idx[MAX_RECORDS];
        int total = build_active_karyawan_indexes(active_idx, MAX_RECORDS);

        int total_pages = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        cls();
        draw_layout_base(w, h, "Kelola Karyawan");

        int top = 10;

        // Header tabel
        gotoXY(split_x + 4, top);
        printf("%-4s %-10s %-25s %-22s %-15s %-10s", "No", "ID", "Nama", "Email", "Jabatan", "Status");
        gotoXY(split_x + 4, top + 1);
        for (int i = 0; i < w - split_x - 10; i++) putchar('-');

        // Paging data
        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int row = top + 2;
        for (int n = start; n < end; n++) {
            int i = active_idx[n];
            gotoXY(split_x + 4, row++);
            printf("%-4d %-10s %-25s %-22s %-15s %-10s",
                   n + 1,
                   g_karyawan[i].id,
                   g_karyawan[i].nama,
                   g_karyawan[i].email,
                   g_karyawan[i].jabatan,
                   "Aktif");
        }

        // Footer
        gotoXY(split_x + 4, h - 8);
        printf("Total aktif: %d | Halaman: %d/%d", total, page + 1, total_pages);

        gotoXY(split_x + 4,  h - 6); printf("[1] Tambah Data");
        gotoXY(split_x + 25, h - 6); printf("[2] Hapus Data");
        gotoXY(split_x + 45, h - 6); printf("[3] Edit Data");

        gotoXY(split_x + 4,  h - 4); printf("[<=] Prev (Left)   [=>] Next (Right)  |  [0] Kembali");


        int ch = _getch();
        if (ch == '0') return;

        // Tombol panah di _getch() biasanya datang sebagai: 0 atau 224 lalu kode kedua
        if (ch == 0 || ch == 224) {
            int ext = _getch();
            // Left Arrow = 75, Right Arrow = 77 (umum di conio)
            if (ext == 75) { // LEFT
                if (page > 0) page--;
                else Beep(800, 60);
                continue;
            }
            if (ext == 77) { // RIGHT
                if (page + 1 < total_pages) page++;
                else Beep(800, 60);
                continue;
            }
        }

        // Tetap dukung tombol huruf
        if (ch == 'n' || ch == 'N') {
            if (page + 1 < total_pages) page++;
            else Beep(800, 60);
            continue;
        }
        if (ch == 'p' || ch == 'P') {
            if (page > 0) page--;
            else Beep(800, 60);
            continue;
        }

        /* ===== CREATE ===== */
        if (ch == '1') {
            int pop_w = 72, pop_h = 16;
            int pop_x = split_x + (content_w - pop_w) / 2;
            int pop_y = 9;
            if (pop_x < split_x + 2) pop_x = split_x + 2;

            draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Karyawan");

            char new_id[16];
            char nama[64], email[64], jabatan[64];

            gotoXY(pop_x + 3, pop_y + 2);  printf("ID Otomatis          : (dibuat sistem)");
            gotoXY(pop_x + 3, pop_y + 4);  printf("Nama                 : ");
            gotoXY(pop_x + 3, pop_y + 6);  printf("Email (tanpa spasi)  : ");
            gotoXY(pop_x + 3, pop_y + 8);  printf("Jabatan              : ");

            gotoXY(pop_x + 27, pop_y + 4); input_text(nama, 63, 1);
            if (nama[0] == 27) continue;

            gotoXY(pop_x + 27, pop_y + 6); input_text(email, 63, 0);
            if (email[0] == 27) continue;

            gotoXY(pop_x + 27, pop_y + 8); input_text(jabatan, 63, 1);
            if (jabatan[0] == 27) continue;

            if (is_blank(nama) || is_blank(email) || is_blank(jabatan)) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 12);
                printf("Semua field wajib diisi. Tekan tombol apa saja...");
                _getch();
                continue;
            }
            if (!looks_like_email(email)) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 12);
                printf("Format email tidak valid. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            // CREATE AUTO ID
            karyawan_create_auto(new_id, sizeof(new_id), nama, email, jabatan);

            gotoXY(pop_x + 3, pop_y + 12);
            printf("Berhasil menambah karyawan dengan ID: %s. Tekan tombol apa saja...", new_id);
            _getch();
            continue;


            gotoXY(pop_x + 3, pop_y + 12);
            printf("Berhasil menambah karyawan. Tekan tombol apa saja...");
            _getch();
            continue;
        }

        /* ===== UPDATE ===== */
        if (ch == '3') {
            if (total == 0) { Beep(800, 80); continue; }

            int pop_w = 72, pop_h = 18;
            int pop_x = split_x + (content_w - pop_w) / 2;
            int pop_y = 8;
            if (pop_x < split_x + 2) pop_x = split_x + 2;

            draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Karyawan");

            char no_buf[8];
            gotoXY(pop_x + 3, pop_y + 2);
            printf("Masukkan No (lihat kolom No): ");
            gotoXY(pop_x + 33, pop_y + 2);
            input_digits(no_buf, 7);
            if (no_buf[0] == 27) continue;

            int no = atoi(no_buf);
            if (no < 1 || no > total) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 4);
                printf("No tidak valid. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            int idx = active_idx[no - 1];

            gotoXY(pop_x + 3, pop_y + 4);
            printf("ID: %s", g_karyawan[idx].id);

            gotoXY(pop_x + 3, pop_y + 6);  printf("Nama baru (Enter=tetap)    : ");
            gotoXY(pop_x + 3, pop_y + 8);  printf("Email baru (Enter=tetap)   : ");
            gotoXY(pop_x + 3, pop_y + 10); printf("Jabatan baru (Enter=tetap) : ");

            char nama[64], email[64], jabatan[64];

            gotoXY(pop_x + 34, pop_y + 6);  input_text(nama, 63, 1);
            if (nama[0] == 27) continue;

            gotoXY(pop_x + 34, pop_y + 8);  input_text(email, 63, 0);
            if (email[0] == 27) continue;

            gotoXY(pop_x + 34, pop_y + 10); input_text(jabatan, 63, 1);
            if (jabatan[0] == 27) continue;

            const char *new_nama   = is_blank(nama)   ? g_karyawan[idx].nama   : nama;
            const char *new_email  = is_blank(email)  ? g_karyawan[idx].email  : email;
            const char *new_jabatan= is_blank(jabatan)? g_karyawan[idx].jabatan: jabatan;

            if (!looks_like_email(new_email)) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 13);
                printf("Format email tidak valid. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            karyawan_update(idx, new_nama, new_email, new_jabatan);

            gotoXY(pop_x + 3, pop_y + 13);
            printf("Data berhasil diupdate. Tekan tombol apa saja...");
            _getch();
            continue;
        }

        /* ===== DELETE ===== */
        if (ch == '2') {
            if (total == 0) { Beep(800, 80); continue; }

            int pop_w = 72, pop_h = 14;
            int pop_x = split_x + (content_w - pop_w) / 2;
            int pop_y = 10;
            if (pop_x < split_x + 2) pop_x = split_x + 2;

            draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Hapus Karyawan");

            char no_buf[8];
            gotoXY(pop_x + 3, pop_y + 2);
            printf("Masukkan No (lihat kolom No): ");
            gotoXY(pop_x + 33, pop_y + 2);
            input_digits(no_buf, 7);
            if (no_buf[0] == 27) continue;

            int no = atoi(no_buf);
            if (no < 1 || no > total) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 4);
                printf("No tidak valid. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            int idx = active_idx[no - 1];

            gotoXY(pop_x + 3, pop_y + 4);
            printf("Hapus: %s - %s ?", g_karyawan[idx].id, g_karyawan[idx].nama);

            gotoXY(pop_x + 3, pop_y + 6);
            printf("Konfirmasi [Y] Ya / [N] Tidak: ");

            int c = _getch();
            if (c == 'y' || c == 'Y') {
                karyawan_delete(idx);
                gotoXY(pop_x + 3, pop_y + 8);
                printf("Berhasil dihapus. Tekan tombol apa saja...");
                _getch();
            }
            continue;
        }
    }
}


/* ================== CRUD AKUN (SUPERADMIN) ================== */
static void view_akun() {
    const int ROWS_PER_PAGE = 15;
    int page = 0;

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

        cls();
        draw_layout_base(w, h, "Kelola Akun");

        int top = 10;

        gotoXY(split_x + 4, top);
        printf("%-4s %-20s %-22s %-10s", "No", "Username", "Role", "Status");
        gotoXY(split_x + 4, top + 1);
        for (int i = 0; i < w - split_x - 10; i++) putchar('-');

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > total) end = total;

        int row = top + 2;
        for (int n = start; n < end; n++) {
            int i = active_idx[n];
            const char *status = g_accounts[i].active ? "Aktif" : "Nonaktif";
            gotoXY(split_x + 4, row++);
            printf("%-4d %-20s %-22s %-10s",
                   n + 1,
                   g_accounts[i].username,
                   role_to_label(g_accounts[i].role),
                   status);
        }

        gotoXY(split_x + 4, h - 8);
        printf("Total aktif: %d | Halaman: %d/%d", total, page + 1, total_pages);

        gotoXY(split_x + 4,  h - 6); printf("[1] Tambah Akun");
        gotoXY(split_x + 25, h - 6); printf("[2] Hapus Akun");
        gotoXY(split_x + 45, h - 6); printf("[3] Edit Akun");

        gotoXY(split_x + 4,  h - 4); printf("[<=] Prev (Left)   [=>] Next (Right)  |  [0] Kembali");

        int ch = _getch();
        if (ch == '0') return;

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 75) { if (page > 0) page--; else Beep(800, 60); continue; }   /* LEFT */
            if (ext == 77) { if (page + 1 < total_pages) page++; else Beep(800, 60); continue; } /* RIGHT */
        }

        if (ch == 'n' || ch == 'N') { if (page + 1 < total_pages) page++; else Beep(800, 60); continue; }
        if (ch == 'p' || ch == 'P') { if (page > 0) page--; else Beep(800, 60); continue; }

        /* ===== CREATE ===== */
        if (ch == '1') {
            int pop_w = 72, pop_h = 18;
            int pop_x = split_x + (content_w - pop_w) / 2;
            int pop_y = 8;
            if (pop_x < split_x + 2) pop_x = split_x + 2;

            draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Akun Karyawan");

            char username[64], password[64], rolebuf[8];

            gotoXY(pop_x + 3, pop_y + 2);  printf("Username (tanpa spasi)     : ");
            gotoXY(pop_x + 3, pop_y + 4);  printf("Password (tanpa spasi)     : ");
            gotoXY(pop_x + 3, pop_y + 6);  printf("Role [1=Data 2=Transaksi 3=Manager] : ");

            gotoXY(pop_x + 34, pop_y + 2);
            input_text(username, 63, 0);
            if (username[0] == 27) continue;

            gotoXY(pop_x + 34, pop_y + 4);
            input_password_masked(password, 63, pop_x + 34, pop_y + 4, 30);
            if (password[0] == 27) continue;

            gotoXY(pop_x + 48, pop_y + 6);
            input_text(rolebuf, 7, 0);
            if (rolebuf[0] == 27) continue;

            if (is_blank(username) || is_blank(password) || is_blank(rolebuf)) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 12);
                printf("Semua field wajib diisi. Tekan tombol apa saja...");
                _getch();
                continue;
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
                continue;
            }

            int ok = akun_create(username, password, role);
            gotoXY(pop_x + 3, pop_y + 12);
            if (ok) {
                printf("Berhasil menambah akun. Tekan tombol apa saja...");
            } else {
                printf("Gagal menambah akun (duplikat / input tidak valid). Tekan tombol apa saja...");
            }
            _getch();
            continue;
        }

        /* ===== UPDATE ===== */
        if (ch == '3') {
            if (total == 0) { Beep(800, 80); continue; }

            int pop_w = 78, pop_h = 22;
            int pop_x = split_x + (content_w - pop_w) / 2;
            int pop_y = 6;
            if (pop_x < split_x + 2) pop_x = split_x + 2;

            draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Akun");

            char no_buf[8];
            gotoXY(pop_x + 3, pop_y + 2);
            printf("Masukkan No (lihat kolom No): ");
            gotoXY(pop_x + 33, pop_y + 2);
            input_digits(no_buf, 7);
            if (no_buf[0] == 27) continue;

            int no = atoi(no_buf);
            if (no < 1 || no > total) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 4);
                printf("No tidak valid. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            int idx = active_idx[no - 1];
            int is_admin = (strcmp(g_accounts[idx].username, "admin") == 0);

            gotoXY(pop_x + 3, pop_y + 4);
            printf("Username: %s", g_accounts[idx].username);
            gotoXY(pop_x + 3, pop_y + 5);
            printf("Role    : %s", role_to_label(g_accounts[idx].role));

            char new_user[64] = "";
            char new_pass[64] = "";
            char rolebuf[8] = "";

            int line = pop_y + 7;

            if (!is_admin) {
                gotoXY(pop_x + 3, line);     printf("Username baru (Enter=tetap)                  : ");
                gotoXY(pop_x + 55, line);    input_text(new_user, 63, 0);
                if (new_user[0] == 27) continue;
                line += 2;
            } else {
                gotoXY(pop_x + 3, line); printf("Catatan: akun admin hanya bisa ganti password.");
                line += 2;
            }

            gotoXY(pop_x + 3, line);         printf("Password baru (Enter=tetap, tanpa spasi)     : ");
            gotoXY(pop_x + 55, line);        input_password_masked(new_pass, 63, pop_x + 55, line, 20);
            if (new_pass[0] == 27) continue;
            line += 2;

            if (!is_admin) {
                gotoXY(pop_x + 3, line);
                printf("Role baru [1=Data 2=Transaksi 3=Manager] (Enter=tetap): ");
                gotoXY(pop_x + 68, line);
                input_text(rolebuf, 7, 0);
                if (rolebuf[0] == 27) continue;
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
            if (!any_change) {
                printf("Tidak ada perubahan. Tekan tombol apa saja...");
            } else if (ok_all) {
                printf("Berhasil update akun. Tekan tombol apa saja...");
            } else {
                printf("Sebagian/perubahan gagal (input tidak valid/duplikat/role salah). Tekan tombol apa saja...");
            }
            _getch();
            continue;
        }

        /* ===== DELETE ===== */
        if (ch == '2') {
            if (total == 0) { Beep(800, 80); continue; }

            int pop_w = 72, pop_h = 14;
            int pop_x = split_x + (content_w - pop_w) / 2;
            int pop_y = 10;
            if (pop_x < split_x + 2) pop_x = split_x + 2;

            draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Hapus Akun");

            char no_buf[8];
            gotoXY(pop_x + 3, pop_y + 2);
            printf("Masukkan No (lihat kolom No): ");
            gotoXY(pop_x + 33, pop_y + 2);
            input_digits(no_buf, 7);
            if (no_buf[0] == 27) continue;

            int no = atoi(no_buf);
            if (no < 1 || no > total) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 4);
                printf("No tidak valid. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            int idx = active_idx[no - 1];
            if (strcmp(g_accounts[idx].username, "admin") == 0) {
                Beep(500, 200);
                gotoXY(pop_x + 3, pop_y + 4);
                printf("Akun admin tidak bisa dihapus. Tekan tombol apa saja...");
                _getch();
                continue;
            }

            gotoXY(pop_x + 3, pop_y + 4);
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
            if (ch == '2' || ch == '3' || ch == '1') {
                gotoXY(center_x - 22, start_y + (menu_count*2) + 4);
                printf(">> Menu Data: CRUD belum diintegrasikan (placeholder)");
                Sleep(900);
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
    stasiun_init();
    kereta_init();
}

void ui_run() {
    login_screen();
}
