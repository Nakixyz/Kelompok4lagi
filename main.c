#include <windows.h>
#include <stdio.h>
#include "ui_app.h"
#include "modul_karyawan.h"

/* Fungsi Sakti: Paksa Fullscreen Tanpa Border */
void set_fullscreen_mode() {
    // 1. Ambil Handle Jendela Console
    HWND hwnd = GetConsoleWindow();

    // 2. Ubah Style Jendela: Hapus Border, Judul, Tombol X, dll
    // Ini bikin jendelanya jadi "telanjang" tanpa bingkai
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, style);

    // 3. Ambil Ukuran Layar Monitor Laptop Kamu
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);

    // 4. Paksa Jendela Console menuhin layar dari pojok (0,0) sampai ujung
    SetWindowPos(hwnd, HWND_TOP, 0, 0, screen_w, screen_h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // 5. Hilangkan Kursor Mouse biar rapi kayak mesin ATM
    ShowCursor(FALSE);
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

    karyawan_init();      // Load file + Seed 50 data
    const char ** id;
    const char ** nama;
    const char ** email;
    int ** jabatan;
    karyawan_create(id, nama, email, jabatan);
    int index;
    karyawan_delete(index);

    return 0;
}