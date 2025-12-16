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
        else if (ch == '2') {
            view_karyawan(); // Masuk menu karyawan
        }
        else if (ch == '4') {
            view_kereta();
        }
        else if (ch == '1' || ch == '3') {
            gotoXY(center_x - 15, start_y + 14); printf(">> Membuka Menu. (Placeholder)");
            Sleep(500);
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