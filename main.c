#include <windows.h>
#include <stdio.h>
#include "ui_app.h"

/* Fungsi Sakti: Paksa Fullscreen (tanpa border) */
void set_fullscreen_mode() {
    HWND   hwnd = GetConsoleWindow();
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // 1. Cari ukuran maksimal window console (dalam kolom x baris)
    COORD largest = GetLargestConsoleWindowSize(hOut);
    if (largest.X == 0 || largest.Y == 0) {
        // fallback kalau gagal baca
        largest.X = 160;
        largest.Y = 50;
    }

    // 2. Set buffer console ke ukuran maksimal
    SetConsoleScreenBufferSize(hOut, largest);

    // 3. Set window console pakai seluruh buffer
    SMALL_RECT rect;
    rect.Left   = 0;
    rect.Top    = 0;
    rect.Right  = largest.X - 1;
    rect.Bottom = largest.Y - 1;
    SetConsoleWindowInfo(hOut, TRUE, &rect);

    // 4. Ubah style jendela: hapus title bar, border, tombol X, dll
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME |
               WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, style);

    // 5. Maksimalkan jendela ke layar
    ShowWindow(hwnd, SW_MAXIMIZE);
    SetForegroundWindow(hwnd);

    // 6. (Opsional) sembunyikan kursor mouse biar rapi
    ShowCursor(FALSE);
}

int main(void) {
    // Judul Program
    SetConsoleTitle("FOURTRAIN SYSTEM");

    // Warna: Background Hitam (0), Teks Aqua/Biru Muda (B)
    system("color 0B");

    // AKTIFKAN FULLSCREEN
    set_fullscreen_mode();

    // Kasih waktu Windows selesai resize sebelum gambar UI
    Sleep(500);

    // Masuk ke Aplikasi
    ui_init();
    ui_run();

    return 0;
}
