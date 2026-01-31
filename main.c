#include <windows.h>
#include <stdio.h>
#include "ui_app.h"

/* Fungsi Sakti: Paksa Fullscreen Tanpa Border */
void set_fullscreen_mode() {
    // 1. Ambil Handle ke Console & Window
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HWND hwnd = GetConsoleWindow();

    // 2. Fullscreen "Hack" (Alt+Enter) - Coba paksa mode native fullscreen dulu
    //    Ini hardware-level fullscreen. Kalau berhasil, return.
    COORD newSize;
    if (SetConsoleDisplayMode(hOut, CONSOLE_FULLSCREEN_MODE, &newSize)) {
        return;
    }

    // 3. Fallback: Borderless Window Fullscreen (Fake Fullscreen)


    // a. Hapus semua dekorasi jendela (Border, Title Bar, Scrollbar)
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, style);

    // b. Ambil ukuran layar monitor penuh
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    // c. Paksa jendela console menuhin layar
    SetWindowPos(hwnd, HWND_TOP, 0, 0, screen_w, screen_h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // d. Sesuaikan ukuran BUFFER console agar pas dengan ukuran jendela baru
    //    Supaya tidak ada scrollbar dan tulisan bisa sampai pojok

    //    Ambil info font saat ini
    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    GetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);

    //    Hitung berapa kolom & baris yang muat di layar
    //    (Lebar Layar / Lebar Font), (Tinggi Layar / Tinggi Font)
    SHORT cols = (SHORT)(screen_w / fontInfo.dwFontSize.X);
    SHORT rows = (SHORT)(screen_h / fontInfo.dwFontSize.Y);

    //    Set ukuran buffer (area logic)
    COORD bufferSize;
    bufferSize.X = cols;
    bufferSize.Y = rows;
    SetConsoleScreenBufferSize(hOut, bufferSize);

    //    Set ukuran viewport (area yang terlihat)
    SMALL_RECT windowSize;
    windowSize.Left = 0;
    windowSize.Top = 0;
    windowSize.Right = cols - 1;
    windowSize.Bottom = rows - 1;
    SetConsoleWindowInfo(hOut, TRUE, &windowSize);

    // 5. Hilangkan Kursor Mouse & Text Cursor
    ShowCursor(FALSE); // Mouse cursor (Windows API)

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE; // Text blinking cursor
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

int main(void) {
    // Judul Program (Gak bakal kelihatan di fullscreen, tapi bagus buat taskbar)
    SetConsoleTitle("FOURTRAIN SYSTEM");

    // Warna: Background Hitam (0), Teks Aqua/Biru Muda (B)
    system("color 0B");

    // AKTIFKAN FULLSCREEN
    set_fullscreen_mode();

    // PENTING BANGET: Jeda 0.5 detik.
    // Kasih waktu Windows "kaget" ngubah ukuran layar sebelum kita gambar UI.
    // Kalau gak ada ini, kadang layarnya blank hitam doang.
    Sleep(500);

    // Masuk ke Aplikasi
    ui_init();
    ui_run();

    return 0;
}