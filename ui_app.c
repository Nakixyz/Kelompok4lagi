    #include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include "ui_app.h"
#include "globals.h"

// Include Modul Logic
#include "modul_akun.h"
#include "master/penumpang/penumpang.h"
#include "master/stasiun/stasiun.h"
#include "master/kereta/kereta.h"
#include "transaksi/jadwal/jadwal.h"
#include "transaksi/pembayaran/pembayaran.h"
#include "modul_laporan.h"  /* modul laporan (READ ONLY) */

/* Track posisi cursor terakhir untuk keperluan clipping output UI */
static int g_ui_cursor_x = 0;
static int g_ui_cursor_y = 0;

/* ================== PROTOTIPE SORTING (WAJIB DI C11) ==================
   Comparator qsort() dipakai oleh view_*() sebelum definisi fungsi di file ini.
   Di C11, implicit function declaration dianggap ERROR, jadi harus ada prototipe.
*/
static int kereta_cmp_idx(const void *a, const void *b);
static int stasiun_cmp_idx(const void *a, const void *b);
static int penumpang_cmp_idx(const void *a, const void *b);
static int jadwal_cmp_idx(const void *a, const void *b);
static int pembayaran_cmp_idx(const void *a, const void *b);

/* --- Helper Dasar --- */
/*
   Clear screen tanpa memanggil system("cls").
   Lebih cepat, mengurangi flicker, dan lebih konsisten antar terminal (CMD/PowerShell/Windows Terminal).
*/
void cls() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        system("cls");
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        system("cls");
        return;
    }

    DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
    DWORD written = 0;
    COORD home = {0, 0};

    FillConsoleOutputCharacterA(hOut, ' ', cellCount, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

void gotoXY(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    /*
      BUGFIX (UI): versi sebelumnya meng-clamp ke 250x80.
      Di fullscreen terminal yang lebih lebar/tinggi, frame kanan & tabel bisa
      tidak tergambar utuh (cursor dibatasi).

      Sekarang clamp mengikuti ukuran buffer console aktual.
    */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        int max_x = (int)csbi.dwSize.X - 1;
        int max_y = (int)csbi.dwSize.Y - 1;
        if (max_x < 0) max_x = 0;
        if (max_y < 0) max_y = 0;
        if (x > max_x) x = max_x;
        if (y > max_y) y = max_y;
    }

    /* simpan posisi cursor untuk helper clipping */
    g_ui_cursor_x = x;
    g_ui_cursor_y = y;

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

/* --- Layout & Table Helpers (UI polishing) --- */
#define FRAME_THICK 2
/* Jarak aman antara border tebal dan konten agar tampilan tidak terlihat "nempel" */
#define CONTENT_MARGIN 1

/* Jumlah baris data per halaman untuk semua tabel */
#define ROWS_PER_PAGE_FIXED 5

/* ASCII art kereta untuk panel kiri (ditampilkan jika muat) */
static const char *TRAIN_ART[] = {
    "                    ;$$&;+&&&&&&$X..                                                   ",
    "                 .$&&&&+.x&&&&&&$...;X+:.                                              ",
    "                .&$Xx++$+;:..:&&..+&&&&&&&X;..                                         ",
    "                x$..  .X.    X&X .&&&&&&&&&&&&$:                                       ",
    "               :&..  .$.    .&&:.$&&&&&:;X$&&&&&&&$;. ..                               ",
    "               xX..  :X    .$&x :&&&&&&:   .$XX&&&&&&&$+.                              ",
    "              :&&&&&&&&&&&&&&&:.$&&&&&&:   .$+  .:&$&&&&&&$.                           ",
    "              X&&&&&&&&&&&&&&X :&&&&&&&:   .$+   :&.. :&$&&$.x$;..                     ",
    "             .&&&&&&&&&&&&&&&:.X&&&&&&&&$xx:&+.  .&.  .&. .&.&&&&&X;.                  ",
    "             x&+..$&&&&$..x&$ :&&&&&&&&&&&&&&&&&$$&.. .&. .$.&&+.+&&&&X.               ",
    "             :&&&&&&&&&&+;&$..X&&&&&&&&&&&&&&&&&&&&&&&&&X;;&.&&;..X:.xX$$.;..          ",
    "            ;.;&&&&&&&&&&&$. X&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.&&&Xx$:.x;.x:$x&X;.       ",
    "             $;...;xXXx;...:&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&:&&&&&&&&&&&&:$:;:x::      ",
    "             ;&&&&&$$XX$&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&:&&&&&&&&&&&&:$&&&&&:      ",
    "              X&&&&&&&&&&&&&&&&&&&&&&&$...........................;++x:x++xxxxXX.      ",
    "              :&&&&&$$&&&&&&&&&&&&&&&x.&&.$.&&$:&&&&&xX&;&:$&x+$X;$&&&+.:&$;.:$$;.     ",
    "               x&$XXX$$&&&&&&&&&&&&$::&&&.$.&&X:;+&&&&:x.+;..xX+x+x+:;x$&&&Xx...       ",
    "               :&&&&&&&&&&&&&&&&&&X.x.$&+:&+.::X:.:&&&&$x....+x$&&&x+Xx...             ",
    "                          .......  .&$..x&$&&+.. ....:$$&&&&;;$&&+  ...                ",
    "       .......;;;X$$&&&&&&&&&&$X+;;:..;xx;..;;X$&&&&$&&$+;..;:.                        ",
    "     :X$&&&&&&&&&&$+;;x&&&$Xx+.:;:..:xX$&&&&&$x&&&&$X.....                             ",
    "      :&&&X:....:X$&&$$;.....+$&&&&&&&$$$$&&&&$: ..                                    ",
    "       .:xX$$Xx;....;;X$&&&&&&&$&&&&&$X;:......                                        ",
    "              .X&&&&&&&&&&&&X+.....:;X&x:                                              ",
    "                   ..   .;$&&&&&$.                                                     ",
    "                             ...                                                       ",
};
static const int TRAIN_ART_H = (int)(sizeof(TRAIN_ART) / sizeof(TRAIN_ART[0]));

static int g_session_account_index = -1; /* index akun yang sedang login */

static void session_set(int account_index) {
    g_session_account_index = account_index;
}

static const Account* session_me(void) {
    if (g_session_account_index >= 0 && g_session_account_index < g_accountCount) return &g_accounts[g_session_account_index];
    return NULL;
}

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int layout_split_x(int w) {
    /* Sidebar stabil: tidak kebesaran/kekecilan, dan memberi ruang untuk tabel full kolom */
    int s = w / 5;
    s = clampi(s, 20, 44);
    return s;
}

static void layout_content_bounds(int w, int split_x, int *left, int *right, int *usable_w) {
    /*
       Frame luar: x=0..1 dan x=w-2..w-1
       Frame pemisah: x=split_x..split_x+1
       Tambahkan margin agar tabel/teks tidak menempel ke border.
    */
    int l = split_x + FRAME_THICK + CONTENT_MARGIN;
    int r = (w - FRAME_THICK - 1) - CONTENT_MARGIN;
    if (left) *left = l;
    if (right) *right = r;
    if (usable_w) *usable_w = (r >= l) ? (r - l + 1) : 0;
}


static int center_within(int left, int right, int box_w) {
    int area_w = right - left + 1;
    int x = left + (area_w - box_w) / 2;
    if (x < left) x = left;
    if (x + box_w - 1 > right) x = right - box_w + 1;
    if (x < left) x = left;
    return x;
}

static int table_total_width(const int *col_w, int ncols) {
    int total = 1; /* leading '|' */
    for (int i = 0; i < ncols; i++) total += col_w[i] + 3; /* "| <col> |" */
    return total;
}

/*
   Helper untuk baris kosong tabel.
   Penting untuk mencegah "ghost UI" (kolom sisa / highlight bocor) ketika
   ROWS_PER_PAGE lebih besar daripada jumlah data di halaman terakhir.
*/
static void table_print_blank_row(int x, int y, const int *col_w, int ncols) {
    gotoXY(x, y);
    putchar('|');
    for (int i = 0; i < ncols; i++) {
        printf(" %-*s |", col_w[i], "");
    }
}

static void fit_columns(int *col_w, const int *min_w, int ncols, int max_table_w) {
    /*
       Menjaga tabel selalu pas di area konten:
       - Jika kebesaran: diperkecil sampai muat (tidak kurang dari min_w)
       - Jika masih ada ruang: dibesarkan lagi agar "mentok" area konten
    */
    int cur = table_total_width(col_w, ncols);
    int diff = max_table_w - cur;

    if (diff == 0) return;

    /* terlalu lebar -> shrink */
    if (diff < 0) {
        int need = -diff;
        while (need > 0) {
            int best = -1;
            int best_extra = 0;
            for (int i = 0; i < ncols; i++) {
                int extra = col_w[i] - min_w[i];
                if (extra > best_extra) {
                    best_extra = extra;
                    best = i;
                }
            }
            if (best < 0 || best_extra <= 0) break;
            int dec = (best_extra > need) ? need : best_extra;
            col_w[best] -= dec;
            need -= dec;
        }
        return;
    }

    /* masih ada ruang -> expand (skip kolom No jika ada) */
    int spare = diff;
    if (ncols <= 0) return;
    int start = (ncols > 1) ? 1 : 0;
    int i = start;
    while (spare > 0) {
        col_w[i] += 1;
        spare--;
        i++;
        if (i >= ncols) i = start;
        if (start == 0 && i == 0) break; /* safety */
    }
}


/* --- Console Color Helper (highlight selection) --- */
static WORD g_defaultAttr = 0;

/* forward decl: helper UI box dipakai oleh semua fungsi input */
static void ui_draw_input_box_inline(int inner_w);

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
        /* Highlight putih (background putih, teks hitam) */
        WORD attr = (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
    } else {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), g_defaultAttr);
    }
}



/* Password input: TAB tampil/sembunyi, spasi ditolak */
void input_password_masked(char *buffer, int max_len, int x, int y, int field_w) {
    int i = 0;
    int visible = 0;

    int limit = max_len - 1;
    if (field_w > 0 && field_w < limit) limit = field_w;

    buffer[0] = '\0';

    /* gambar field box [     ] agar konsisten di semua input */
    if (field_w > 0) {
        gotoXY(x, y);
        ui_draw_input_box_inline(field_w);
    }

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        // ENTER
        if (ch == 27) { buffer[0] = 27; return; }         // ESC

        if (ch == 9) {                                    // TAB toggle
            visible = !visible;

            /* redraw isi field tanpa menghapus bracket */
            gotoXY(x + 1, y);
            for (int k = 0; k < field_w; k++) putchar(' ');
            gotoXY(x + 1, y);
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

    /* Batasi jumlah karakter yang bisa diketik agar tidak melewati kotak [ ] */
    int limit = max_len - 1;
    if (limit < 1) limit = 1;

    /* gambar kotak input [     ] secara otomatis (global) */
    {
        int inner = max_len - 1;
        if (inner < 1) inner = 1;
        if (inner > 40) inner = 40; /* jangan terlalu panjang */
        ui_draw_input_box_inline(inner);

        /* selaraskan limit input dengan lebar box yang digambar */
        if (inner < limit) limit = inner;
    }

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
            if (i < limit) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                Beep(800, 40);
            }
        }
    }
}

/* ================== INPUT PLAIN (khusus LOGIN) ==================

/* Password input plain untuk login: TAB tampil/sembunyi, tanpa kotak [ ] */
static void input_password_masked_login_plain(char *buffer, int max_len, int x, int y, int field_w) {
    int i = 0;
    int visible = 0;
    if (!buffer || max_len <= 0) return;

    int limit = max_len - 1;
    if (field_w > 0 && field_w < limit) limit = field_w;
    buffer[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        /* ENTER */
        if (ch == 27) { buffer[0] = 27; return; }         /* ESC */

        if (ch == 9) {                                    /* TAB toggle */
            visible = !visible;
            gotoXY(x, y);
            for (int k = 0; k < field_w; k++) putchar(' ');
            gotoXY(x, y);
            for (int k = 0; k < i; k++) putchar(visible ? buffer[k] : '*');
            continue;
        }

        if (ch == 8) {                                    /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            if (ch == ' ') { Beep(800, 40); continue; }   /* tolak spasi */
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


/* Input text plain untuk login (tanpa kotak UI).
   - max_len = kapasitas buffer (termasuk '\0')
   - allow_space: 0 = tolak spasi, 1 = boleh spasi
   Catatan: ESC akan mengisi buffer[0] = 27 agar caller bisa exit.
*/
static void input_text_login_plain(char *buffer, int max_len, int allow_space) {
    int i = 0;
    if (!buffer || max_len <= 0) return;
    buffer[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 27) {                 /* ESC */
            buffer[0] = 27;
            buffer[1] = '\0';
            return;
        }

        if (ch == 13) {                 /* ENTER */
            buffer[i] = '\0';
            return;
        }

        if (ch == 8) {                  /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch == 0 || ch == 224) {     /* tombol spesial: abaikan */
            (void)_getch();
            continue;
        }

        if (!allow_space && ch == ' ') { /* tolak spasi */
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

/* Input angka saja.
   - Karakter selain 0-9 DIABAIKAN (tidak tampil di layar).
   - Tombol panah/F1 dst (0/224) juga diabaikan supaya tidak ada beep.
   - max_len = kapasitas buffer (termasuk '\0').
*/
void input_digits(char *buffer, int max_len) {
    int i = 0;
    if (!buffer || max_len <= 0) return;
    buffer[0] = '\0';

    /* Batasi jumlah digit agar tidak melewati kotak [ ] */
    int limit = max_len - 1;
    if (limit < 1) limit = 1;

    /* gambar kotak input [     ] secara otomatis (global) */
    {
        int inner = max_len - 1;
        if (inner < 1) inner = 1;
        if (inner > 20) inner = 20;
        ui_draw_input_box_inline(inner);

        /* selaraskan limit input dengan lebar box yang digambar */
        if (inner < limit) limit = inner;
    }

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        /* ENTER */
        if (ch == 27) { buffer[0] = 27; return; }         /* ESC */

        /* extended key (arrow, function key) -> consume & ignore */
        if (ch == 0 || ch == 224) {
            (void)_getch();
            continue;
        }

        if (ch == 8) {                                    /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch >= '0' && ch <= '9') {
            if (i < limit) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            }
            /* kalau penuh: abaikan (tanpa beep) */
            continue;
        }

        /* selain digit: abaikan (tanpa beep) */
    }
}

/* ================== INPUT FILTERED (KARAKTER TERLARANG TIDAK AKAN MUNCUL) ==================
   Dipakai untuk field yang punya rule ketat.
   Semua input di-handle per-keypress, karakter yang tidak diizinkan diabaikan.
*/

static void input_alpha_only_filtered(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';

    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    /* tampilkan kotak input agar konsisten di semua form */
    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        /* ENTER */
        if (ch == 27) { buffer[0] = 27; return; }          /* ESC */

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {                                     /* BACKSPACE */
            if (i > 0) {
                i--; buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (isalpha((unsigned char)ch)) {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                /* penuh -> abaikan */
            }
        } else {
            /* karakter terlarang diabaikan (tidak tampil) */
        }
    }
}

static void input_alpha_space_filtered(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';

    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    /* tampilkan kotak input agar konsisten di semua form */
    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }
        if (ch == 27) { buffer[0] = 27; return; }

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {
            if (i > 0) {
                i--; buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch == ' ' || ch == '\t') {
            if (i < max_chars) {
                buffer[i++] = ' ';
                buffer[i] = '\0';
                putchar(' ');
            } else {
                /* penuh -> abaikan */
            }
            continue;
        }

        if (isalpha((unsigned char)ch)) {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                /* penuh -> abaikan */
            }
        } else {
            /* karakter terlarang diabaikan */
        }
    }
}

/* Nama orang: huruf + spasi + ('-.) ; digit/simbol lain tidak tampil */
static void input_person_name_filtered(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    /* tampilkan kotak input agar konsisten di semua form */
    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }
        if (ch == 27) { buffer[0] = 27; return; }

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {
            if (i > 0) {
                i--; buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch == ' ' || ch == '\t') {
            if (i < max_chars) {
                buffer[i++] = ' ';
                buffer[i] = '\0';
                putchar(' ');
            } else {
                /* penuh -> abaikan */
            }
            continue;
        }

        if (isalpha((unsigned char)ch) || ch == '\'' || ch == '-' || ch == '.') {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                /* penuh -> abaikan */
            }
        } else {
            /* karakter terlarang diabaikan */
        }
    }
}

/* Surel: karakter umum email saja, spasi/simbol liar tidak tampil */
static int is_email_char_ui(int ch) {
    unsigned char c = (unsigned char)ch;
    return isalnum(c) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+';
}

/* -------------------------------------------------------------------------
   INPUT UI HELPERS
   Membuat tampilan input lebih profesional: semua field input akan tampil
   sebagai kotak sederhana [          ] sehingga user tahu area ketik.
   Kotak digambar tepat di posisi kursor saat fungsi input dipanggil.
   ------------------------------------------------------------------------- */

static void ui_draw_input_box_inline(int inner_w) {
    if (inner_w <= 0) return;

    /*
       Guard: jangan pernah menggambar kotak input melewati border kanan.
       Ini penting untuk terminal dengan lebar kecil / layout yang padat.
    */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int cur_x = 0;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        cur_x = (int)csbi.dwCursorPosition.X;
    }
    int screen_w = get_screen_width();
    /* kolom paling aman sebelum border tebal kanan + margin */
    int safe_right = screen_w - FRAME_THICK - 1 - CONTENT_MARGIN;
    if (safe_right < 0) safe_right = screen_w - 1;

    /* total lebar kotak = '[' + inner_w + ']' */
    int total = inner_w + 2;
    if (cur_x + total - 1 > safe_right) {
        int max_inner = safe_right - cur_x - 1; /* sisakan untuk ']' */
        if (max_inner < 1) return;
        inner_w = max_inner;
        total = inner_w + 2;
    }

    /* gambar box di posisi saat ini */
    putchar('[');
    for (int i = 0; i < inner_w; i++) putchar(' ');
    putchar(']');

    /* balikkan kursor ke dalam box (setelah '[') */
    for (int i = 0; i < inner_w + 1; i++) putchar('\b');
}

static void input_email_filtered(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    /* tampilkan kotak input */
    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }
        if (ch == 27) { buffer[0] = 27; return; }

        if (ch == 8) {
            if (i > 0) {
                i--; buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (is_email_char_ui(ch)) {
            /* Surel tidak menerima huruf besar: tampilkan & simpan sebagai lowercase */
            if (isupper((unsigned char)ch)) ch = tolower((unsigned char)ch);
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            } else {
                /* penuh -> abaikan */
            }
        } else {
            /* karakter terlarang diabaikan */
        }
    }
}

/* Tanggal lahir DD-MM-YYYY: hanya digit, '-' otomatis di posisi 3 & 6 */
static void input_date_ddmmyyyy_filtered(char *buffer, int buffer_sz) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';

    /* DD-MM-YYYY panjang 10 */
    ui_draw_input_box_inline(10);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }
        if (ch == 27) { buffer[0] = 27; return; }

        if (ch == 8) {
            if (i > 0) {
                i--;
                printf("\b \b");
                if (i > 0 && buffer[i - 1] == '-') {
                    i--;
                    printf("\b \b");
                }
                buffer[i] = '\0';
            }
            continue;
        }

        if (!isdigit((unsigned char)ch)) {
            /* non-digit diabaikan */
            continue;
        }

        if (i >= 10) { /* penuh */ continue; }
        if (i == 2 || i == 5) {
            buffer[i++] = '-';
            putchar('-');
        }
        if (i < 10) {
            buffer[i++] = (char)ch;
            buffer[i] = '\0';
            putchar((char)ch);
        }
    }
}

/* Jenis kelamin: hanya 1 karakter L/P (case bebas). Selain itu tidak tampil. */
static void input_gender_lp_filtered(char *buffer, int buffer_sz) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';

    ui_draw_input_box_inline(1);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }
        if (ch == 27) { buffer[0] = 27; return; }

        if (ch == 8) {
            if (i > 0) {
                i = 0;
                buffer[0] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (i >= 1) { /* cuma 1 char */ continue; }

        if (ch == 'L' || ch == 'l' || ch == 'P' || ch == 'p') {
            buffer[i++] = (char)ch;
            buffer[i] = '\0';
            putchar((char)ch);
        } else {
            /* selain L/P diabaikan */
        }
    }
}

/* Input opsi (kelas/status): huruf & spasi + digit tertentu saja (mis: "123" atau "12"). */
static void input_option_text_filtered(char *buffer, int buffer_sz, int max_chars, const char *allowed_digits) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    /* tampilkan kotak input agar konsisten di semua form */
    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }
        if (ch == 27) { buffer[0] = 27; return; }

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {
            if (i > 0) {
                i--; buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch == ' ' || ch == '\t') {
            if (i < max_chars) { buffer[i++] = ' '; buffer[i] = '\0'; putchar(' '); }
            continue;
        }

        if (isalpha((unsigned char)ch)) {
            if (i < max_chars) { buffer[i++] = (char)ch; buffer[i] = '\0'; putchar((char)ch); }
            continue;
        }

        if (allowed_digits && strchr(allowed_digits, ch) != NULL) {
            if (i < max_chars) { buffer[i++] = (char)ch; buffer[i] = '\0'; putchar((char)ch); }
            continue;
        }

        /* digit lain / simbol -> diabaikan */
    }
}

/* Input pilihan digit tertentu saja (mis: "12"), karakter lain ditolak (tidak tampil). */
static void input_choice_digits_filtered(char *buffer, int buffer_sz, const char *allowed_digits, int allow_blank) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';

    /* kotak input 1 digit */
    ui_draw_input_box_inline(1);

    while (1) {
        int ch = _getch();

        if (ch == 13) { /* ENTER */
            if (allow_blank || i > 0) { buffer[i] = '\0'; break; }
            Beep(800, 40);
            continue;
        }
        if (ch == 27) { buffer[0] = 27; return; } /* ESC */

        if (ch == 8) { /* BACKSPACE */
            if (i > 0) {
                i--; buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (allowed_digits && strchr(allowed_digits, ch) != NULL) {
            if (i >= buffer_sz - 1) { continue; }
            if (i >= 1) { continue; } /* cuma 1 digit */
            buffer[i++] = (char)ch;
            buffer[i] = '\0';
            putchar((char)ch);
            continue;
        }

        /* selain digit allowed -> diabaikan */
    }
}


/* Input teks tanpa spasi & tanpa simbol liar (alnum saja).
   Dipakai untuk ID (KAxxx, STSxxx, JDWxxx, dll). */
static int is_alnum_ui(int ch) {
    unsigned char c = (unsigned char)ch;
    return isalnum(c);
}

static void input_text_filtered(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        /* ENTER */
        if (ch == 27) { buffer[0] = 27; return; }         /* ESC */

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {                                    /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (is_alnum_ui(ch)) {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            }
            continue;
        }

        /* selain alnum -> diabaikan */
    }
}

/* Input ID seperti input_text_filtered, tapi mendukung shortcut F1/F2 untuk membuka menu referensi.
   Return:
   - 0 = selesai normal (ENTER)
   - 1 = F1 ditekan
   - 2 = F2 ditekan
   - -1 = dibatalkan (ESC)
   Catatan: hanya dipakai di form transaksi (CRD) agar UX lebih mudah. */
static int input_text_filtered_fkeys(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return -1;
    buffer[0] = '\0';
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; return 0; }  /* ENTER */
        if (ch == 27) { buffer[0] = 27; return -1; }   /* ESC */

        /* Extended key: F1/F2 */
        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 59) return 1; /* F1 */
            if (ext == 60) return 2; /* F2 */
            continue;
        }

        if (ch == 8) { /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (is_alnum_ui(ch)) {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            }
            continue;
        }
        /* selain alnum -> diabaikan */
    }
}

/* Input digits saja (lebih fleksibel daripada input_choice_digits_filtered). */
static void input_digits_only_filtered(char *buffer, int buffer_sz, int max_chars) {
    int i = 0;
    if (!buffer || buffer_sz <= 0) return;
    buffer[0] = '\0';
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;

    ui_draw_input_box_inline(max_chars);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        /* ENTER */
        if (ch == 27) { buffer[0] = 27; return; }         /* ESC */

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {                                    /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch >= '0' && ch <= '9') {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            }
            continue;
        }

        /* selain digit -> diabaikan */
    }
}

/* Input uang: tampilkan prefix "Rp " agar user tahu ini nominal uang.
   Nilai yang disimpan tetap hanya digit (tanpa Rp). */
static void input_money_rp_only_filtered(char *buffer, int buffer_sz, int max_digits) {
    /* Prefix yang terlihat user */
    printf("Rp ");
    input_digits_only_filtered(buffer, buffer_sz, max_digits);
}

/* ===================== FORMAT RUPIAH & WAKTU =====================

   UI dibuat lebih profesional dengan:
   - Nominal uang konsisten: Rp 55.000 (bukan Rp 55000)
   - Timestamp compact "YYYYMMDDhhmmss" ditampilkan sebagai "DD-MM-YYYY hh:mm:ss"
*/

static void format_money_rp_int(long long value, char *out, int out_sz) {
    /* Format Rupiah dengan pemisah ribuan (.)
       Contoh: 1950000 -> "Rp 1.950.000" */
    if (!out || out_sz <= 0) return;

    long long v = value;
    int negative = 0;
    if (v < 0) { negative = 1; v = -v; }

    char digits[64];
    snprintf(digits, sizeof(digits), "%lld", v);
    int len = (int)strlen(digits);

    /* Hitung jumlah titik */
    int dots = (len > 3) ? ( (len - 1) / 3 ) : 0;
    int out_len = len + dots;

    char grouped[80];
    int gi = out_len;
    grouped[gi] = '\0';

    int di = len - 1;
    int group = 0;
    while (di >= 0) {
        if (group == 3) {
            grouped[--gi] = '.';
            group = 0;
        }
        grouped[--gi] = digits[di--];
        group++;
    }

    if (negative) {
        snprintf(out, out_sz, "Rp -%s", grouped);
    } else {
        snprintf(out, out_sz, "Rp %s", grouped);
    }
}


static void format_compact_date_id(const char *compact, char *out, int out_sz) {
    /* compact: "YYYYMMDD..." -> "DD-MM-YYYY" */
    if (!out || out_sz <= 0) return;
    out[0] = '\0';
    if (!compact) return;

    int n = (int)strlen(compact);
    if (n < 8) {
        snprintf(out, out_sz, "%s", compact);
        return;
    }

    char yyyy[5]={0}, mm[3]={0}, dd[3]={0};
    memcpy(yyyy, compact + 0, 4);
    memcpy(mm,   compact + 4, 2);
    memcpy(dd,   compact + 6, 2);

    snprintf(out, out_sz, "%s-%s-%s", dd, mm, yyyy);
}

/* ===================== LOOKUP NAMA MASTER =====================
   UI menampilkan NAMA (bukan ID) untuk Kereta & Stasiun.
   Input tetap memakai ID (kode). Jika tidak ditemukan, fallback ke ID.
*/
static const char* ui_kereta_nama_by_kode(const char *kode) {
    if (!kode || !kode[0]) return "";
    for (int i = 0; i < g_keretaCount; i++) {
        if (!g_kereta[i].active) continue;
        if (strcmp(g_kereta[i].kode, kode) == 0) return g_kereta[i].nama;
    }
    return kode;
}

static const char* ui_stasiun_nama_by_id(const char *id) {
    if (!id || !id[0]) return "";
    for (int i = 0; i < g_stasiunCount; i++) {
        if (!g_stasiun[i].active) continue;
        if (strcmp(g_stasiun[i].id, id) == 0) return g_stasiun[i].nama;
    }
    return id;
}

static const char* ui_penumpang_nama_by_id(const char *id) {
    if (!id || !id[0]) return "";
    for (int i = 0; i < g_penumpangCount; i++) {
        if (!g_penumpang[i].active) continue;
        if (strcmp(g_penumpang[i].id, id) == 0) return g_penumpang[i].nama;
    }
    return id;
}



/* Ambil tanggal (DD-MM-YYYY) dari string datetime ("DD-MM-YYYY HH:MM").
   Mengembalikan pointer ke buffer statik. */
static const char* ui_extract_jadwal_date(const char *datetime_jadwal) {
    static char buf[16];
    buf[0] = '\0';

    if (!datetime_jadwal || !datetime_jadwal[0]) return "";

    int i = 0;
    /* copy sampai spasi (tanggal saja) */
    while (datetime_jadwal[i] && datetime_jadwal[i] != ' ' && i < 10) {
        buf[i] = datetime_jadwal[i];
        i++;
    }
    buf[i] = '\0';

    return buf;
}

/* Untuk tampilan pembayaran: tampilkan TGL BERANGKAT jadwal (DD-MM-YYYY) dari ID jadwal. Diambil dari waktu_berangkat./;
   Jika tidak ditemukan, fallback ke ID jadwal. */
static const char* ui_jadwal_tgl_by_id(const char *id_jadwal) {
    if (!id_jadwal || !id_jadwal[0]) return "";
    for (int i = 0; i < g_jadwalCount; i++) {
        if (!g_jadwal[i].active) continue;
        if (strcmp(g_jadwal[i].id_jadwal, id_jadwal) == 0) return ui_extract_jadwal_date(g_jadwal[i].waktu_berangkat);
    }
    return id_jadwal;
}


/* Prefill edit: tampilkan nilai awal, lalu izinkan edit.
   Karakter yang diizinkan: printable ASCII (32..126). */
static void input_text_prefill_filtered(char *buffer, int buffer_sz, int max_chars) {
    if (!buffer || buffer_sz <= 0) return;

    int i = (int)strlen(buffer);
    if (i < 0) i = 0;
    if (max_chars > buffer_sz - 1) max_chars = buffer_sz - 1;
    if (i > max_chars) i = max_chars;
    buffer[i] = '\0';

    /* kotak input + tampilkan nilai awal */
    ui_draw_input_box_inline(max_chars);

    /* tampilkan prefill */
    for (int k = 0; k < i; k++) putchar(buffer[k]);

    while (1) {
        int ch = _getch();

        if (ch == 13) { buffer[i] = '\0'; break; }        /* ENTER */
        if (ch == 27) { buffer[0] = 27; return; }         /* ESC */

        if (ch == 0 || ch == 224) { (void)_getch(); continue; } /* extended key */

        if (ch == 8) {                                    /* BACKSPACE */
            if (i > 0) {
                i--;
                buffer[i] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            if (i < max_chars) {
                buffer[i++] = (char)ch;
                buffer[i] = '\0';
                putchar((char)ch);
            }
            continue;
        }

        /* selain printable -> diabaikan */
    }
}



/* forward decl (dipakai sebelum definisi di bawah) */
static void print_padded(int x, int y, int width, const char *fmt, ...);
static void print_clipped(int x, int y, int max_w, const char *text);

static void draw_popup_box(int x, int y, int w, int h, const char *title) {
    /*
       POPUP dinonaktifkan: sesuai request UI, semua form/detail tampil di area bawah navigasi.
       Fungsi ini sekarang hanya: membersihkan area dan menulis judul sederhana (tanpa box).
    */
    if (w < 4 || h < 2) return;

    /* clear area */
    for (int yy = 0; yy < h; yy++) {
        gotoXY(x, y + yy);
        for (int i = 0; i < w; i++) putchar(' ');
    }

    /* title: baris penuh '=' supaya tidak terlihat "bolong" ketika overlay */
    if (title && title[0]) {
        if (w > 0) {
            char line[1024];
            if (w >= (int)sizeof(line)) w = (int)sizeof(line) - 1;

            /* fill '=' */
            memset(line, '=', (size_t)w);
            line[w] = '\0';

            /* sisipkan judul di tengah */
            char mid[256];
            snprintf(mid, sizeof(mid), "==[ %s ]==", title);
            int mlen = (int)strlen(mid);
            if (mlen > w) mlen = w;
            int start = (w - mlen) / 2;
            if (start < 0) start = 0;
            memcpy(&line[start], mid, (size_t)mlen);

            gotoXY(x, y);
            printf("%s", line);
        }
    }
}

/* Helper: gambar 1 baris label form ("Label : ") dan kembalikan posisi input.
   Ditaruh di area umum (bukan di blok modul Karyawan) karena dipakai oleh modul Akun. */
static void form_row_draw(int pop_x, int pop_y, int row_y,
                          int label_w, int input_w,
                          const char *label,
                          int *out_x_input, int *out_y_input) {
    int x_label = pop_x + 3;
    int x_input = x_label + label_w + 3; /* label + " : " */
    int y = pop_y + row_y;

    gotoXY(x_label, y);
    printf("%-*.*s : ", label_w, label_w, label ? label : "");

    /* Bersihkan area input agar tidak ada sisa karakter dari render sebelumnya. */
    int screen_w = get_screen_width();
    int safe_right = screen_w - FRAME_THICK - 1 - CONTENT_MARGIN;
    if (safe_right < 0) safe_right = screen_w - 1;
    int max_w = safe_right - x_input + 1;
    if (max_w < 0) max_w = 0;
    if (input_w > max_w) input_w = max_w;
    gotoXY(x_input, y);
    for (int i = 0; i < input_w; i++) putchar(' ');

    if (out_x_input) *out_x_input = x_input;
    if (out_y_input) *out_y_input = y;
}



/* -------------------------------------------------------------------------
   FORM ACTIONS BAR
   Menampilkan tombol aksi di bawah form agar konsisten & profesional.
   Dipakai untuk semua Add/Edit/Delete/Detail yang tampil di panel bawah.
   ------------------------------------------------------------------------- */


/* ================== DRAW PRIMITIVES (ANTI-BOLONG) ==================
   Semua komponen UI yang hidup "di dalam box" wajib menggambar HANYA
   pada area inner (tanpa menyentuh border). Ini mencegah border terhapus
   oleh separator/action bar/clear line.
*/
static void ui_box_inner(int x, int y, int w, int h, int *ix, int *iy, int *iw, int *ih) {
    (void)y; (void)h;
    if (ix) *ix = x + 1;
    if (iy) *iy = y + 1;
    if (iw) *iw = (w >= 2) ? (w - 2) : 0;
    if (ih) *ih = (h >= 2) ? (h - 2) : 0;
}


static void ui_hline(int x, int y, int w, char ch);


static void ui_hline(int x, int y, int w, char ch) {
    if (w <= 0) return;
    char buf[1024];
    if (w >= (int)sizeof(buf)) w = (int)sizeof(buf) - 1;
    memset(buf, ch, (size_t)w);
    buf[w] = '\0';
    gotoXY(x, y);
    printf("%s", buf);
}

static void ui_clear_line(int x, int y, int w) {
    if (w <= 0) return;
    char buf[1024];
    if (w >= (int)sizeof(buf)) w = (int)sizeof(buf) - 1;
    memset(buf, ' ', (size_t)w);
    buf[w] = '\0';
    gotoXY(x, y);
    printf("%s", buf);
}

static void ui_draw_actions_bar(int x, int y, int w, int h, const char *text) {
    /*
       ACTION BAR (ANTI-BOLONG) - PROFESSIONAL FINAL

       Di versi project kamu, draw_popup_box() sudah TIDAK menggambar border popup,
       tapi hanya clear area + judul. Selain itu, bottom panel memakai box frame
       sendiri dan form/detail digambar di AREA INNER.

       Jadi kontrak paling aman adalah: x,y,w,h = AREA KONTEN (INNER / TANPA BORDER).
       Fungsi ini akan menggambar separator + teks aksi di 2 baris paling bawah
       dari area konten, tanpa menyentuh border luar layout.

       Ini menghilangkan kasus "garis hilang/bolong" saat Create/Edit/Detail,
       karena sebelumnya action bar meng-offset x+2 / w-4 seolah ada border popup.
    */
    if (w < 10 || h < 3) return;

    /* separator tepat di atas baris aksi */
    int sep_y = y + h - 2;
    int act_y = y + h - 1;

    int scr_h = get_screen_height();
    if (scr_h <= 0) scr_h = 30;

    /* jangan pernah menggambar di 3 baris terbawah layar (area frame luar) */
    int safe_max_y = scr_h - 4;
    if (safe_max_y < 0) safe_max_y = 0;
    if (act_y > safe_max_y) {
        act_y = safe_max_y;
        sep_y = act_y - 1;
    }
    if (sep_y < y) return;

    /* separator full width (inner) */
    ui_hline(x, sep_y, w, '=');

    /* clear action line full width */
    ui_clear_line(x, act_y, w);

    /* print action text (clipped) */
    if (text && text[0]) {
        int pad = 2;
        int tx = x + pad;
        int max_w = w - (pad * 2);
        if (max_w < 1) max_w = 1;
        print_clipped(tx, act_y, max_w, text);
    }
}



static void ui_actions_save_cancel(int x, int y, int w, int h) {
    ui_draw_actions_bar(x, y, w, h, "Aksi: [ENTER] Simpan   [ESC] Batal");
}

static void ui_actions_confirm_cancel(int x, int y, int w, int h) {
    ui_draw_actions_bar(x, y, w, h, "Aksi: [ENTER] Konfirmasi   [ESC] Batal");
}


/* -------------------------------------------------------------------------
   FORM HELPERS (TIPS)
   Dipakai untuk menampilkan bantuan input agar rapi (tidak sesak).
   Ditampilkan tepat di atas actions bar.
   ------------------------------------------------------------------------- */
static void ui_draw_helpers_block(int x, int y, int w, int h,
                                  const char *title,
                                  const char *const *lines,
                                  int n) {
    /* gunakan area inner supaya tidak menimpa border */
    int ix, iy, iw, ih;
    ui_box_inner(x, y, w, h, &ix, &iy, &iw, &ih);
    x = ix; w = iw;

    if (!lines || n <= 0 || w < 24 || h < 8) return;

    int sep_y = y + h - 3;             /* garis actions bar */
    int max_w = w - 6;                 /* padding kiri/kanan */

    /* start_y diset supaya ada 1 baris jarak dari actions bar */
    int start_y = sep_y - (n + 1) - 1; /* (judul + n baris) + jarak */

    /* jangan sampai naik melewati area judul box */
    if (start_y < y + 2) start_y = y + 2;

    /* judul info */
    if (title && title[0]) {
        print_clipped(x + 3, start_y, max_w, title);
    } else {
        print_clipped(x + 3, start_y, max_w, "Info:");
    }

    /* isi info */
    for (int i = 0; i < n; i++) {
        char out[256];
        snprintf(out, sizeof(out), "- %s", lines[i] ? lines[i] : "");
        print_clipped(x + 5, start_y + 1 + i, max_w - 2, out);
    }
}



static void ui_draw_helpers_block_below(int x, int y, int w, int h,
                                        int min_y,
                                        const char *title,
                                        const char *const *lines,
                                        int n) {
    /* gunakan area inner supaya tidak menimpa border */
    int ix, iy, iw, ih;
    ui_box_inner(x, y, w, h, &ix, &iy, &iw, &ih);
    x = ix; w = iw;

    if (!lines || n <= 0 || w < 24 || h < 8) return;

    int sep_y = y + h - 3;             /* garis actions bar */
    int max_w = w - 6;                 /* padding kiri/kanan */

    /* default posisi dekat actions bar */
    int start_y = sep_y - (n + 1) - 1; /* (judul + n baris) + jarak */

    /* jangan sampai naik melewati area judul box */
    if (start_y < y + 2) start_y = y + 2;

    /* pastikan tidak menimpa area form (di atas min_y) */
    if (start_y < min_y) start_y = min_y;

    /* kalau tidak cukup ruang, jangan gambar block */
    if (start_y + 1 + (n - 1) >= sep_y - 1) return;

    /* bersihkan area supaya tidak ada sisa karakter */
    ui_clear_line(x, start_y, w);
    for (int i = 0; i < n; i++) ui_clear_line(x, start_y + 1 + i, w);

    /* judul info */
    if (title && title[0]) {
        print_clipped(x + 3, start_y, max_w, title);
    } else {
        print_clipped(x + 3, start_y, max_w, "Info:");
    }

    /* isi info */
    for (int i = 0; i < n; i++) {
        char out[256];
        snprintf(out, sizeof(out), "- %s", lines[i] ? lines[i] : "");
        print_clipped(x + 5, start_y + 1 + i, max_w - 2, out);
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


static int looks_like_email(const char *s) {
    if (!s || !*s) return 0;

    /* Tidak boleh ada whitespace */
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        if (isspace(*p)) return 0;
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

/* No. Telp: tepat 12 digit dan diawali '08' */
static int looks_like_phone_08_len12(const char *s) {
    if (!s) return 0;
    size_t n = strlen(s);
    if (n != 12) return 0;
    if (s[0] != '0' || s[1] != '8') return 0;
    for (size_t i = 0; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}


/* --- Validasi khusus Kereta (UI) --- */
static int is_alpha_space_only(const char *s) {
    if (!s || !*s) return 0;
    int has_alpha = 0;
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        unsigned char c = *p;
        if (c == ' ' || c == '\t') continue;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) { has_alpha = 1; continue; }
        return 0; /* angka & simbol ditolak */
    }
    return has_alpha;
}

static void token_lower_nospace_ui(char *out, int out_sz, const char *in) {
    if (!out || out_sz <= 0) return;
    out[0] = '\0';
    if (!in) return;
    int j = 0;
    for (const unsigned char *p = (const unsigned char*)in; *p && j + 1 < out_sz; p++) {
        unsigned char c = *p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        else if (c >= 'a' && c <= 'z') { /* ok */ }
        else if (c >= '0' && c <= '9') { /* ok */ }
        else continue;
        out[j++] = (char)c;
    }
    out[j] = '\0';
}

static int normalize_kelas_ui(char *out, int out_sz, const char *in) {
    char tok[32];
    token_lower_nospace_ui(tok, (int)sizeof(tok), in);
    if (tok[0] == '\0') return 0;

    if (strcmp(tok, "1") == 0 || strcmp(tok, "bisnis") == 0) {
        snprintf(out, (size_t)out_sz, "%s", "Bisnis");
        return 1;
    }
    if (strcmp(tok, "2") == 0 || strcmp(tok, "ekonomi") == 0) {
        snprintf(out, (size_t)out_sz, "%s", "Ekonomi");
        return 1;
    }
    if (strcmp(tok, "3") == 0 || strcmp(tok, "eksekutif") == 0) {
        snprintf(out, (size_t)out_sz, "%s", "Eksekutif");
        return 1;
    }
    return 0;
}

static int normalize_status_ui(char *out, int out_sz, const char *in) {
    char tok[32];
    token_lower_nospace_ui(tok, (int)sizeof(tok), in);
    if (tok[0] == '\0') return 0;

    if (strcmp(tok, "1") == 0 || strcmp(tok, "aktif") == 0) {
        snprintf(out, (size_t)out_sz, "%s", "Aktif");
        return 1;
    }

    if (strcmp(tok, "2") == 0 || strcmp(tok, "maintenance") == 0 || strcmp(tok, "maintenacne") == 0 ||
        strncmp(tok, "maint", 5) == 0) {
        snprintf(out, (size_t)out_sz, "%s", "Perawatan");
        return 1;
    }

    return 0;
}

/* --- Validasi khusus Stasiun (UI) --- */

/* hanya huruf A-Z/a-z dan panjang tepat N */
static int is_alpha_only_exact_len(const char *s, int n) {
    if (!s) return 0;
    if ((int)strlen(s) != n) return 0;
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        unsigned char c = *p;
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return 0;
    }
    return 1;
}



/* mapping: No (tampilan) -> index asli array */

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
    /* tampilkan semua akun (aktif + nonaktif) agar kolom Status bermakna */
    int n = 0;
    for (int i = 0; i < g_accountCount && n < max_out; i++) {
        out_idx[n++] = i;
    }
    return n;
}


/* --- GAMBAR LAYOUT DASAR --- */
static const char* role_to_label(Role role);

static void console_write_line(int y, const char *line, int w) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    COORD c = {0, (SHORT)y};
    SetConsoleCursorPosition(hOut, c);
    WriteConsoleA(hOut, line, (DWORD)w, &written, NULL);
}

static void print_clipped(int x, int y, int max_w, const char *text) {
    if (max_w <= 0) return;
    if (!text) text = "";
    gotoXY(x, y);
    printf("%.*s", max_w, text);
}

static void print_padded(int x, int y, int width, const char *fmt, ...) {
    if (width <= 0) return;
    char buf[768];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    gotoXY(x, y);
    /* pad + clip supaya garis footer/label selalu rapi */
    printf("%-*.*s", width, width, buf);
}

/* ===== Form helper: pesan peringatan konsisten untuk field wajib (Master) ===== */
static void ui_form_message(int pop_x, int pop_y, int pop_w, int pop_h, const char *msg) {
    int x = pop_x + 2;
    int w = pop_w - 4;
    /*
       Action bar menggambar separator di (pop_y + pop_h - 3).
       Pesan peringatan harus 1 baris di atasnya supaya tidak menimpa garis '+-----+'.
    */
    int y = pop_y + pop_h - 4; /* baris pesan di atas separator action bar */
    if (w < 10) return;
    if (y <= pop_y) return;
    print_padded(x, y, w, "%s", msg ? msg : "");
}
static void ui_form_message_clear(int pop_x, int pop_y, int pop_w, int pop_h) {
    ui_form_message(pop_x, pop_y, pop_w, pop_h, "");
}

/* Helper: validasi format dengan pesan konsisten (tanpa keluar dari popup) */
static int ui_require_valid(int ok, int pop_x, int pop_y, int pop_w, int pop_h, const char *msg) {
    if (ok) {
        ui_form_message_clear(pop_x, pop_y, pop_w, pop_h);
        return 1;
    }
    Beep(700, 80);
    ui_form_message(pop_x, pop_y, pop_w, pop_h, msg ? msg : "[PERINGATAN] Input tidak valid.");
    return 0;
}
static int ui_require_not_blank(const char *value, int pop_x, int pop_y, int pop_w, int pop_h, const char *field_label) {
    if (value && !is_blank(value)) {
        ui_form_message_clear(pop_x, pop_y, pop_w, pop_h);
        return 1;
    }
    Beep(700, 80);
    char msg[256];
    snprintf(msg, sizeof(msg), "[PERINGATAN] %s wajib diisi.", field_label ? field_label : "Field");
    ui_form_message(pop_x, pop_y, pop_w, pop_h, msg);
    return 0;
}

/*
   Layout base versi "fast":
   - Frame digambar per-baris (lebih cepat, lebih stabil, mengurangi flicker)
   - Ada margin konten agar tabel tidak menempel ke border
*/
static void bottom_panel_clear(void);

void draw_layout_base(int w, int h, const char* section_title) {
    int split_x = layout_split_x(w);
    int header_h = 7;

    /* Reset panel bawah setiap ganti halaman (akan disiapkan ulang oleh layar tabel) */
    bottom_panel_clear();

    /* Jika terminal terlalu kecil, hindari menggambar di luar buffer */
    if (h < 10 || w < 60) {
        cls();
        gotoXY(0, 0);
        printf("Terminal terlalu kecil. Perbesar jendela untuk melihat UI.");
        return;
    }

    /* Build & draw frame lines */
    char *line = (char*)malloc((size_t)w + 1);
    if (!line) return;
    line[w] = '\0';

    for (int y = 0; y <= h - 2; y++) {
        int is_hline = (y == 0 || y == header_h || y == h - 2);
        memset(line, is_hline ? '=' : ' ', (size_t)w);
        /* Jika horizontal line, gunakan joint '+' pada titik perpotongan supaya tidak terlihat putus */
        char vb = is_hline ? '+' : '|';
       /* outer border */
        if (w >= 2) { line[0] = vb; line[1] = vb; }
        if (w >= 2) { line[w - 2] = vb; line[w - 1] = vb; }

        /* splitter border */
        if (split_x >= 0 && split_x + 1 < w) {
            line[split_x] = vb;
            line[split_x + 1] = vb;
        }

        console_write_line(y, line, w);
    }

    /* Bersihkan baris paling bawah (kadang terminal menyisakan artefak) */
    memset(line, ' ', (size_t)w);
    console_write_line(h - 1, line, w);
    free(line);

    /* Content bounds */
    int content_left, content_right, usable_w;
    layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
    int content_w = usable_w;

    /* Header Title (ASCII art) */
    const char *title_lines[] = {
        " _______ _             ______                 _            _       ",
        "|__   __| |           |  ____|               | |          (_)      ",
        "   | |  | |__   ___   | |__ ___  _   _ _ __| |_ _ __ __ _ _ _ __ ",
        "   | |  | '_ \\ / _ \\  |  __/ _ \\| | | | '__| __| '__/ _` | | '_ \\",
        "   | |  | | | |  __/  | | | (_) | |_| | |  | |_| | | (_| | | | | |",
        "   |_|  |_| |_|\\___|  |_|  \\___/ \\__,_|_|   \\__|_|  \\__,_|_|_| |_|"
    };
    int title_h = 6;
    int title_w = 0;
    for (int i = 0; i < title_h; i++) {
        int lw = (int)strlen(title_lines[i]);
        if (lw > title_w) title_w = lw;
    }
    int title_x = center_within(content_left, content_right, title_w);
    int title_y = 1;
    for (int i = 0; i < title_h; i++) {
        int max_w = content_right - title_x + 1;
        print_clipped(title_x, title_y + i, max_w, title_lines[i]);
    }

    /* Sidebar Info Box (lebih rapi & profesional) */
    const Account *me = session_me();
    int sb_left = 2 + CONTENT_MARGIN;
    int sb_right = split_x - 1 - CONTENT_MARGIN;
    int sb_w = (sb_right >= sb_left) ? (sb_right - sb_left + 1) : 0;
    if (sb_w > 0) {
        /* Judul */
        print_clipped(sb_left, 2, sb_w, "Kelompok 4");
        print_clipped(sb_left, 3, sb_w, "Sistem Manajemen Kereta");

        char buf[128];
        snprintf(buf, sizeof(buf), "Akun : %s", me ? (me->email[0] ? me->email : "-") : "-");
        print_clipped(sb_left, 5, sb_w, buf);
        snprintf(buf, sizeof(buf), "Peran : %s", me ? role_to_label(me->role) : "-");
        print_clipped(sb_left, 6, sb_w, buf);

        /* ASCII Art Kereta (sidebar kiri - tengah)
           - Dibuat lebih kecil (scale down) agar tidak makan ruang
           - Posisi vertikal di tengah
           - Moncong tetap di pojok kiri sidebar (tidak melewati border)
           - Aman: auto-crop jika sempit */
        {
            const int art_h = TRAIN_ART_H;

            /* area sidebar yang aman (di bawah header dan di atas border) */
            int inner_top = header_h + 2;
            int inner_bottom = h - 4;
            int avail_h = inner_bottom - inner_top + 1;
            if (avail_h > 0 && sb_w > 0) {
                /* scale down: ambil 1 dari setiap 2 baris/kolom (lebih kecil & rapi) */
                const int y_step = 2;
                const int x_step = 2;

                int scaled_h = (art_h + y_step - 1) / y_step;
                int draw_h = (avail_h < scaled_h) ? avail_h : scaled_h;

                /* center secara vertikal di area sidebar */
                int start_scaled_line = (scaled_h > draw_h) ? (scaled_h - draw_h) / 2 : 0;
                int art_y = inner_top + (avail_h - draw_h) / 2;
                if (art_y < inner_top) art_y = inner_top;

                /* moncong di kiri, jangan melewati border */
                int art_x = sb_left;
                int max_out_w = sb_right - art_x + 1;
                if (max_out_w < 1) max_out_w = 1;

                char *out = (char*)malloc((size_t)max_out_w + 1);
                if (!out) return;

                for (int i = 0; i < draw_h; i++) {
                    int src_line = (start_scaled_line + i) * y_step;
                    if (src_line < 0) src_line = 0;
                    if (src_line >= art_h) src_line = art_h - 1;

                    const char *src = TRAIN_ART[src_line];
                    int len = (int)strlen(src);

                    int oi = 0;
                    for (int c = 0; c < len && oi < max_out_w; c += x_step) {
                        out[oi++] = src[c];
                    }
                    out[oi] = '\0';

                    /* print sudah otomatis crop ke sidebar */
                    print_clipped(art_x, art_y + i, max_out_w, out);
                }

                free(out);
            }
        }
    }

    /* Label Halaman / Breadcrumb */
    {
        char crumb[160];
        if (section_title && section_title[0]) {
            snprintf(crumb, sizeof(crumb), "Dasbor / %s", section_title);
        } else {
            snprintf(crumb, sizeof(crumb), "Dasbor");
        }
        /* gunakan padded agar sisa teks layar sebelumnya tidak nyangkut */
        print_padded(content_left, header_h + 1, content_w, "%s", crumb);
    }
}


/* ================== CRUD KARYAWAN ================== */
/* UI table-style seperti Kelola Akun:
   - UP/DOWN pilih baris
   - LEFT/RIGHT pindah halaman
   - ENTER detail
   - [A] tambah, [E] edit, [X] nonaktifkan (soft delete), [ESC] kembali
   - Data NONAKTIF tetap ditampilkan di tabel
*/

/* --- Bottom Panel Form/Detail (di bawah navigasi tabel) --- */
static int g_bottom_panel_x = -1;
static int g_bottom_panel_y = -1;
static int g_bottom_panel_w = 0;
static int g_bottom_panel_h = 0;

static void bottom_panel_clear(void) {
    g_bottom_panel_x = -1;
    g_bottom_panel_y = -1;
    g_bottom_panel_w = 0;
    g_bottom_panel_h = 0;
}

static void bottom_panel_set(int x, int y, int w, int h) {
    g_bottom_panel_x = x;
    g_bottom_panel_y = y;
    g_bottom_panel_w = w;
    g_bottom_panel_h = h;
}

/*
   PRINT CLIPPING (BORDER SAFE)
   Banyak kasus "border hilang/nembus" terjadi karena printf mencetak teks
   yang lebih panjang dari area panel, sehingga menimpa karakter border '|'.

   Solusi: clamp semua output printf berbasis posisi cursor terakhir (gotoXY).
   Jika cursor berada di area panel bawah, batasi hingga lebar panel.
   Jika tidak, batasi hingga area aman sebelum border luar.
*/
static int ui_printf_clamped(const char *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    int x = g_ui_cursor_x;
    int y = g_ui_cursor_y;
    int scr_w = get_screen_width();
    if (scr_w <= 0) scr_w = 120;

    /* default: sisakan border luar (FRAME_THICK kanan) */
    int right_limit = scr_w - FRAME_THICK - 1;
    if (right_limit < 0) right_limit = scr_w - 1;

    /* jika sedang di panel bawah, clamp ke batas panel */
    if (g_bottom_panel_x >= 0 && g_bottom_panel_w > 0 && g_bottom_panel_h > 0) {
        int panel_left = g_bottom_panel_x;
        int panel_right = g_bottom_panel_x + g_bottom_panel_w - 1;
        if (y >= g_bottom_panel_y && y <= g_bottom_panel_y + g_bottom_panel_h - 1 && x >= panel_left && x <= panel_right) {
            right_limit = panel_right;
        }
    }

    int max_w = right_limit - x + 1;
    if (max_w <= 0) return 0;

    /* Hindari loncat baris: stop di
 /
 */
    int len = 0;
    while (buf[len] && buf[len] != '\n' && buf[len] != '\r' && len < max_w) {
        len++;
    }

    if (len > 0) {
        fwrite(buf, 1, (size_t)len, stdout);
        /* update posisi cursor internal supaya clamp berikutnya akurat */
        g_ui_cursor_x += len;
    }
    return len;
}

/* aktifkan clipping untuk semua printf di file ini (setelah titik ini) */
#define printf ui_printf_clamped

/*
   Menyediakan area kotak untuk form CRUD/CRD & detail, ditempatkan
   di bawah baris Navigasi tabel.
   after_nav_y = Y terakhir dari baris navigasi/aksi tabel.
*/
static void ui_draw_box_frame(int x, int y, int w, int h) {
    if (w < 2 || h < 2) return;

    /* top border */
    gotoXY(x, y);
    putchar('+');
    ui_hline(x + 1, y, w - 2, '-');
    gotoXY(x + w - 1, y);
    putchar('+');

    /* sides */
    for (int yy = y + 1; yy <= y + h - 2; yy++) {
        gotoXY(x, yy);
        putchar('|');
        gotoXY(x + w - 1, yy);
        putchar('|');
    }

    /* bottom border */
    gotoXY(x, y + h - 1);
    putchar('+');
    ui_hline(x + 1, y + h - 1, w - 2, '-');
    gotoXY(x + w - 1, y + h - 1);
    putchar('+');
}

/*
   Menyediakan area kotak untuk form CRUD/CRD & detail, ditempatkan
   di bawah baris Navigasi tabel.
   after_nav_y = Y terakhir dari baris navigasi/aksi tabel.

   Catatan UI:
   - Kotak panel SELALU digambar penuh (top/side/bottom) agar border konsisten.
   - Semua isi form/detail menggambar DI DALAM kotak (tidak menimpa border).
*/
static void bottom_panel_prepare(int x, int after_nav_y, int w, int screen_h) {
    /*
      BORDER-SAFE PANEL:
      Panel bawah harus selalu berada di area konten (kanan sidebar)
      dan melebar penuh sampai batas aman (tanpa menabrak border luar).
      Ini mencegah kasus "garis box hilang/ketimpa" saat form/detail
      mencetak teks panjang.
    */
    {
        int scr_w = get_screen_width();
        if (scr_w <= 0) scr_w = 120;
        int split_x = layout_split_x(scr_w);
        int content_left, content_right, usable_w;
        layout_content_bounds(scr_w, split_x, &content_left, &content_right, &usable_w);

        /* paksa panel selalu full-width pada area konten */
        x = content_left;
        w = usable_w;

        /* jangan terlalu kecil (butuh border) */
        if (w < 10) {
            bottom_panel_clear();
            return;
        }
    }

    /* kotak panel dimulai tepat 1 baris di bawah navigasi */
    int box_y = after_nav_y + 1;
    int bottom_border_y = screen_h - 3; /* sisakan border luar */
    int box_h = bottom_border_y - box_y + 1;

    if (box_h < 8) {
        bottom_panel_clear();
        return;
    }

    /* gambar kotak panel */
    ui_draw_box_frame(x, box_y, w, box_h);

    /* set area dalam (tanpa border) */
    bottom_panel_set(x + 1, box_y + 1, w - 2, box_h - 2);
}


/*
   Popup lebar mengikuti area konten (menutup tabel di belakang), tinggi secukupnya.

   BUGFIX (border bawah hilang):
   Sebelumnya pop_h hanya di-clamp *secara lokal* di fungsi ini.
   Karena pop_h dikirim by-value, caller tetap memakai nilai pop_h awal untuk
   menghitung posisi teks (mis: pop_y + pop_h - 3). Saat panel bawah lebih
   pendek dari pop_h, teks bisa tercetak di luar area panel dan menimpa
   border bawah layout.

   Solusi: kembalikan nilai pop_h efektif lewat out_h.
*/
static void popup_content_clamped(int split_x, int pop_h, int *out_x, int *out_y, int *out_w, int *out_h) {
    int w = get_screen_width();  if (w <= 0) w = 120;
    int h = get_screen_height(); if (h <= 0) h = 30;

    /* Jika sedang berada di layar list tabel yang sudah menyiapkan panel bawah,
       pakai area panel itu agar popup CRUD/CRD muncul di bawah navigasi. */
    if (g_bottom_panel_x >= 0 && g_bottom_panel_w > 0) {
        int pop_w = g_bottom_panel_w;
        int pop_x = g_bottom_panel_x;
        int pop_y = g_bottom_panel_y;

        /*
           Paksa popup berada di panel bawah (di bawah navigasi) dan jangan naik ke atas.
           Jika tinggi popup melebihi panel, cukup terpotong oleh area clear (aman).
        */
        if (pop_h > g_bottom_panel_h) pop_h = g_bottom_panel_h;

        *out_x = pop_x;
        *out_y = pop_y;
        *out_w = pop_w;
        if (out_h) *out_h = pop_h;
        return;
    }

    /* Fallback: popup lebar mengikuti area konten (menutup tabel di belakang), tinggi secukupnya */
    int content_left, content_right, usable_w;
    layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);

    int pop_w = usable_w;
    int pop_x = content_left;
    int pop_y = 8; // di bawah header

    int min_y = 6;
    int max_y = (h - 3) - pop_h;
    if (pop_y < min_y) pop_y = min_y;
    if (pop_y > max_y) pop_y = max_y;
    if (pop_y < min_y) pop_y = min_y;

    *out_x = pop_x;
    *out_y = pop_y;
    *out_w = pop_w;
    if (out_h) *out_h = pop_h;
}

/*
   Setelah keluar dari layar lookup (mis: view_stasiun/view_kereta) menggunakan ESC,
   tampilan layar di belakang berubah total dan border/layout lama masih tertinggal.
   Untuk mencegah judul form "terpotong"/naik, kita redraw base layout sederhana.
   NOTE: base layout saja (tanpa redraw tabel) sudah cukup untuk menghilangkan artefak.
*/
static void ui_redraw_base_simple(const char *base_title) {
    int w = get_screen_width();  if (w <= 0) w = 140;
    int h = get_screen_height(); if (h <= 0) h = 30;
    cls();
    draw_layout_base(w, h, base_title);
}



#if 0 /* Modul Karyawan dihapus dari build + tidak dipanggil dari UI */
static void karyawan_print_hline(int x, int y,
                                int w_no, int w_nama, int w_email, int w_jabatan, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_email + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_jabatan + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}


static void karyawan_popup_detail(int split_x, int content_w, const Karyawan *k) {
    (void)content_w;

    /* Layout detail dibuat rapat dan konsisten dengan "Rincian Jadwal". */
    int pop_h = 12;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Karyawan");

    int x = pop_x + 3;
    int y = pop_y + 1;
    const int L = 12;

    gotoXY(x, y++); printf("%-*s : %s", L, "ID", k->id);
    gotoXY(x, y++); printf("%-*s : %s", L, "Nama", k->nama);
    gotoXY(x, y++); printf("%-*s : %s", L, "Surel", k->email);
    gotoXY(x, y++); printf("%-*s : %s", L, "Jabatan", k->jabatan);
    gotoXY(x, y++); printf("%-*s : %s", L, "Status", k->active ? "Aktif" : "Nonaktif");

    print_padded(pop_x + 3, pop_y + pop_h - 2, pop_w - 6, "Tekan tombol apa saja untuk kembali...");
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
    /* clip label supaya tidak tembus border */
    printf("%-*.*s : ", label_w, label_w, label ? label : "");

    /* clear area input supaya rapi (clamp agar tidak melewati border kanan) */
    int screen_w = get_screen_width();
    int safe_right = screen_w - FRAME_THICK - 1 - CONTENT_MARGIN;
    if (safe_right < 0) safe_right = screen_w - 1;
    int max_w = safe_right - x_input + 1;
    if (max_w < 0) max_w = 0;
    if (input_w > max_w) input_w = max_w;
    for (int i = 0; i < input_w; i++) putchar(' ');

    if (out_x_input) *out_x_input = x_input;
    if (out_y_input) *out_y_input = y;
}

static void karyawan_popup_add(int split_x, int content_w) {
    (void)content_w;
    int pop_h = 18;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Karyawan");
    ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    const int LABEL_W = 18;
    int INPUT_W = pop_w - (3 + LABEL_W + 3) - 4; /* margin kiri+label+" : "+margin kanan */
    if (INPUT_W < 10) INPUT_W = 10;
    int MAX_VIS = INPUT_W;
    if (MAX_VIS > 63) MAX_VIS = 63;

    char new_id[16];
    char nama[64], email[64], jabatan_opt[8];

    // ==== RENDER FORM SEKALIGUS (TAMPIL LANGSUNG) ====
    int x_id, y_id, x_nama, y_nama, x_email, y_email, x_jab, y_jab;

    form_row_draw(pop_x, pop_y, 2, LABEL_W, INPUT_W, "ID", &x_id, &y_id);
    gotoXY(x_id, y_id); printf("(dibuat sistem)");

    form_row_draw(pop_x, pop_y, 4, LABEL_W, INPUT_W, "Nama", &x_nama, &y_nama);

    form_row_draw(pop_x, pop_y, 6, LABEL_W, INPUT_W, "Surel", &x_email, &y_email);

    form_row_draw(pop_x, pop_y, 9, LABEL_W, INPUT_W, "Jabatan", &x_jab, &y_jab);

    const char *helpers[] = {
        "Surel: nama@domain.com (contoh: user@gmail.com / user@kai.id)",
        "Jabatan: 1=Data, 2=Transaksi, 3=Manager"
    };
    ui_draw_helpers_block(pop_x, pop_y, pop_w, pop_h, "Tips:", helpers, 2);


    // ==== INPUT (SETELAH FORM SUDAH KE-PRINT SEMUA) ====
    while (1) {
        gotoXY(x_nama, y_nama);
        input_person_name_filtered(nama, (int)sizeof(nama), MAX_VIS);
        if (nama[0] == 27) return;
        if (!ui_require_not_blank(nama, pop_x, pop_y, pop_w, pop_h, "Nama")) continue;
        break;
    }

    while (1) {
        gotoXY(x_email, y_email);
        input_email_filtered(email, (int)sizeof(email), MAX_VIS);
        if (email[0] == 27) return;
        if (!ui_require_not_blank(email, pop_x, pop_y, pop_w, pop_h, "Surel")) continue;
        if (!looks_like_email(email)) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Surel tidak valid (contoh: user@gmail.com).");
            continue;
        }
        break;
    }

    while (1) {
        gotoXY(x_jab, y_jab);
        input_choice_digits_filtered(jabatan_opt, (int)sizeof(jabatan_opt), "123", 0);
        if (jabatan_opt[0] == 27) return;
        if (!ui_require_not_blank(jabatan_opt, pop_x, pop_y, pop_w, pop_h, "Jabatan")) continue;
        break;
    }

    // ==== VALIDASI ====
    if (is_blank(nama) || is_blank(email) || is_blank(jabatan_opt)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 13);
        print_padded(pop_x + 3, pop_y + 13, pop_w - 6, "Semua field wajib diisi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    if (!looks_like_email(email)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 13);
        print_padded(pop_x + 3, pop_y + 13, pop_w - 6, "Surel tidak valid. Contoh: user@gmail.com / user@kai.id");
        gotoXY(pop_x + 3, pop_y + 14);
        print_padded(pop_x + 3, pop_y + 14, pop_w - 6, "Tekan tombol apa saja...");
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
        print_padded(pop_x + 3, pop_y + 13, pop_w - 6, "Pilihan jabatan tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    karyawan_create_auto(new_id, sizeof(new_id), nama, email, jabatan);

    gotoXY(pop_x + 3, pop_y + 13);
    printf("Berhasil tambah karyawan. ID: %s", new_id);
    gotoXY(pop_x + 3, pop_y + 14);
    print_padded(pop_x + 3, pop_y + 14, pop_w - 6, "Tekan tombol apa saja...");
    _getch();
}



static void karyawan_popup_edit(int split_x, int content_w, int idx) {
    (void)content_w;
    int pop_h = 20;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Karyawan (kosongkan untuk tetap)");
    ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    const int LABEL_W = 18;
    int INPUT_W = pop_w - (3 + LABEL_W + 3) - 4;
    if (INPUT_W < 10) INPUT_W = 10;
    int MAX_VIS = INPUT_W;
    if (MAX_VIS > 63) MAX_VIS = 63;

    char nama[64] = "";
    char email[64] = "";
    char jabatan_opt[8] = "";

    int x_id, y_id, x_nama, y_nama, x_email, y_email, x_jab, y_jab;

    form_row_draw(pop_x, pop_y, 2, LABEL_W, INPUT_W, "ID", &x_id, &y_id);
    gotoXY(x_id, y_id); printf("%s", g_karyawan[idx].id);

    form_row_draw(pop_x, pop_y, 4, LABEL_W, INPUT_W, "Nama", &x_nama, &y_nama);
    {
        char buf[180];
        snprintf(buf, sizeof(buf), "  Saat ini: %s", g_karyawan[idx].nama);
        print_clipped(pop_x + 3, pop_y + 5, pop_w - 6, buf);
    }

    form_row_draw(pop_x, pop_y, 7, LABEL_W, INPUT_W, "Surel", &x_email, &y_email);
    {
        char buf[180];
        snprintf(buf, sizeof(buf), "  Saat ini: %s", g_karyawan[idx].email);
        print_clipped(pop_x + 3, pop_y + 8, pop_w - 6, buf);
    }

    form_row_draw(pop_x, pop_y, 11, LABEL_W, INPUT_W, "Jabatan", &x_jab, &y_jab);

    {
        const char *helpers[] = {
            "Surel: nama@domain.com (contoh: user@gmail.com / user@kai.id)",
            "Jabatan: 1=Data, 2=Transaksi, 3=Manager"
        };
        ui_draw_helpers_block(pop_x, pop_y, pop_w, pop_h, "Tips:", helpers, 2);
    }


    gotoXY(x_nama, y_nama);
    input_person_name_filtered(nama, (int)sizeof(nama), MAX_VIS);
    if (nama[0] == 27) return;

    gotoXY(x_email, y_email);
    input_email_filtered(email, (int)sizeof(email), MAX_VIS);
    if (email[0] == 27) return;

    gotoXY(x_jab, y_jab);
    input_choice_digits_filtered(jabatan_opt, (int)sizeof(jabatan_opt), "123", 1);
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
            print_padded(pop_x + 3, pop_y + 14, pop_w - 6, "Pilihan jabatan tidak valid. Tekan tombol apa saja...");
            _getch();
            return;
        }
    }

    if (!looks_like_email(new_email)) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + pop_h - 5);
        print_padded(pop_x + 3, pop_y + pop_h - 5, pop_w - 6, "Surel tidak valid. Contoh: user@gmail.com / user@kai.id");
        gotoXY(pop_x + 3, pop_y + pop_h - 4);
        print_padded(pop_x + 3, pop_y + pop_h - 4, pop_w - 6, "Tekan tombol apa saja...");
        _getch();
        return;
    }

    karyawan_update(idx, new_nama, new_email, jabatan_final);

    /* Tampilkan pesan di area aman agar tidak overwrite action bar */
    ui_form_message(pop_x, pop_y, pop_w, pop_h, "Berhasil memperbarui. Tekan tombol apa saja...");
    _getch();
}

/* Soft delete: active=0, tapi tetap ditampilkan */
static void karyawan_popup_delete_soft(int split_x, int content_w, int idx) {
    (void)content_w;
    int pop_h = 14;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Karyawan");
    ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    {
        char buf[220];
        snprintf(buf, sizeof(buf), "Ubah status karyawan %s (%s) menjadi NONAKTIF?", g_karyawan[idx].id, g_karyawan[idx].nama);
        print_clipped(pop_x + 3, pop_y + 3, pop_w - 6, buf);
    }

    gotoXY(pop_x + 3, pop_y + 6);
    print_padded(pop_x + 3, pop_y + 6, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            karyawan_delete(idx);
            gotoXY(pop_x + 3, pop_y + 8);
            print_padded(pop_x + 3, pop_y + 8, pop_w - 6, "Data berhasil dinonaktifkan. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

void view_karyawan() {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO = 3;
    int W_ID = 8;
    int W_NAMA = 18;
    int W_EMAIL = 22;
    int W_JAB = 14;
    int W_STATUS = 10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
        W_NO = 3; W_NAMA = 22; W_EMAIL = 22; W_JAB = 16; W_STATUS = 12;
        int col_w[] = { W_NO, W_NAMA, W_EMAIL, W_JAB, W_STATUS };
        const int min_w[] = { 3, 6, 10, 8, 7 };
        fit_columns(col_w, min_w, 5, usable_w);
        W_NO = col_w[0]; W_NAMA = col_w[1]; W_EMAIL = col_w[2]; W_JAB = col_w[3]; W_STATUS = col_w[4];


        int idx_map[MAX_RECORDS];
        int total = build_karyawan_indexes_all(idx_map, MAX_RECORDS);
        if (total > 1) qsort(idx_map, (size_t)total, sizeof(int), karyawan_cmp_idx);

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

        int table_w = table_total_width(col_w, 5);
        int table_x = content_left;

        int table_y = 10;

        karyawan_print_hline(table_x, table_y, W_NO, W_NAMA, W_EMAIL, W_JAB, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_NAMA, "Nama", W_EMAIL, "Surel", W_JAB, "Jabatan", W_STATUS, "Status");
        karyawan_print_hline(table_x, table_y + 2, W_NO, W_NAMA, W_EMAIL, W_JAB, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int n = start + r;
                int idx = idx_map[n];

                const char *status = g_karyawan[idx].active ? "Aktif" : "Nonaktif";

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*s |",
                       W_NO, n + 1,
                       W_NAMA, W_NAMA, g_karyawan[idx].nama,
                       W_EMAIL, W_EMAIL, g_karyawan[idx].email,
                       W_JAB, W_JAB, g_karyawan[idx].jabatan,
                       W_STATUS, status);
            } else {
                table_print_blank_row(table_x, y, col_w, 5);
            }

            set_highlight(0);
        }

        karyawan_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_NAMA, W_EMAIL, W_JAB, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        print_padded(table_x, footer_y, table_w,
                     "Data: %d   Aktif: %d   Nonaktif: %d   Halaman: %d/%d",
                     total, aktif, nonaktif, page + 1, total_pages);
        print_padded(table_x, footer_y + 1, table_w,
                     "Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian");
        print_padded(table_x, footer_y + 2, table_w,
                     "Aksi: [A] Tambah   [E] Edit   [X] Nonaktifkan   [ESC] Kembali");

        /* Siapkan panel bawah untuk form CRUD/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, footer_y + 2, table_w, h);


        int ch = _getch();
        if (ch == 27) return; /* ESC */

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


#endif /* 0 */

/* ================== CRUD AKUN (SUPERADMIN) ================== */

/* Table style seperti screenshot:
   - UP/DOWN pilih row
   - LEFT/RIGHT pindah halaman
   - ENTER detail
   - [A] tambah, [E] edit, [X] hapus, [ESC] kembali
*/

static void akun_print_hline(int x, int y, int w_no, int w_email, int w_nama, int w_role, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_email + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_role + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}


static void akun_popup_detail(int split_x, int content_w, const Account *a) {
    (void)content_w;

    /* Layout detail dibuat rapat dan konsisten dengan "Rincian Jadwal". */
    int pop_h = 13;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Akun");

    int x = pop_x + 3;
    int y = pop_y + 1;
    const int L = 14;

    gotoXY(x, y++); printf("%-*s : %s", L, "Surel", a->email);
    gotoXY(x, y++); printf("%-*s : %s", L, "Nama", a->nama);
    gotoXY(x, y++); printf("%-*s : %s", L, "ID Karyawan", a->id_karyawan);
    gotoXY(x, y++); printf("%-*s : %s", L, "Peran", role_to_label(a->role));
    gotoXY(x, y++); printf("%-*s : %s", L, "Status", a->active ? "Aktif" : "Nonaktif");
    gotoXY(x, y++); printf("%-*s : %s", L, "Sandi", "********");

    print_padded(pop_x + 3, pop_y + pop_h - 2, pop_w - 6, "Tekan tombol apa saja untuk kembali...");
    _getch();
}

static void akun_popup_add(int split_x, int content_w) {
    (void)content_w;
    int pop_h = 22;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Akun Karyawan");
    ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    const int LABEL_W = 16;
    int INPUT_W = pop_w - (3 + LABEL_W + 3) - 4;
    if (INPUT_W < 10) INPUT_W = 10;
    int MAX_VIS = INPUT_W;
    if (MAX_VIS > 63) MAX_VIS = 63;

    char email[64], nama[64], password[64], rolebuf[8];

    int x_email, y_email, x_nama, y_nama, x_pass, y_pass, x_role, y_role;
    form_row_draw(pop_x, pop_y, 2, LABEL_W, INPUT_W, "Surel", &x_email, &y_email);
    form_row_draw(pop_x, pop_y, 4, LABEL_W, INPUT_W, "Nama", &x_nama, &y_nama);
    form_row_draw(pop_x, pop_y, 6, LABEL_W, INPUT_W, "Sandi", &x_pass, &y_pass);
    form_row_draw(pop_x, pop_y, 8, LABEL_W, INPUT_W, "Peran", &x_role, &y_role);


    const char *helpers[] = {
        "Peran: 1=Data, 2=Transaksi, 3=Manager",
        "Login hanya domain @kai.id. Surel tanpa @ akan dianggap @kai.id",
        "Sandi: TAB tampil/sembunyi"
    };
    ui_draw_helpers_block(pop_x, pop_y, pop_w, pop_h, "Tips:", helpers, 3);



    while (1) {
        gotoXY(x_email, y_email);
        input_email_filtered(email, (int)sizeof(email), MAX_VIS);
        if (email[0] == 27) return;
        if (!ui_require_not_blank(email, pop_x, pop_y, pop_w, pop_h, "Surel")) continue;
        break;
    }

    while (1) {
        gotoXY(x_nama, y_nama);
        input_person_name_filtered(nama, (int)sizeof(nama), MAX_VIS);
        if (nama[0] == 27) return;
        if (!ui_require_not_blank(nama, pop_x, pop_y, pop_w, pop_h, "Nama")) continue;
        break;
    }

    while (1) {
        gotoXY(x_pass, y_pass);
        int pw_w = INPUT_W;
        if (pw_w > 30) pw_w = 30;
        input_password_masked(password, 63, x_pass, y_pass, pw_w);
        if (password[0] == 27) return;
        if (!ui_require_not_blank(password, pop_x, pop_y, pop_w, pop_h, "Sandi")) continue;
        break;
    }

    while (1) {
        gotoXY(x_role, y_role);
        input_choice_digits_filtered(rolebuf, (int)sizeof(rolebuf), "123", 0);
        if (rolebuf[0] == 27) return;
        if (!ui_require_not_blank(rolebuf, pop_x, pop_y, pop_w, pop_h, "Peran")) continue;
        break;
    }

    if (0) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 12);
        print_padded(pop_x + 3, pop_y + 12, pop_w - 6, "Semua field wajib diisi. Tekan tombol apa saja...");
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
        print_padded(pop_x + 3, pop_y + 12, pop_w - 6, "Peran tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    int ok = akun_create(email, nama, password, role);

    gotoXY(pop_x + 3, pop_y + 12);
    if (ok) printf("Berhasil menambah akun. Tekan tombol apa saja...");
    else    printf("Gagal menambah akun (duplikat / input tidak valid / domain bukan kai.id). Tekan tombol apa saja...");

    _getch();
}

static void akun_popup_edit(int split_x, int content_w, int idx) {
    (void)content_w;
    int pop_h = 26;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Akun");
    ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    int is_admin = (strcmp(g_accounts[idx].email, "admin@kai.id") == 0);

    /* Ringkas info akun saat ini */
    {
        char buf[240];
        snprintf(buf, sizeof(buf), "Surel: %s  |  Nama: %s  |  Peran: %s",
                 g_accounts[idx].email,
                 g_accounts[idx].nama,
                 role_to_label(g_accounts[idx].role));
        print_clipped(pop_x + 3, pop_y + 2, pop_w - 6, buf);
    }

    const int LABEL_W = 18;
    int INPUT_W = pop_w - (3 + LABEL_W + 3) - 4;
    if (INPUT_W < 10) INPUT_W = 10;
    int MAX_VIS = INPUT_W;
    if (MAX_VIS > 63) MAX_VIS = 63;

    char new_email[64] = "";
    char new_nama[64]  = "";
    char new_pass[64]  = "";
    char rolebuf[8]    = "";

    int x_email = 0, y_email = 0, x_nama = 0, y_nama = 0, x_pass = 0, y_pass = 0, x_role = 0, y_role = 0;
    int row = 5;

    if (!is_admin) {
        form_row_draw(pop_x, pop_y, row, LABEL_W, INPUT_W, "Surel Baru", &x_email, &y_email);
        row += 2;
        form_row_draw(pop_x, pop_y, row, LABEL_W, INPUT_W, "Nama Baru", &x_nama, &y_nama);
        row += 2;
    } else {
        row += 4;
    }

    form_row_draw(pop_x, pop_y, row, LABEL_W, INPUT_W, "Sandi Baru", &x_pass, &y_pass);
    row += 2;

    if (!is_admin) {
        form_row_draw(pop_x, pop_y, row, LABEL_W, INPUT_W, "Peran Baru", &x_role, &y_role);
        row += 2;
    }



    {
        const char *helpers[3];
        int hn = 0;
        if (is_admin) {
            helpers[hn++] = "Akun admin hanya bisa ganti password";
        } else {
            helpers[hn++] = "Peran: 1=Data, 2=Transaksi, 3=Manager";
            helpers[hn++] = "Login hanya domain @kai.id (surel tanpa @ dianggap @kai.id)";
            helpers[hn++] = "Kosongkan field jika tidak diganti";
        }
        ui_draw_helpers_block_below(pop_x, pop_y, pop_w, pop_h, y_role + 2, "Tips:", helpers, hn);
    }

    /* INPUT */
    if (!is_admin) {
        gotoXY(x_email, y_email);
        input_email_filtered(new_email, (int)sizeof(new_email), MAX_VIS);
        if (new_email[0] == 27) return;

        gotoXY(x_nama, y_nama);
        input_person_name_filtered(new_nama, (int)sizeof(new_nama), MAX_VIS);
        if (new_nama[0] == 27) return;
    }

    gotoXY(x_pass, y_pass);
    {
        int pw_w = INPUT_W;
        if (pw_w > 30) pw_w = 30;
        input_password_masked(new_pass, 63, x_pass, y_pass, pw_w);
    }
    if (new_pass[0] == 27) return;

    if (!is_admin) {
        gotoXY(x_role, y_role);
        input_choice_digits_filtered(rolebuf, (int)sizeof(rolebuf), "123", 1);
        if (rolebuf[0] == 27) return;
    }

    int any_change = 0;
    int ok_all = 1;

    if (!is_admin && !is_blank(new_email)) {
        if (!akun_update_email(idx, new_email)) ok_all = 0;
        else any_change = 1;
    }

    if (!is_admin && !is_blank(new_nama)) {
        if (!akun_update_nama(idx, new_nama)) ok_all = 0;
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

    {
        char msg[200];
        if (!any_change) snprintf(msg, sizeof(msg), "Tidak ada perubahan. Tekan tombol apa saja...");
        else if (ok_all) snprintf(msg, sizeof(msg), "Berhasil memperbarui akun. Tekan tombol apa saja...");
        else             snprintf(msg, sizeof(msg), "Pembaruan gagal (input tidak valid/duplikat/role salah). Tekan tombol apa saja...");
        ui_form_message(pop_x, pop_y, pop_w, pop_h, msg);
    }

    _getch();
}

static void akun_popup_delete(int split_x, int content_w, int idx) {
    (void)content_w;
    int pop_h = 14;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Hapus Akun");
    ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    if (strcmp(g_accounts[idx].email, "admin@kai.id") == 0) {
        Beep(500, 200);
        gotoXY(pop_x + 3, pop_y + 3);
        print_padded(pop_x + 3, pop_y + 3, pop_w - 6, "Akun admin tidak bisa dihapus. Tekan tombol apa saja...");
        _getch();
        return;
    }

    {
        char buf[220];
        snprintf(buf, sizeof(buf), "Hapus akun: %s (%s)?", g_accounts[idx].email, role_to_label(g_accounts[idx].role));
        print_clipped(pop_x + 3, pop_y + 3, pop_w - 6, buf);
    }

    gotoXY(pop_x + 3, pop_y + 6);
    print_padded(pop_x + 3, pop_y + 6, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            akun_delete(idx);
            gotoXY(pop_x + 3, pop_y + 8);
            print_padded(pop_x + 3, pop_y + 8, pop_w - 6, "Berhasil dihapus. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

static void view_akun() {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO = 3;
    int W_EMAIL = 24;
    int W_NAMA = 18;
    int W_ROLE = 18;
    int W_STATUS = 10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
        W_NO = 3; W_EMAIL = 26; W_NAMA = 22; W_ROLE = 14; W_STATUS = 10;
        int col_w[] = { W_NO, W_EMAIL, W_NAMA, W_ROLE, W_STATUS };
        const int min_w[] = { 3, 12, 8, 10, 7 };
        fit_columns(col_w, min_w, 5, usable_w);
        W_NO = col_w[0]; W_EMAIL = col_w[1]; W_NAMA = col_w[2]; W_ROLE = col_w[3]; W_STATUS = col_w[4];


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

        int table_w = table_total_width(col_w, 5);
        int table_x = content_left;

        int table_y = 10;

        akun_print_hline(table_x, table_y, W_NO, W_EMAIL, W_NAMA, W_ROLE, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_EMAIL, "Surel", W_NAMA, "Nama", W_ROLE, "Peran", W_STATUS, "Status");
        akun_print_hline(table_x, table_y + 2, W_NO, W_EMAIL, W_NAMA, W_ROLE, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int n = start + r;
                int idx = active_idx[n];

                const char *status = g_accounts[idx].active ? "Aktif" : "Nonaktif";

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*s | %-*s |",
                       W_NO, n + 1,
                       W_EMAIL, W_EMAIL, g_accounts[idx].email,
                       W_NAMA, W_NAMA, g_accounts[idx].nama,
                       W_ROLE, role_to_label(g_accounts[idx].role),
                       W_STATUS, status);
            } else {
                table_print_blank_row(table_x, y, col_w, 5);
            }

            set_highlight(0);
        }

        akun_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_EMAIL, W_NAMA, W_ROLE, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        int aktif = 0, nonaktif = 0;
        for (int i = 0; i < total; i++) {
            if (g_accounts[active_idx[i]].active) aktif++;
            else nonaktif++;
        }
        print_padded(table_x, footer_y, table_w,
                     "Data: %d   Aktif: %d   Nonaktif: %d   Halaman: %d/%d",
                     total, aktif, nonaktif, page + 1, total_pages);
        print_padded(table_x, footer_y + 1, table_w,
                     "Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian");
        print_padded(table_x, footer_y + 2, table_w,
                     "Aksi: [A] Tambah  [E] Edit  [X] Hapus  [ESC] Kembali");

        /* Siapkan panel bawah untuk form CRUD/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, footer_y + 2, table_w, h);

        int ch = _getch();
        if (ch == 27) return; /* ESC */

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

    int dd = (s[0]-'0')*10 + (s[1]-'0');
    int mm = (s[3]-'0')*10 + (s[4]-'0');
    int yy = (s[6]-'0')*1000 + (s[7]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0');
    if (yy < 1900 || yy > 2100) return 0;
    if (mm < 1 || mm > 12) return 0;

    int maxd = 31;
    if (mm == 4 || mm == 6 || mm == 9 || mm == 11) maxd = 30;
    else if (mm == 2) {
        int leap = ((yy % 4 == 0 && yy % 100 != 0) || (yy % 400 == 0));
        maxd = leap ? 29 : 28;
    }
    if (dd < 1 || dd > maxd) return 0;

    return 1;
}

/* Nama orang: huruf + spasi + ('-.) ; tidak boleh angka/simbol lain */
static int looks_like_person_name(const char *s) {
    if (!s || !*s) return 0;
    int has_alpha = 0;
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        unsigned char c = *p;
        if (c == ' ' || c == '\t') continue;
        if (c >= '0' && c <= '9') return 0;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) { has_alpha = 1; continue; }
        if (c == '\'' || c == '-' || c == '.') continue;
        return 0;
    }
    return has_alpha;
}

/* ---------- PENUMPANG ---------- */

static void penumpang_print_hline(int x, int y,
                                  int w_no, int w_nama, int w_email,
                                  int w_telp, int w_lahir, int w_jk, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_email + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_telp + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_lahir + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_jk + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}

static void penumpang_popup_detail(int split_x, int content_w, const Penumpang *p) {
    /* Rincian (READ): rapat & konsisten seperti rincian jadwal */
    int pop_w = 0, pop_h = 18;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Penumpang");

    int x = pop_x + 3;
    int y = pop_y + 1;

    gotoXY(x, y++); printf("ID             : %s", p->id);
    gotoXY(x, y++); printf("Nama           : %s", p->nama);
    gotoXY(x, y++); printf("Surel          : %s", p->email);
    gotoXY(x, y++); printf("No. Telp       : %s", p->no_telp);
    gotoXY(x, y++); printf("Tanggal Lahir  : %s", p->tanggal_lahir);
    gotoXY(x, y++); printf("Jenis Kelamin  : %s", p->jenis_kelamin);
    gotoXY(x, y++); printf("Status         : %s", p->active ? "Aktif" : "Nonaktif");

    print_padded(pop_x + 3, pop_y + pop_h - 3, pop_w - 6, "Tekan tombol apa saja...");
    _getch();
}

static void penumpang_popup_add(int split_x, int content_w) {
    int pop_w = 0, pop_h = 20;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Penumpang");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    char nama[64], email[64], telp[24], lahir[16], jk[8];

    const int LABEL_W = 30;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: ", LABEL_W, "Nama");
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "Surel");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "No. Telp");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "Tanggal Lahir (DD-MM-YYYY)");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Jenis Kelamin [L/P]");
    gotoXY(label_x, pop_y + 11); printf("%-*s  %s", LABEL_W, "", "Laki laki (L) / Perempuan (P)");

    while (1) {
        gotoXY(field_x, pop_y + 2);
        input_person_name_filtered(nama, (int)sizeof(nama), 63);
        if (nama[0] == 27) return;
        if (!ui_require_not_blank(nama, pop_x, pop_y, pop_w, pop_h, "Nama")) continue;
        if (!ui_require_valid(looks_like_person_name(nama), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Nama tidak valid (hanya huruf & spasi, tanpa angka/simbol).")) {
            continue;
        }
        break;
    }

    while (1) {
        gotoXY(field_x, pop_y + 4);
        input_email_filtered(email, (int)sizeof(email), 63);
        if (email[0] == 27) return;
        if (!ui_require_not_blank(email, pop_x, pop_y, pop_w, pop_h, "Surel")) continue;
        if (!ui_require_valid(looks_like_email(email), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Surel tidak valid (contoh: nama@domain.com).")) {
            continue;
        }
        break;
    }

    
	while (1) {
	    gotoXY(field_x, pop_y + 6);
	    /* input_digits() menggambar box sebesar (max_len-1). Untuk 12 digit, pass 13. */
	    input_digits(telp, 13);
    if (telp[0] == 27) return;
    if (!ui_require_not_blank(telp, pop_x, pop_y, pop_w, pop_h, "No. Telp")) continue;
	    if (!ui_require_valid(looks_like_phone_08_len12(telp), pop_x, pop_y, pop_w, pop_h,
	                          "[PERINGATAN] No. Telp harus 12 digit dan diawali 08 (contoh: 081234567890).")) {
        continue;
    }
    break;
}
while (1) {
        gotoXY(field_x, pop_y + 8);
        input_date_ddmmyyyy_filtered(lahir, (int)sizeof(lahir));
        if (lahir[0] == 27) return;
        if (!ui_require_not_blank(lahir, pop_x, pop_y, pop_w, pop_h, "Tanggal Lahir")) continue;
        if (!ui_require_valid(looks_like_date_ddmmyyyy(lahir), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Tanggal lahir tidak valid. Gunakan DD-MM-YYYY (contoh: 01-01-2000).")) {
            continue;
        }
        break;
    }

    while (1) {
        gotoXY(field_x, pop_y + 10);
        input_gender_lp_filtered(jk, (int)sizeof(jk));
        if (jk[0] == 27) return;
        if (!ui_require_not_blank(jk, pop_x, pop_y, pop_w, pop_h, "Jenis Kelamin")) continue;
        if (!ui_require_valid(((jk[0] == 'L' || jk[0] == 'l' || jk[0] == 'P' || jk[0] == 'p') && jk[1] == '\0'),
                              pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Jenis kelamin harus L atau P.")) {
            continue;
        }
        break;
    }

    char id_new[16];
    penumpang_create_auto(id_new, sizeof(id_new), nama, email, telp, lahir,
                        (jk[0] == 'L' || jk[0] == 'l') ? "Laki laki" :
                        (jk[0] == 'P' || jk[0] == 'p') ? "Perempuan" : jk);

    gotoXY(pop_x + 3, pop_y + 14);
    printf("Berhasil tambah. ID: %s. Tekan tombol apa saja...", id_new);
    _getch();
}

static void penumpang_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 0, pop_h = 22;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Penumpang (kosongkan untuk tetap)");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    char nama[64], email[64], telp[24], lahir[16], jk[8];

    const int LABEL_W = 30;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: %s", LABEL_W, "ID (tidak bisa diubah)", g_penumpang[idx].id);
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "Nama");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "Surel");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "No. Telp");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Tanggal Lahir (DD-MM-YYYY)");
    gotoXY(label_x, pop_y + 12); printf("%-*s: ", LABEL_W, "Jenis Kelamin [L/P]");
    gotoXY(label_x, pop_y + 13); printf("%-*s  %s", LABEL_W, "", "Laki laki (L) / Perempuan (P)");

    /* Nama (opsional) */
    while (1) {
        gotoXY(field_x, pop_y + 4);
        input_person_name_filtered(nama, (int)sizeof(nama), 63);
        if (nama[0] == 27) return;
        if (is_blank(nama)) break;
        if (!ui_require_valid(looks_like_person_name(nama), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Nama tidak valid (hanya huruf & spasi, tanpa angka/simbol).")) {
            continue;
        }
        break;
    }

    /* Surel (opsional) */
    while (1) {
        gotoXY(field_x, pop_y + 6);
        input_email_filtered(email, (int)sizeof(email), 63);
        if (email[0] == 27) return;
        if (is_blank(email)) break;
        if (!ui_require_valid(looks_like_email(email), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Surel tidak valid (contoh: nama@domain.com).")) {
            continue;
        }
        break;
    }

    
/* No. Telp (opsional) */
    while (1) {
        gotoXY(field_x, pop_y + 8);
        /* input_digits() menggambar box sebesar (max_len-1). Untuk 12 digit, pass 13. */
        input_digits(telp, 13);
        if (telp[0] == 27) return;
        if (is_blank(telp)) break;
        if (!ui_require_valid(looks_like_phone_08_len12(telp), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] No. Telp harus 12 digit dan diawali 08 (contoh: 081234567890).")) {
            continue;
        }
        break;
    }

/* Tanggal Lahir (opsional) */
    while (1) {
        gotoXY(field_x, pop_y + 10);
        input_date_ddmmyyyy_filtered(lahir, (int)sizeof(lahir));
        if (lahir[0] == 27) return;
        if (is_blank(lahir)) break;
        if (!ui_require_valid(looks_like_date_ddmmyyyy(lahir), pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Tanggal lahir tidak valid. Gunakan DD-MM-YYYY (contoh: 01-01-2000).")) {
            continue;
        }
        break;
    }

    /* Jenis Kelamin (opsional) */
    while (1) {
        gotoXY(field_x, pop_y + 12);
        input_gender_lp_filtered(jk, (int)sizeof(jk));
        if (jk[0] == 27) return;
        if (is_blank(jk)) break;
        if (!ui_require_valid(((jk[0] == 'L' || jk[0] == 'l' || jk[0] == 'P' || jk[0] == 'p') && jk[1] == '\0'),
                              pop_x, pop_y, pop_w, pop_h,
                              "[PERINGATAN] Jenis kelamin harus L atau P.")) {
            continue;
        }
        break;
    }

    const char *new_nama = is_blank(nama) ? g_penumpang[idx].nama : nama;
    const char *new_email = is_blank(email) ? g_penumpang[idx].email : email;
    const char *new_telp = is_blank(telp) ? g_penumpang[idx].no_telp : telp;
    const char *new_lahir = is_blank(lahir) ? g_penumpang[idx].tanggal_lahir : lahir;

    char jk_final[16];
    snprintf(jk_final, sizeof(jk_final), "%s", g_penumpang[idx].jenis_kelamin);
    if (!is_blank(jk)) {
        snprintf(jk_final, sizeof(jk_final), "%s",
                 (jk[0] == 'L' || jk[0] == 'l') ? "Laki laki" :
                 (jk[0] == 'P' || jk[0] == 'p') ? "Perempuan" : jk);
    }

    /* Validasi format dilakukan saat input untuk field yang diubah.
       Saat kosong (tetap), kita tidak memaksa validasi ulang data lama agar edit tetap bisa dilakukan. */
    if (!is_blank(nama) && !looks_like_person_name(new_nama)) {
        ui_require_valid(0, pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Nama tidak valid (hanya huruf & spasi, tanpa angka/simbol).");
        return;
    }
    if (!is_blank(lahir) && !looks_like_date_ddmmyyyy(new_lahir)) {
        ui_require_valid(0, pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Tanggal lahir tidak valid. Gunakan DD-MM-YYYY (contoh: 01-01-2000).");
        return;
    }
    if (!is_blank(telp) && !is_digits_only(new_telp)) {
        ui_require_valid(0, pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] No. telp harus angka saja.");
        return;
    }

    penumpang_update(idx, new_nama, new_email, new_telp, new_lahir, jk_final);

    /* Tampilkan pesan di area aman agar tidak overwrite action bar */
    ui_form_message(pop_x, pop_y, pop_w, pop_h, "Berhasil memperbarui. Tekan tombol apa saja...");
    _getch();
}

static void penumpang_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 0, pop_h = 14;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Penumpang");
ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Ubah status penumpang %s (%s) menjadi NONAKTIF?", g_penumpang[idx].id, g_penumpang[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    print_padded(pop_x + 3, pop_y + 6, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            penumpang_delete(idx);
            gotoXY(pop_x + 3, pop_y + 8);
            print_padded(pop_x + 3, pop_y + 8, pop_w - 6, "Data berhasil dinonaktifkan. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

static void view_penumpang() {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO = 3;
    int W_ID = 8;
    int W_NAMA = 18;
    int W_EMAIL = 22;
    int W_TELP = 12;
    int W_LAHIR = 10;
    int W_JK = 2;
    int W_STATUS = 10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);

        /* Responsive sizing (full kolom, tapi tetap muat di terminal fullscreen) */
        /* reset ukuran kolom */
        W_NO = 3; W_NAMA = 22; W_EMAIL = 22; W_TELP = 14; W_LAHIR = 10; W_JK = 14; W_STATUS = 10;
        int col_w[] = { W_NO, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS };
        const int min_w[] = { 3, 6, 10, 10, 8, 10, 7 };
        fit_columns(col_w, min_w, 7, usable_w);
        W_NO = col_w[0]; W_NAMA = col_w[1]; W_EMAIL = col_w[2]; W_TELP = col_w[3]; W_LAHIR = col_w[4]; W_JK = col_w[5]; W_STATUS = col_w[6];


        int total = g_penumpangCount;
        int idxs[MAX_RECORDS];
        for (int i = 0; i < total; i++) idxs[i] = i;
        if (total > 1) qsort(idxs, (size_t)total, sizeof(int), penumpang_cmp_idx);

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

        int table_w = table_total_width(col_w, 7);
        int table_x = content_left;

        int table_y = 10;

        penumpang_print_hline(table_x, table_y, W_NO, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No",
               W_NAMA, "Nama",
               W_EMAIL, "Surel",
               W_TELP, "Telp",
               W_LAHIR, "Lahir",
               W_JK, "Jenis Kelamin",
               W_STATUS, "Status");
        penumpang_print_hline(table_x, table_y + 2, W_NO, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int pos = start + r;
                int n = idxs[pos];
                const char *status = g_penumpang[n].active ? "Aktif" : "Nonaktif";

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*s |",
                       W_NO, pos + 1,
                       W_NAMA, W_NAMA, g_penumpang[n].nama,
                       W_EMAIL, W_EMAIL, g_penumpang[n].email,
                       W_TELP, W_TELP, g_penumpang[n].no_telp,
                       W_LAHIR, W_LAHIR, g_penumpang[n].tanggal_lahir,
                       W_JK, W_JK, g_penumpang[n].jenis_kelamin,
                       W_STATUS, status);
            } else {
                table_print_blank_row(table_x, y, col_w, 6);
            }

            set_highlight(0);
        }

        penumpang_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_NAMA, W_EMAIL, W_TELP, W_LAHIR, W_JK, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        print_padded(table_x, footer_y, table_w,
                     "Data: %d  (Aktif: %d | Nonaktif: %d)  |  Halaman: %d/%d",
                     total, aktif, nonaktif, page + 1, total_pages);
        print_padded(table_x, footer_y + 1, table_w,
                     "Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian");
        print_padded(table_x, footer_y + 2, table_w,
                     "Aksi: [A] Tambah  [E] Edit  [X] Nonaktifkan  [ESC] Kembali");

        /* Siapkan panel bawah untuk form CRUD/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, footer_y + 2, table_w, h);

        int ch = _getch();
        if (ch == 27) return; /* ESC */

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
            penumpang_popup_detail(split_x, content_w, &g_penumpang[idxs[start + selected]]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idxs[start + selected];
            if (!g_penumpang[idx].active) { Beep(800, 60); continue; }
            penumpang_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idxs[start + selected];
            if (!g_penumpang[idx].active) { Beep(800, 60); continue; }
            penumpang_popup_delete_soft(split_x, content_w, idx);
            continue;
        }
    }
}

/* ---------- STASIUN ---------- */

static void stasiun_print_hline(int x, int y,
                                int w_no, int w_kode, int w_nama,
                                int w_mdpl, int w_kota, int w_alamat, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kode + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_mdpl + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kota + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_alamat + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}



static void stasiun_popup_detail(int split_x, int content_w, const Stasiun *s) {
    (void)content_w;

    /* Layout detail dibuat rapat dan konsisten dengan "Rincian Jadwal". */
    int pop_h = 14;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Stasiun");

    int x = pop_x + 3;
    int y = pop_y + 1;
    const int L = 12;

    const char *status_ops = "Aktif";
    if (!s->active) status_ops = "Nonaktif";
    else if (strcmp(s->status, "Perawatan") == 0) status_ops = "Perawatan";

    gotoXY(x, y++); printf("%-*s : %s", L, "ID", s->id);
    gotoXY(x, y++); printf("%-*s : %s", L, "Kode", s->kode);
    gotoXY(x, y++); printf("%-*s : %s", L, "Nama", s->nama);
    gotoXY(x, y++); printf("%-*s : %d m", L, "MDPL", s->mdpl);
    gotoXY(x, y++); printf("%-*s : %s", L, "Kota", s->kota);
    gotoXY(x, y++); printf("%-*s : %s", L, "Alamat", s->alamat);
    gotoXY(x, y++); printf("%-*s : %s", L, "Status", status_ops);

    print_padded(pop_x + 3, pop_y + pop_h - 2, pop_w - 6, "Tekan tombol apa saja untuk kembali...");
    _getch();
}

static void stasiun_popup_add(int split_x, int content_w) {
    int pop_w = 0, pop_h = 22;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Stasiun");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    /* aturan:
       - kode: 3 huruf (tanpa spasi/angka/simbol) -> input terlarang tidak akan tampil
       - nama: hanya huruf & spasi (tanpa angka/simbol) -> input terlarang tidak akan tampil
       - mdpl: angka, maksimal 3 digit
    */
    char kode[8], nama[64], kota[64], alamat[96], mdpl_buf[8];
    char status_choice[8], status[20];
    int mdpl = 0;
    const int LABEL_W = 34;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: ", LABEL_W, "Kode Stasiun (3 huruf)");
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "Nama Stasiun");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "MDPL (0-999)");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "Kota");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Alamat");
    gotoXY(label_x, pop_y + 12); printf("%-*s: ", LABEL_W, "Status [1] Aktif [2] Perawatan");

    /* Kode: 3 huruf */
    while (1) {
        gotoXY(field_x, pop_y + 2);
        printf("%-10s", "");
        gotoXY(field_x, pop_y + 2);
        input_alpha_only_filtered(kode, (int)sizeof(kode), 3);
        if (kode[0] == 27) return;

        if (!is_alpha_only_exact_len(kode, 3)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 16);
            printf("Kode harus 3 huruf, tanpa spasi/angka/simbol.%-15s", "");
            continue;
        }
        break;
    }

    /* Nama: huruf & spasi */
    while (1) {
        gotoXY(field_x, pop_y + 4);
        printf("%-40s", "");
        gotoXY(field_x, pop_y + 4);
        input_alpha_space_filtered(nama, (int)sizeof(nama), 63);
        if (nama[0] == 27) return;

        if (is_blank(nama) || !is_alpha_space_only(nama)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 16);
            printf("Nama hanya boleh huruf dan spasi (tanpa angka/simbol).%-12s", "");
            continue;
        }
        break;
    }

    /* MDPL: angka max 3 digit (0..999) */
    while (1) {
        gotoXY(field_x, pop_y + 6);
        printf("%-6s", "");
        gotoXY(field_x, pop_y + 6);
        input_digits(mdpl_buf, 4); /* 3 digit */
        if (mdpl_buf[0] == 27) return;

        if (is_blank(mdpl_buf)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 16);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "MDPL wajib diisi (angka 0-999).");
            continue;
        }
        mdpl = atoi(mdpl_buf);
        if (mdpl < 0) mdpl = 0;
        if (mdpl > 999) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 16);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "MDPL maksimal 3 digit (0-999).");
            continue;
        }
        break;
    }

    /* Kota */
    while (1) {
        gotoXY(field_x, pop_y + 8);
        printf("%-45s", "");
        gotoXY(field_x, pop_y + 8);
        input_text(kota, 63, 1);
        if (kota[0] == 27) return;
        if (!ui_require_not_blank(kota, pop_x, pop_y, pop_w, pop_h, "Kota")) continue;
        break;
    }

    /* Alamat */
    while (1) {
        gotoXY(field_x, pop_y + 10);
        printf("%-45s", "");
        gotoXY(field_x, pop_y + 10);
        input_text(alamat, 95, 1);
        if (alamat[0] == 27) return;
        if (!ui_require_not_blank(alamat, pop_x, pop_y, pop_w, pop_h, "Alamat")) continue;
        break;
    }

    /* Status (pilihan) - WAJIB pilih */
    while (1) {
        gotoXY(field_x, pop_y + 12);
        printf("%-10s", "");
        gotoXY(field_x, pop_y + 12);
        input_choice_digits_filtered(status_choice, (int)sizeof(status_choice), "12", 0);
        if (status_choice[0] == 27) return;
        if (!ui_require_not_blank(status_choice, pop_x, pop_y, pop_w, pop_h, "Status")) continue;
        break;
    }

    if (status_choice[0] == '1') snprintf(status, sizeof(status), "Aktif");
    else snprintf(status, sizeof(status), "Perawatan");

    if (0) {
        Beep(700, 80);
        gotoXY(pop_x + 3, pop_y + 16);
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "Kota dan alamat wajib diisi. Tekan tombol apa saja...");
        _getch();
        return;
    }

    char id_new[16];
    stasiun_create_auto(id_new, sizeof(id_new), kode, nama, mdpl, kota, alamat, status);

    gotoXY(pop_x + 3, pop_y + 16);
    printf("Berhasil tambah. ID: %s. Tekan tombol apa saja...", id_new);
    _getch();
}

static void stasiun_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 0, pop_h = 24;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Stasiun (kosongkan untuk tetap)");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    char kode_in[8], nama_in[64], kota[64], alamat[96], mdpl_buf[8];
    char status_choice[8];
    char kode_norm[8], nama_norm[64];

    const int LABEL_W = 34;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: %s", LABEL_W, "ID (tidak bisa diubah)", g_stasiun[idx].id);
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "Kode Stasiun (3 huruf)");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "Nama Stasiun");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "MDPL (0-999)");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Kota");
    gotoXY(label_x, pop_y + 12); printf("%-*s: ", LABEL_W, "Alamat");
    gotoXY(label_x, pop_y + 14); printf("%-*s: ", LABEL_W, "Status [1] Aktif [2] Perawatan");
    gotoXY(label_x, pop_y + 16); printf("(Saat ini: %s)", g_stasiun[idx].status);

    gotoXY(field_x, pop_y + 4);  input_alpha_only_filtered(kode_in, (int)sizeof(kode_in), 3);       if (kode_in[0] == 27) return;
    gotoXY(field_x, pop_y + 6);  input_alpha_space_filtered(nama_in, (int)sizeof(nama_in), 63);     if (nama_in[0] == 27) return;
    gotoXY(field_x, pop_y + 8);  input_digits(mdpl_buf, 4);                                         if (mdpl_buf[0] == 27) return;
    gotoXY(field_x, pop_y + 10); input_text(kota, 63, 1);                                            if (kota[0] == 27) return;
    gotoXY(field_x, pop_y + 12); input_text(alamat, 95, 1);                                          if (alamat[0] == 27) return;
    gotoXY(field_x, pop_y + 14); input_choice_digits_filtered(status_choice, (int)sizeof(status_choice), "12", 1);
    if (status_choice[0] == 27) return;

    /* Kode (jika diisi) */
    const char *new_kode = g_stasiun[idx].kode;
    if (!is_blank(kode_in)) {
        if (!is_alpha_only_exact_len(kode_in, 3)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "Kode harus 3 huruf, tanpa spasi/angka/simbol. Tekan tombol apa saja...");
            _getch();
            return;
        }
        snprintf(kode_norm, sizeof(kode_norm), "%s", kode_in);
        new_kode = kode_norm;
    }

    /* Nama (jika diisi) */
    const char *new_nama = g_stasiun[idx].nama;
    if (!is_blank(nama_in)) {
        if (!is_alpha_space_only(nama_in)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "Nama hanya boleh huruf dan spasi (tanpa angka/simbol). Tekan tombol apa saja...");
            _getch();
            return;
        }
        snprintf(nama_norm, sizeof(nama_norm), "%s", nama_in);
        new_nama = nama_norm;
    }

    const char *new_kota = is_blank(kota) ? g_stasiun[idx].kota : kota;
    const char *new_alamat = is_blank(alamat) ? g_stasiun[idx].alamat : alamat;
    const char *new_status = g_stasiun[idx].status;
    char status_norm[20];
    if (!is_blank(status_choice)) {
        if (status_choice[0] == '1') snprintf(status_norm, sizeof(status_norm), "Aktif");
        else snprintf(status_norm, sizeof(status_norm), "Perawatan");
        new_status = status_norm;
    }

    /* MDPL (jika diisi) */
    int mdpl = g_stasiun[idx].mdpl;
    if (!is_blank(mdpl_buf)) {
        if (!is_digits_only(mdpl_buf)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "MDPL harus angka (0-999). Tekan tombol apa saja...");
            _getch();
            return;
        }
        mdpl = atoi(mdpl_buf);
        if (mdpl < 0) mdpl = 0;
        if (mdpl > 999) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "MDPL maksimal 3 digit (0-999).");
            _getch();
            return;
        }
    }

    stasiun_update(idx, new_kode, new_nama, mdpl, new_kota, new_alamat, new_status);

    /* Tampilkan pesan di area aman agar tidak overwrite action bar */
    ui_form_message(pop_x, pop_y, pop_w, pop_h, "Berhasil memperbarui. Tekan tombol apa saja...");
    _getch();
}

static void stasiun_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 0, pop_h = 14;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Stasiun");
ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Ubah status stasiun %s (%s) menjadi NONAKTIF?", g_stasiun[idx].id, g_stasiun[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    print_padded(pop_x + 3, pop_y + 6, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            stasiun_delete(idx);
            gotoXY(pop_x + 3, pop_y + 8);
            print_padded(pop_x + 3, pop_y + 8, pop_w - 6, "Data berhasil dinonaktifkan. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

static void view_stasiun() {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO = 3;
    int W_ID = 8;
    int W_KODE = 8;
    int W_NAMA = 18;
    int W_KOTA = 14;
    int W_MDPL = 6;
    int W_ALAMAT = 26;
    int W_STATUS = 12;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
        /* reset ukuran kolom */
        W_NO = 3; W_KODE = 10; W_NAMA = 18; W_MDPL = 6; W_KOTA = 12; W_ALAMAT = 22; W_STATUS = 10;
        int col_w[] = { W_NO, W_KODE, W_NAMA, W_MDPL, W_KOTA, W_ALAMAT, W_STATUS };
        const int min_w[] = { 3, 6, 6, 4, 6, 10, 7 };
        fit_columns(col_w, min_w, 7, usable_w);
        W_NO = col_w[0]; W_KODE = col_w[1]; W_NAMA = col_w[2]; W_MDPL = col_w[3]; W_KOTA = col_w[4]; W_ALAMAT = col_w[5]; W_STATUS = col_w[6];


        int total = g_stasiunCount;
        int idxs[MAX_RECORDS];
        for (int i = 0; i < total; i++) idxs[i] = i;
        if (total > 1) qsort(idxs, (size_t)total, sizeof(int), stasiun_cmp_idx);

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

        int table_w = table_total_width(col_w, 7);
        int table_x = content_left;

        int table_y = 10;

        stasiun_print_hline(table_x, table_y, W_NO, W_KODE, W_NAMA, W_MDPL, W_KOTA, W_ALAMAT, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_KODE, "Kode", W_NAMA, "Nama", W_MDPL, "MDPL", W_KOTA, "Kota", W_ALAMAT, "Alamat", W_STATUS, "Status");
        stasiun_print_hline(table_x, table_y + 2, W_NO, W_KODE, W_NAMA, W_MDPL, W_KOTA, W_ALAMAT, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int pos = start + r;
                int n = idxs[pos];
                const char *status = g_stasiun[n].active ? g_stasiun[n].status : "Nonaktif";

                char mdpl_s[16];
                snprintf(mdpl_s, sizeof(mdpl_s), "%dm", g_stasiun[n].mdpl);

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %*s | %-*.*s | %-*.*s | %-*.*s |",
                       W_NO, pos + 1,
                       W_KODE, W_KODE, g_stasiun[n].kode,
                       W_NAMA, W_NAMA, g_stasiun[n].nama,
                       W_MDPL, mdpl_s,
                       W_KOTA, W_KOTA, g_stasiun[n].kota,
                       W_ALAMAT, W_ALAMAT, g_stasiun[n].alamat,
                       W_STATUS, W_STATUS, status);
            } else {
                /* BUGFIX: sebelumnya kolom kosong tidak lengkap + highlight bocor */
                table_print_blank_row(table_x, y, col_w, 7);
            }

            /* selalu reset supaya highlight tidak "bocor" ke area kosong/footer */
            set_highlight(0);
        }

        stasiun_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_KODE, W_NAMA, W_MDPL, W_KOTA, W_ALAMAT, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        print_padded(table_x, footer_y, table_w,
                     "Data: %d  (Aktif: %d | Nonaktif: %d)  |  Halaman: %d/%d",
                     total, aktif, nonaktif, page + 1, total_pages);
        print_padded(table_x, footer_y + 1, table_w,
                     "Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian");
        print_padded(table_x, footer_y + 2, table_w,
                     "Aksi: [A] Tambah  [E] Edit  [X] Nonaktifkan  [ESC] Kembali");

        /* Siapkan panel bawah untuk form CRUD/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, footer_y + 2, table_w, h);

        int ch = _getch();
        if (ch == 27) return; /* ESC */

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
            stasiun_popup_detail(split_x, content_w, &g_stasiun[idxs[start + selected]]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idxs[start + selected];
            if (!g_stasiun[idx].active) { Beep(800, 60); continue; }
            stasiun_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idxs[start + selected];
            if (!g_stasiun[idx].active) { Beep(800, 60); continue; }
            stasiun_popup_delete_soft(split_x, content_w, idx);
            continue;
        }
    }
}

/* ---------- KERETA ---------- */

static void kereta_print_hline(int x, int y,
                               int w_no, int w_nama, int w_kelas,
                               int w_kap, int w_gerb, int w_status) {
    gotoXY(x, y); putchar('+');
    for (int i = 0; i < w_no + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_nama + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kelas + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_kap + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_gerb + 2; i++) putchar('-'); putchar('+');
    for (int i = 0; i < w_status + 2; i++) putchar('-'); putchar('+');
}


static void kereta_popup_detail(int split_x, int content_w, const Kereta *k) {
    (void)content_w;

    /* Layout detail dibuat rapat dan konsisten dengan "Rincian Jadwal". */
    int pop_h = 13;
    int pop_x, pop_y, pop_w;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Kereta");

    int x = pop_x + 3;
    int y = pop_y + 1;
    const int L = 12;

    /* Status operasional hanya: Aktif / Perawatan (dan Nonaktif bila soft delete). */
    const char *status_ops = "Aktif";
    if (!k->active) status_ops = "Nonaktif";
    else if (strcmp(k->status, "Perawatan") == 0) status_ops = "Perawatan";

    gotoXY(x, y++); printf("%-*s : %s", L, "ID Kereta", k->kode);
    gotoXY(x, y++); printf("%-*s : %s", L, "Nama", k->nama);
    gotoXY(x, y++); printf("%-*s : %s", L, "Kelas", k->kelas);
    gotoXY(x, y++); printf("%-*s : %d", L, "Kapasitas", k->kapasitas);
    gotoXY(x, y++); printf("%-*s : %d", L, "Gerbong", k->gerbong);
    gotoXY(x, y++); printf("%-*s : %s", L, "Status", status_ops);
    gotoXY(x, y++); printf("%-*s : %s", L, "Data Aktif", k->active ? "Ya" : "Tidak");

    print_padded(pop_x + 3, pop_y + pop_h - 2, pop_w - 6, "Tekan tombol apa saja untuk kembali...");
    _getch();
}

static void kereta_popup_add(int split_x, int content_w) {
    int pop_w = 0, pop_h = 24;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Kereta");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    /* kelas/status dibuat pilihan digit agar input terlarang tidak tampil */
    char nama[64], kelas_in[8], kelas[20], kap_buf[16], gerb_buf[16], status_in[8], status[16];

    const int LABEL_W = 30;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: ", LABEL_W, "Nama Kereta");
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "Kelas Kereta (pilih 1-3)");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "Kapasitas (angka)");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "Jumlah Gerbong (angka)");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Status Operasional (pilih 1-2)");

    gotoXY(label_x, pop_y + 12);
    printf("Kelas: 1=Bisnis  2=Ekonomi  3=Eksekutif    |    Status: 1=Aktif  2=Perawatan");

    /* Nama */
    while (1) {
        gotoXY(field_x, pop_y + 2);
        input_alpha_space_filtered(nama, (int)sizeof(nama), 63);
        if (nama[0] == 27) return;
        if (is_blank(nama) || !is_alpha_space_only(nama)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 13);
            printf("Nama tidak valid. Hanya huruf dan spasi (tanpa angka/simbol).%*s", 25, "");
            continue;
        }

        /* Validasi Duplikat */
        int duplicate = 0;
        for (int i = 0; i < g_keretaCount; i++) {
            if (g_kereta[i].active && _stricmp(g_kereta[i].nama, nama) == 0) {
                duplicate = 1;
                break;
            }
        }

        if (duplicate) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 13);
            printf("Nama kereta sudah digunakan. Gunakan nama lain.%*s", 20, "");
            continue;
        }

        break;
    }

    /* Kelas (digit only: 1/2/3) */
    while (1) {
        gotoXY(field_x, pop_y + 4);
        printf("%-6s", "");
        gotoXY(field_x, pop_y + 4);
        input_choice_digits_filtered(kelas_in, (int)sizeof(kelas_in), "123", 0);
        if (kelas_in[0] == 27) return;
        if (!normalize_kelas_ui(kelas, (int)sizeof(kelas), kelas_in)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 13);
            printf("Kelas wajib pilih 1/2/3.%*s", 55, "");
            continue;
        }
        break;
    }

    /* Kapasitas & gerbong (dibatasi agar input tidak melewati batas tampilan)
       Kapasitas: max 3 digit (1..999)
       Gerbong  : max 2 digit (1..99)
    */
    /* NOTE: input_digits() menerima parameter max_len = ukuran buffer (termasuk '\0').
       Jadi untuk 3 digit, butuh 4 (3 digit + '\0'); untuk 2 digit, butuh 3.
    */
    while (1) {
        gotoXY(field_x, pop_y + 6);
        input_digits(kap_buf, 4);   /* 3 digit */
        if (kap_buf[0] == 27) return;
        if (!ui_require_not_blank(kap_buf, pop_x, pop_y, pop_w, pop_h, "Kapasitas")) continue;
        {
            int kap_tmp = atoi(kap_buf);
            if (!ui_require_valid(kap_tmp > 0, pop_x, pop_y, pop_w, pop_h,
                                  "[PERINGATAN] Kapasitas harus > 0.")) {
                continue;
            }
        }
        break;
    }

    while (1) {
        gotoXY(field_x, pop_y + 8);
        input_digits(gerb_buf, 3);  /* 2 digit */
        if (gerb_buf[0] == 27) return;
        if (!ui_require_not_blank(gerb_buf, pop_x, pop_y, pop_w, pop_h, "Jumlah Gerbong")) continue;
        {
            int gerb_tmp = atoi(gerb_buf);
            if (!ui_require_valid(gerb_tmp > 0 && gerb_tmp <= 12, pop_x, pop_y, pop_w, pop_h,
                                  "[PERINGATAN] Jumlah gerbong harus 1-12.")) {
                continue;
            }
        }
        break;
    }
    int kap = atoi(kap_buf);
    int gerb = atoi(gerb_buf);

    /* Status (digit only: 1/2) - WAJIB pilih */
    while (1) {
        gotoXY(field_x, pop_y + 10);
        printf("%-6s", "");
        gotoXY(field_x, pop_y + 10);
        input_choice_digits_filtered(status_in, (int)sizeof(status_in), "12", 0);
        if (status_in[0] == 27) return;
        if (!ui_require_not_blank(status_in, pop_x, pop_y, pop_w, pop_h, "Status Operasional")) continue;
        if (!normalize_status_ui(status, (int)sizeof(status), status_in)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 13);
            printf("Status wajib pilih 1/2.%*s", 55, "");
            continue;
        }
        break;
    }

    char id_new[16];
    kereta_create_auto(id_new, sizeof(id_new), nama, kelas, kap, gerb, status);

    gotoXY(pop_x + 3, pop_y + 15);
    printf("Berhasil tambah kereta. ID: %s. Tekan tombol apa saja...", id_new);
    _getch();
}

static void kereta_popup_edit(int split_x, int content_w, int idx) {
    int pop_w = 0, pop_h = 26;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Kereta (kosongkan untuk tetap)");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    /* kelas/status dibuat pilihan digit agar input terlarang tidak tampil */
    char nama[64], kelas_in[8], kelas[20], kap_buf[16], gerb_buf[16], status_in[8], status[16];

    const int LABEL_W = 30;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: %s", LABEL_W, "ID Kereta", g_kereta[idx].kode);
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "Nama Kereta");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "Kelas Kereta (pilih 1-3)");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "Kapasitas (angka)");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Jumlah Gerbong (angka)");
    gotoXY(label_x, pop_y + 12); printf("%-*s: ", LABEL_W, "Status Operasional (pilih 1-2)");

    gotoXY(label_x, pop_y + 14);
    printf("Saat ini: Kelas=%s  |  Status=%s", g_kereta[idx].kelas, g_kereta[idx].status);
    gotoXY(label_x, pop_y + 15);
    printf("Kelas: 1=Bisnis  2=Ekonomi  3=Eksekutif    |    Status: 1=Aktif  2=Perawatan");

    /* input */
    gotoXY(field_x, pop_y + 4);  input_alpha_space_filtered(nama, (int)sizeof(nama), 63); if (nama[0] == 27) return;

    gotoXY(field_x, pop_y + 6);  printf("%-6s", "");
    gotoXY(field_x, pop_y + 6);  input_choice_digits_filtered(kelas_in, (int)sizeof(kelas_in), "123", 1);
    if (kelas_in[0] == 27) return;

    /* Batasi digit agar konsisten dengan tampilan dan validasi:
       Kapasitas max 3 digit (1..999), Gerbong max 2 digit (1..99)
       Edit: boleh dikosongkan untuk tetap, namun jika diisi harus > 0.
    */
    while (1) {
        gotoXY(field_x, pop_y + 8);
        input_digits(kap_buf, 4);  /* 3 digit */
        if (kap_buf[0] == 27) return;
        if (is_blank(kap_buf)) break;
        {
            int kap_tmp = atoi(kap_buf);
            if (!ui_require_valid(kap_tmp > 0, pop_x, pop_y, pop_w, pop_h,
                                  "[PERINGATAN] Kapasitas harus > 0.")) {
                continue;
            }
        }
        break;
    }
    while (1) {
        gotoXY(field_x, pop_y + 10);
        input_digits(gerb_buf, 3); /* 2 digit */
        if (gerb_buf[0] == 27) return;
        if (is_blank(gerb_buf)) break;
        {
            int gerb_tmp = atoi(gerb_buf);
            if (!ui_require_valid(gerb_tmp > 0 && gerb_tmp <= 12, pop_x, pop_y, pop_w, pop_h,
                                  "[PERINGATAN] Jumlah gerbong harus 1-12.")) {
                continue;
            }
        }
        break;
    }

    gotoXY(field_x, pop_y + 12); printf("%-6s", "");
    /* Edit: boleh dikosongkan (ENTER) untuk tetap */
    gotoXY(field_x, pop_y + 12); input_choice_digits_filtered(status_in, (int)sizeof(status_in), "12", 1);
    if (status_in[0] == 27) return;

    /* Validasi nama (jika diisi) */
    const char *new_nama = g_kereta[idx].nama;
    if (!is_blank(nama)) {
        if (!is_alpha_space_only(nama)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "Nama tidak valid. Hanya huruf dan spasi (tanpa angka/simbol). Tekan tombol apa saja...");
            _getch();
            return;
        }

        /* Cek Duplikat (exclude diri sendiri) */
        int duplicate = 0;
        for (int i = 0; i < g_keretaCount; i++) {
            if (i == idx) continue; /* skip diri sendiri */
            if (g_kereta[i].active && _stricmp(g_kereta[i].nama, nama) == 0) {
                duplicate = 1;
                break;
            }
        }

        if (duplicate) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "Nama kereta sudah digunakan. Gunakan nama lain. Tekan tombol apa saja...");
            _getch();
            return;
        }

        new_nama = nama;
    }

    /* Validasi kelas (jika diisi) */
    const char *new_kelas = g_kereta[idx].kelas;
    if (!is_blank(kelas_in)) {
        if (!normalize_kelas_ui(kelas, (int)sizeof(kelas), kelas_in)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "Kelas harus pilih 1/2/3. Tekan tombol apa saja...");
            _getch();
            return;
        }
        new_kelas = kelas;
    }

    /* Kapasitas */
    int kap = g_kereta[idx].kapasitas;
    if (!is_blank(kap_buf)) {
        kap = atoi(kap_buf);
    }

    /* Gerbong */
    int gerb = g_kereta[idx].gerbong;
    if (!is_blank(gerb_buf)) {
        gerb = atoi(gerb_buf);
    }

    /* Safety net: data lama mungkin corrupt, tapi jangan biarkan simpan nilai <= 0 */
    if (kap <= 0 || gerb <= 0 || gerb > 12) {
        ui_require_valid(0, pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Kapasitas harus > 0 dan gerbong 1-12.");
        return;
    }

    /* Status operasional (jika diisi) */
    const char *new_status = g_kereta[idx].status;
    if (!is_blank(status_in)) {
        if (!normalize_status_ui(status, (int)sizeof(status), status_in)) {
            Beep(700, 80);
            gotoXY(pop_x + 3, pop_y + 18);
            print_padded(pop_x + 3, pop_y + 18, pop_w - 6, "Status harus pilih 1/2. Tekan tombol apa saja...");
            _getch();
            return;
        }
        new_status = status;
    }

    kereta_update(idx, new_nama, new_kelas, kap, gerb, new_status);

    /* Pesan di area aman agar tidak overwrite action bar */
    ui_form_message(pop_x, pop_y, pop_w, pop_h, "Berhasil memperbarui. Tekan tombol apa saja...");
    _getch();
}

static void kereta_popup_delete_soft(int split_x, int content_w, int idx) {
    int pop_w = 0, pop_h = 14;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Nonaktifkan Data Kereta");
ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Nonaktifkan data kereta %s (%s)? (soft delete)", g_kereta[idx].kode, g_kereta[idx].nama);

    gotoXY(pop_x + 3, pop_y + 6);
    print_padded(pop_x + 3, pop_y + 6, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            kereta_delete(idx);
            gotoXY(pop_x + 3, pop_y + 8);
            print_padded(pop_x + 3, pop_y + 8, pop_w - 6, "Data berhasil dinonaktifkan. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

static void view_kereta() {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO = 3;

    int W_NAMA = 18;
    int W_KELAS = 12;
    int W_KAP = 9;
    int W_GERB = 7;
    int W_STATUS = 12;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
        W_NO = 3; W_NAMA = 22; W_KELAS = 12; W_KAP = 9; W_GERB = 8; W_STATUS = 12;
        int col_w[] = { W_NO, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS };
        const int min_w[] = { 3, 12, 8, 8, 6, 8 };
        fit_columns(col_w, min_w, 6, usable_w);
        W_NO = col_w[0]; W_NAMA = col_w[1]; W_KELAS = col_w[2]; W_KAP = col_w[3]; W_GERB = col_w[4]; W_STATUS = col_w[5];


        int total = g_keretaCount;
        int idxs[MAX_RECORDS];
        for (int i = 0; i < total; i++) idxs[i] = i;
        if (total > 1) qsort(idxs, (size_t)total, sizeof(int), kereta_cmp_idx);

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

        int table_w = table_total_width(col_w, 6); (void)table_w;
        int table_x = content_left;

        int table_y = 10;

        kereta_print_hline(table_x, table_y, W_NO, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No", W_NAMA, "Nama", W_KELAS, "Kelas", W_KAP, "Kapasitas", W_GERB, "Gerbong", W_STATUS, "Status");
        kereta_print_hline(table_x, table_y + 2, W_NO, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;

            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int pos = start + r;
                int n = idxs[pos];
                const char *status = g_kereta[n].active ? g_kereta[n].status : "Nonaktif";

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %*d | %*d | %-*.*s |",
                       W_NO, pos + 1,
                       W_NAMA, W_NAMA, g_kereta[n].nama,
                       W_KELAS, W_KELAS, g_kereta[n].kelas,
                       W_KAP, g_kereta[n].kapasitas,
                       W_GERB, g_kereta[n].gerbong,
                       W_STATUS, W_STATUS, status);
            } else {
                table_print_blank_row(table_x, y, col_w, 6);
            }

            set_highlight(0);
        }

        kereta_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_NAMA, W_KELAS, W_KAP, W_GERB, W_STATUS);

        int footer_y = table_y + 3 + ROWS_PER_PAGE + 2;

        print_padded(table_x, footer_y, table_w,
                     "Data: %d  (Aktif: %d | Nonaktif: %d)  |  Halaman: %d/%d",
                     total, aktif, nonaktif, page + 1, total_pages);
        print_padded(table_x, footer_y + 1, table_w,
                     "Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian");
        print_padded(table_x, footer_y + 2, table_w,
                     "Aksi: [A] Tambah  [E] Edit  [X] Nonaktifkan  [ESC] Kembali");

        /* Siapkan panel bawah untuk form CRUD/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, footer_y + 2, table_w, h);

        int ch = _getch();
        if (ch == 27) return; /* ESC */

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
            kereta_popup_detail(split_x, content_w, &g_kereta[idxs[start + selected]]);
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idxs[start + selected];
            if (!g_kereta[idx].active) { Beep(800, 60); continue; }
            kereta_popup_edit(split_x, content_w, idx);
            continue;
        }

        if (ch == 'x' || ch == 'X') {
            if (rows_on_page == 0) { Beep(800, 60); continue; }
            int idx = idxs[start + selected];
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

static int auth_find_account_index(const char* user_input, const char* password) {
    if (!user_input || !password) return -1;

    char email_norm[128];
    if (!akun_normalize_login_email(user_input, email_norm, (int)sizeof(email_norm))) {
        return -1; /* domain selain @kai.id -> ditolak */
    }

    for (int i = 0; i < g_accountCount; i++) {
        if (!g_accounts[i].active) continue;
        if (strcmp(g_accounts[i].email, email_norm) == 0 &&
            strcmp(g_accounts[i].password, password) == 0) {
            return i;
        }
    }
    return -1;
}

/* ================== DASHBOARD MENU UTAMA (ROLE-BASED) ================== */

/* ================== TRANSAKSI: JADWAL & PEMBAYARAN ================== */

/* Datetime DD-MM-YYYY HH:MM : input aman (tanpa auto-separator), terima digit, '-', spasi, ':' */
/* Input datetime dengan auto-format DD-MM-YYYY HH:MM (anti-bug loncat) */
static void input_datetime_ddmmyyyy_hhmm_filtered(char *buffer, int buffer_sz) {
    if (!buffer || buffer_sz <= 0) return;

    /* simpan posisi awal (kursor saat field akan digambar) */
    CONSOLE_SCREEN_BUFFER_INFO csbi0;
    int start_x = 0, start_y = 0;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi0)) {
        start_x = (int)csbi0.dwCursorPosition.X;
        start_y = (int)csbi0.dwCursorPosition.Y;
    }

    /* format internal = 12 digit: DDMMYYYYHHMM */
    char digits[13] = {0};
    int dlen = 0;

    buffer[0] = '\0';

    /* hitung lebar efektif agar tidak overflow ke border kanan */
    int field_w = 16;
    {
        int screen_w = get_screen_width();
        int safe_right = screen_w - FRAME_THICK - 1 - CONTENT_MARGIN;
        if (safe_right < 0) safe_right = screen_w - 1;

        int total = field_w + 2; /* '[' + inner + ']' */
        if (start_x + total - 1 > safe_right) {
            int max_inner = safe_right - start_x - 1; /* sisakan untuk ']' */
            if (max_inner < 1) max_inner = 1;
            if (max_inner < field_w) field_w = max_inner;
        }
    }

    ui_draw_input_box_inline(field_w);

    /* posisi di dalam box = setelah '[' */
    int field_x = start_x + 1;
    int field_y = start_y;

    while (1) {
        int ch = _getch();

        if (ch == 13) { /* ENTER -> simpan hasil */
            char out[17] = {0};
            int oi = 0;
            for (int i = 0; i < dlen && oi < 16; i++) {
                if (i == 2 || i == 4) out[oi++] = '-';
                else if (i == 8) out[oi++] = ' ';
                else if (i == 10) out[oi++] = ':';
                out[oi++] = digits[i];
            }
            out[oi] = '\0';

            int maxc = buffer_sz - 1;
            if (maxc < 0) maxc = 0;
            strncpy(buffer, out, (size_t)maxc);
            buffer[maxc] = '\0';
            return;
        }
        if (ch == 27) { /* ESC */
            buffer[0] = 27;
            return;
        }

        if (ch == 8) { /* BACKSPACE */
            if (dlen > 0) dlen--;
        } else if (ch >= '0' && ch <= '9') {
            if (dlen < 12) {
                digits[dlen++] = (char)ch;
                digits[dlen] = '\0';
            }
        } else {
            /* abaikan tombol lain */
        }

        /* bangun tampilan DD-MM-YYYY HH:MM dari digits */
        char view[17] = {0};
        int vi = 0;
        for (int i = 0; i < dlen && vi < 16; i++) {
            if (i == 2 || i == 4) view[vi++] = '-';
            else if (i == 8) view[vi++] = ' ';
            else if (i == 10) view[vi++] = ':';
            view[vi++] = digits[i];
        }
        view[vi] = '\0';

        /* render ulang field dengan gotoXY (lebih aman dari  / overflow) */
        gotoXY(field_x, field_y);
        int vlen = (int)strlen(view);
        if (vlen > field_w) vlen = field_w;

        for (int k = 0; k < vlen; k++) putchar(view[k]);
        for (int k = vlen; k < field_w; k++) putchar(' ');

        /* set cursor di akhir teks */
        gotoXY(field_x + vlen, field_y);
    }
}


/* Datetime DD-MM-YYYY HH:MM : versi prefill (edit) dengan auto-format */
static void input_datetime_ddmmyyyy_hhmm_prefill_filtered(char *buffer, int buffer_sz) {
    if (!buffer || buffer_sz <= 0) return;

    /* simpan posisi awal (kursor saat field akan digambar) */
    CONSOLE_SCREEN_BUFFER_INFO csbi0;
    int start_x = 0, start_y = 0;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi0)) {
        start_x = (int)csbi0.dwCursorPosition.X;
        start_y = (int)csbi0.dwCursorPosition.Y;
    }

    char digits[13] = {0};
    int dlen = 0;

    /* ekstrak digit dari buffer awal */
    for (int i = 0; buffer[i] != '\0' && dlen < 12; i++) {
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            digits[dlen++] = buffer[i];
            digits[dlen] = '\0';
        }
    }

    /* hitung lebar efektif */
    int field_w = 16;
    {
        int screen_w = get_screen_width();
        int safe_right = screen_w - FRAME_THICK - 1 - CONTENT_MARGIN;
        if (safe_right < 0) safe_right = screen_w - 1;

        int total = field_w + 2;
        if (start_x + total - 1 > safe_right) {
            int max_inner = safe_right - start_x - 1;
            if (max_inner < 1) max_inner = 1;
            if (max_inner < field_w) field_w = max_inner;
        }
    }

    ui_draw_input_box_inline(field_w);

    int field_x = start_x + 1;
    int field_y = start_y;

    while (1) {
        /* tampilkan isi terkini */
        char view[17] = {0};
        int vi = 0;
        for (int i = 0; i < dlen && vi < 16; i++) {
            if (i == 2 || i == 4) view[vi++] = '-';
            else if (i == 8) view[vi++] = ' ';
            else if (i == 10) view[vi++] = ':';
            view[vi++] = digits[i];
        }
        view[vi] = '\0';

        gotoXY(field_x, field_y);
        int vlen = (int)strlen(view);
        if (vlen > field_w) vlen = field_w;

        for (int k = 0; k < vlen; k++) putchar(view[k]);
        for (int k = vlen; k < field_w; k++) putchar(' ');

        gotoXY(field_x + vlen, field_y);

        int ch = _getch();

        if (ch == 13) { /* ENTER */
            char out[17] = {0};
            int oi = 0;
            for (int i = 0; i < dlen && oi < 16; i++) {
                if (i == 2 || i == 4) out[oi++] = '-';
                else if (i == 8) out[oi++] = ' ';
                else if (i == 10) out[oi++] = ':';
                out[oi++] = digits[i];
            }
            out[oi] = '\0';

            int maxc = buffer_sz - 1;
            if (maxc < 0) maxc = 0;
            strncpy(buffer, out, (size_t)maxc);
            buffer[maxc] = '\0';
            return;
        }
        if (ch == 27) { /* ESC */
            buffer[0] = 27;
            return;
        }

        if (ch == 8) { /* BACKSPACE */
            if (dlen > 0) dlen--;
        } else if (ch >= '0' && ch <= '9') {
            if (dlen < 12) {
                digits[dlen++] = (char)ch;
                digits[dlen] = '\0';
            }
        } else {
            /* abaikan */
        }
    }
}



/* validasi format DD-MM-YYYY HH:MM */
static int looks_like_datetime_ddmmyyyy_hhmm(const char *s) {
    if (!s) return 0;
    if (strlen(s) != 16) return 0;
    /* DD-MM-YYYY HH:MM */
    if (!(isdigit((unsigned char)s[0]) && isdigit((unsigned char)s[1]) &&
          s[2] == '-' &&
          isdigit((unsigned char)s[3]) && isdigit((unsigned char)s[4]) &&
          s[5] == '-' &&
          isdigit((unsigned char)s[6]) && isdigit((unsigned char)s[7]) &&
          isdigit((unsigned char)s[8]) && isdigit((unsigned char)s[9]) &&
          s[10] == ' ' &&
          isdigit((unsigned char)s[11]) && isdigit((unsigned char)s[12]) &&
          s[13] == ':' &&
          isdigit((unsigned char)s[14]) && isdigit((unsigned char)s[15]))) return 0;

    int dd = (s[0]-'0')*10 + (s[1]-'0');
    int mm = (s[3]-'0')*10 + (s[4]-'0');
    int yyyy = (s[6]-'0')*1000 + (s[7]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0');
    int hh = (s[11]-'0')*10 + (s[12]-'0');
    int mn = (s[14]-'0')*10 + (s[15]-'0');

    if (yyyy < 1900 || yyyy > 2100) return 0;
    if (mm < 1 || mm > 12) return 0;
    if (dd < 1 || dd > 31) return 0;
    if (hh < 0 || hh > 23) return 0;
    if (mn < 0 || mn > 59) return 0;

    /* validasi hari per bulan (sederhana) */
    int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    int leap = ( (yyyy%4==0 && yyyy%100!=0) || (yyyy%400==0) );
    if (leap) mdays[2] = 29;
    if (dd > mdays[mm]) return 0;

    return 1;
}


/* Konversi DD-MM-YYYY HH:MM menjadi key sortable: YYYYMMDDHHMM (untuk perbandingan cepat) */
static long long datetime_ddmmyyyy_hhmm_to_key(const char *s) {
    if (!looks_like_datetime_ddmmyyyy_hhmm(s)) return -1;

    int dd = (s[0]-'0')*10 + (s[1]-'0');
    int mm = (s[3]-'0')*10 + (s[4]-'0');
    int yyyy = (s[6]-'0')*1000 + (s[7]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0');
    int hh = (s[11]-'0')*10 + (s[12]-'0');
    int mn = (s[14]-'0')*10 + (s[15]-'0');

    return (long long)yyyy*100000000LL + (long long)mm*1000000LL + (long long)dd*10000LL
           + (long long)hh*100LL + (long long)mn;
}

/* forward decl (dipakai oleh sorting pembayaran sebelum modul laporan) */
static int report_date_key_ddmmyyyy(const char *s);

/* ================== SORTING INDEX HELPERS ==================
   Banyak modul menyimpan data dengan cara append (tambah di belakang) + soft delete.
   Kalau list ditampilkan sesuai urutan array mentah, urutan terlihat "acak".
   Solusi aman: tampilkan dengan index terurut (idxs[]) tanpa mengubah urutan data di file.
*/

static int cmp_active_first(int active_a, int active_b) {
    /* aktif dulu (1) lalu nonaktif (0) */
    if (active_a != active_b) return (active_a > active_b) ? -1 : 1;
    return 0;
}

static int kereta_cmp_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int c = cmp_active_first(g_kereta[ia].active, g_kereta[ib].active);
    if (c != 0) return c;
    c = strcmp(g_kereta[ia].kode, g_kereta[ib].kode);
    if (c != 0) return c;
    return strcmp(g_kereta[ia].nama, g_kereta[ib].nama);
}

static int stasiun_cmp_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int c = cmp_active_first(g_stasiun[ia].active, g_stasiun[ib].active);
    if (c != 0) return c;
    c = strcmp(g_stasiun[ia].id, g_stasiun[ib].id);
    if (c != 0) return c;
    return strcmp(g_stasiun[ia].nama, g_stasiun[ib].nama);
}

static int penumpang_cmp_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int c = cmp_active_first(g_penumpang[ia].active, g_penumpang[ib].active);
    if (c != 0) return c;
    c = strcmp(g_penumpang[ia].id, g_penumpang[ib].id);
    if (c != 0) return c;
    return strcmp(g_penumpang[ia].nama, g_penumpang[ib].nama);
}

static int karyawan_cmp_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int c = cmp_active_first(g_karyawan[ia].active, g_karyawan[ib].active);
    if (c != 0) return c;
    c = strcmp(g_karyawan[ia].id, g_karyawan[ib].id);
    if (c != 0) return c;
    return strcmp(g_karyawan[ia].nama, g_karyawan[ib].nama);
}

static int jadwal_cmp_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int c = cmp_active_first(g_jadwal[ia].active, g_jadwal[ib].active);
    if (c != 0) return c;

    long long ka = datetime_ddmmyyyy_hhmm_to_key(g_jadwal[ia].waktu_berangkat);
    long long kb = datetime_ddmmyyyy_hhmm_to_key(g_jadwal[ib].waktu_berangkat);

    /* data lama bisa invalid -> taruh paling bawah */
    if (ka < 0 && kb >= 0) return 1;
    if (ka >= 0 && kb < 0) return -1;
    if (ka < kb) return -1;
    if (ka > kb) return 1;

    return strcmp(g_jadwal[ia].id_jadwal, g_jadwal[ib].id_jadwal);
}

static long long pembayaran_audit_key(const PembayaranTiket *p) {
    if (!p) return -1;
    /* tgl_pembayaran disimpan YYYYMMDDHHMMSS (14 digit) */
    const char *s = p->tgl_pembayaran;
    if (!s) return -1;
    if ((int)strlen(s) != 14) return -1;
    for (int i = 0; i < 14; i++) if (!isdigit((unsigned char)s[i])) return -1;
    return atoll(s);
}

static int pembayaran_cmp_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int c = cmp_active_first(g_pembayaran[ia].active, g_pembayaran[ib].active);
    if (c != 0) return c;

    long long ka = pembayaran_audit_key(&g_pembayaran[ia]);
    long long kb = pembayaran_audit_key(&g_pembayaran[ib]);

    /* fallback: tanggal input (DD-MM-YYYY) */
    if (ka < 0) ka = (long long)report_date_key_ddmmyyyy(g_pembayaran[ia].tgl_dibuat) * 1000000LL;
    if (kb < 0) kb = (long long)report_date_key_ddmmyyyy(g_pembayaran[ib].tgl_dibuat) * 1000000LL;

    if (ka < kb) return -1;
    if (ka > kb) return 1;
    return strcmp(g_pembayaran[ia].id_pembayaran, g_pembayaran[ib].id_pembayaran);
}


static int kereta_exists_active(const char *kode) {
    if (!kode) return 0;
    for (int i = 0; i < g_keretaCount; i++) {
        if (strcmp(g_kereta[i].kode, kode) == 0 && g_kereta[i].active) return 1;
    }
    return 0;
}

static int stasiun_exists_active_by_id(const char *id) {
    if (!id) return 0;
    for (int i = 0; i < g_stasiunCount; i++) {
        if (strcmp(g_stasiun[i].id, id) == 0 && g_stasiun[i].active) return 1;
    }
    return 0;
}

static int penumpang_exists_active_by_id(const char *id) {
    if (!id) return 0;
    for (int i = 0; i < g_penumpangCount; i++) {
        if (strcmp(g_penumpang[i].id, id) == 0 && g_penumpang[i].active) return 1;
    }
    return 0;
}

/* Horizontal line tapi bukan "-----": pakai '=' */
static void jadwal_print_hline(int x, int y,
                             int W_NO, int W_TGL, int W_KER, int W_ASAL, int W_TUJUAN,
                             int W_BRG, int W_TIBA, int W_HRG, int W_KUO, int W_ST) {
    gotoXY(x, y);
    printf("|");
    for (int i = 0; i < W_NO + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TGL + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_KER + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_ASAL + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TUJUAN + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_BRG + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TIBA + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_HRG + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_KUO + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_ST + 2; i++) printf("=");
    printf("|");
}


static void pembayaran_print_hline(int x, int y,
                                   int W_NO, int W_TGL, int W_PNP, int W_JDW, int W_QTY, int W_TOT, int W_MET, int W_STS) {
    gotoXY(x, y);
    printf("|");
    for (int i = 0; i < W_NO + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TGL + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_PNP + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_JDW + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_QTY + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TOT + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_MET + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_STS + 2; i++) printf("=");
    printf("|");
}


/* ------------------- POPUP JADWAL ------------------- */

/* Forward declaration (needed because jadwal_popup_add() uses it before definition) */
static void jadwal_draw_form_add(int pop_x, int pop_y, int pop_w, int pop_h);

static void jadwal_popup_add(int account_index, int split_x, int content_w) {
    int pop_w = 0, pop_h = 26;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
    
    jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);

    char id_kereta[16], asal[16], tujuan[16], brg[32], tiba[32], harga_buf[16], kuota_buf[16];

    const int LABEL_W = 34;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;
    /* ---- Input ID Kereta (WAJIB) dengan referensi (F1 Kereta / F2 Stasiun) ---- */
    while (1) {
        gotoXY(field_x, pop_y + 2);
        int r = input_text_filtered_fkeys(id_kereta, sizeof(id_kereta), 10);
        if (r == -1 || id_kereta[0] == 27) return;
        if (r == 1) { /* F1 */
            /* simpan panel bawah agar posisi popup tetap konsisten */
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_kereta();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;

            /* redraw base untuk hilangkan artefak setelah ESC dari lookup */
            ui_redraw_base_simple("Kelola Jadwal Tiket");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (r == 2) { /* F2 */
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_stasiun();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;

            ui_redraw_base_simple("Kelola Jadwal Tiket");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (!ui_require_not_blank(id_kereta, pop_x, pop_y, pop_w, pop_h, "ID Kereta")) {
            continue;
        }
        break;
    }

    /* ---- Input ID Stasiun Asal (WAJIB) dengan referensi ---- */
    while (1) {
        gotoXY(field_x, pop_y + 4);
        int r = input_text_filtered_fkeys(asal, sizeof(asal), 10);
        if (r == -1 || asal[0] == 27) return;
        if (r == 1) {
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_kereta();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;
            ui_redraw_base_simple("Kelola Jadwal Tiket");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (r == 2) {
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_stasiun();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;
            ui_redraw_base_simple("Kelola Jadwal Tiket");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (!ui_require_not_blank(asal, pop_x, pop_y, pop_w, pop_h, "ID Stasiun Asal")) {
            continue;
        }
        break;
    }

    /* ---- Input ID Stasiun Tujuan (WAJIB) dengan referensi ---- */
    while (1) {
        gotoXY(field_x, pop_y + 6);
        int r = input_text_filtered_fkeys(tujuan, sizeof(tujuan), 10);
        if (r == -1 || tujuan[0] == 27) return;
        if (r == 1) {
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_kereta();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;
            ui_redraw_base_simple("Kelola Jadwal Tiket");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (r == 2) {
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_stasiun();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;
            ui_redraw_base_simple("Kelola Jadwal Tiket");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            jadwal_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (!ui_require_not_blank(tujuan, pop_x, pop_y, pop_w, pop_h, "ID Stasiun Tujuan")) {
            continue;
        }
        break;
    }

    /* ---- Field-field berikut WAJIB: paksa input sampai valid ---- */
    while (1) {
        gotoXY(field_x, pop_y + 8);
        input_datetime_ddmmyyyy_hhmm_filtered(brg, sizeof(brg));
        if (brg[0] == 27) return;
        if (!ui_require_not_blank(brg, pop_x, pop_y, pop_w, pop_h, "Waktu Berangkat")) continue;
        if (!looks_like_datetime_ddmmyyyy_hhmm(brg)) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Format Waktu Berangkat tidak valid.");
            continue;
        }
        break;
    }
    while (1) {
        gotoXY(field_x, pop_y + 10);
        input_datetime_ddmmyyyy_hhmm_filtered(tiba, sizeof(tiba));
        if (tiba[0] == 27) return;
        if (!ui_require_not_blank(tiba, pop_x, pop_y, pop_w, pop_h, "Waktu Tiba")) continue;
        if (!looks_like_datetime_ddmmyyyy_hhmm(tiba)) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Format Waktu Tiba tidak valid.");
            continue;
        }
        break;
    }
    while (1) {
        gotoXY(field_x, pop_y + 12);
        input_money_rp_only_filtered(harga_buf, (int)sizeof(harga_buf), 10);
        if (harga_buf[0] == 27) return;
        if (!ui_require_not_blank(harga_buf, pop_x, pop_y, pop_w, pop_h, "Harga Tiket")) continue;
        if (atoi(harga_buf) <= 0) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Harga Tiket harus > 0.");
            continue;
        }
        break;
    }
    while (1) {
        gotoXY(field_x, pop_y + 14);
        input_digits_only_filtered(kuota_buf, sizeof(kuota_buf), 10);
        if (kuota_buf[0] == 27) return;
        if (!ui_require_not_blank(kuota_buf, pop_x, pop_y, pop_w, pop_h, "Kuota Kursi")) continue;
        if (atoi(kuota_buf) <= 0) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Kuota Kursi harus > 0.");
            continue;
        }
        break;
    }

    ui_form_message_clear(pop_x, pop_y, pop_w, pop_h);

    if (!kereta_exists_active(id_kereta)) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "ID Kereta tidak ditemukan/Nonaktif. Tekan tombol apa saja...");
        _getch();
        return;
    }
    if (!stasiun_exists_active_by_id(asal) || !stasiun_exists_active_by_id(tujuan) || strcmp(asal, tujuan) == 0) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "ID Stasiun Asal/Tujuan tidak valid (atau sama). Tekan tombol apa saja...");
        _getch();
        return;
    }
    if (!looks_like_datetime_ddmmyyyy_hhmm(brg) || !looks_like_datetime_ddmmyyyy_hhmm(tiba)) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "Format waktu tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }
    long long k_brg = datetime_ddmmyyyy_hhmm_to_key(brg);
    long long k_tiba = datetime_ddmmyyyy_hhmm_to_key(tiba);
    if (k_brg < 0 || k_tiba < 0 || k_tiba <= k_brg) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "Waktu tiba harus setelah waktu berangkat. Tekan tombol apa saja...");
        _getch();
        return;
    }



    int harga = atoi(harga_buf);
    int kuota = atoi(kuota_buf);
    if (harga <= 0 || kuota <= 0) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "Harga/Kuota harus > 0. Tekan tombol apa saja...");
        _getch();
        return;
    }

    char tgl_brkt[16] = "";
    if ((int)strlen(brg) >= 10) {
        strncpy(tgl_brkt, brg, 10);
        tgl_brkt[10] = '\0';
    } else {
        strncpy(tgl_brkt, brg, sizeof(tgl_brkt)-1);
        tgl_brkt[sizeof(tgl_brkt)-1] = '\0';
    }

    char id_new[20];
    const char *kry = (account_index >= 0 && account_index < g_accountCount) ? g_accounts[account_index].id_karyawan : "";
    jadwal_create_auto(id_new, sizeof(id_new), tgl_brkt, id_kereta, asal, tujuan, brg, tiba, harga, kuota, kry);

    {
        char msg[128];
        snprintf(msg, sizeof(msg), "Jadwal dibuat: %s. Tekan tombol apa saja...", id_new);
        ui_form_message(pop_x, pop_y, pop_w, pop_h, msg);
    }
    _getch();
}

static void jadwal_popup_detail(int split_x, int content_w, int idx) {
    if (idx < 0 || idx >= g_jadwalCount) return;
    int pop_w = 0, pop_h = 22;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Jadwal");

    JadwalTiket *j = &g_jadwal[idx];

    int x = pop_x + 3;
    int y = pop_y + 1;

    gotoXY(x, y++); printf("ID Jadwal        : %s", j->id_jadwal);
    gotoXY(x, y++); printf("Kereta           : %s", ui_kereta_nama_by_kode(j->id_kereta));
    gotoXY(x, y++); printf("Stasiun Asal     : %s", ui_stasiun_nama_by_id(j->id_stasiun_asal));
    gotoXY(x, y++); printf("Stasiun Tujuan   : %s", ui_stasiun_nama_by_id(j->id_stasiun_tujuan));
    gotoXY(x, y++); printf("Waktu Berangkat  : %s", j->waktu_berangkat);
    gotoXY(x, y++); printf("Waktu Tiba       : %s", j->waktu_tiba);
    {
        char harga_s[64];
        format_money_rp_int(j->harga_tiket, harga_s, (int)sizeof(harga_s));
        gotoXY(x, y++); printf("Harga Tiket      : %s", harga_s);
    }
    gotoXY(x, y++); printf("Kuota Kursi      : %d", j->kuota_kursi);
    gotoXY(x, y++); printf("Pembuat (Akun)   : %s", j->id_karyawan);
    gotoXY(x, y++); printf("Status           : %s", j->active ? "Aktif" : "Nonaktif");

    gotoXY(pop_x + 3, pop_y + pop_h - 3);
    print_padded(pop_x + 3, pop_y + pop_h - 3, pop_w - 6, "Tekan tombol apa saja...");
    _getch();
}

static void jadwal_popup_edit(int split_x, int content_w, int idx) {
    if (idx < 0 || idx >= g_jadwalCount) return;

    int pop_w = 0, pop_h = 26;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Edit Jadwal Tiket");
    ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    JadwalTiket *j = &g_jadwal[idx];

    char id_kereta[16], asal[16], tujuan[16], brg[32], tiba[32], harga_buf[16], kuota_buf[16];

    snprintf(id_kereta, sizeof(id_kereta), "%s", j->id_kereta);
    snprintf(asal, sizeof(asal), "%s", j->id_stasiun_asal);
    snprintf(tujuan, sizeof(tujuan), "%s", j->id_stasiun_tujuan);
    snprintf(brg, sizeof(brg), "%s", j->waktu_berangkat);
    snprintf(tiba, sizeof(tiba), "%s", j->waktu_tiba);
    snprintf(harga_buf, sizeof(harga_buf), "%d", j->harga_tiket);
    snprintf(kuota_buf, sizeof(kuota_buf), "%d", j->kuota_kursi);

    const int LABEL_W = 34;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    /* Layout adaptif: jika panel bawah kecil, rapatkan agar field terakhir tidak tertimpa pesan */
    int step = 2;
    if (pop_h < 23) step = 1;

    int y_id      = pop_y + 2;
    int y_kereta  = y_id + step;
    int y_asal    = y_kereta + step;
    int y_tujuan  = y_asal + step;
    int y_brg     = y_tujuan + step;
    int y_tiba    = y_brg + step;
    int y_harga   = y_tiba + step;
    int y_kuota   = y_harga + step;

    /* pastikan kuota tidak menimpa baris pesan/peringatan */
    {
        int max_field_y = pop_y + pop_h - 6;
        if (y_kuota > max_field_y) {
            step = 1;
            y_id      = pop_y + 2;
            y_kereta  = y_id + step;
            y_asal    = y_kereta + step;
            y_tujuan  = y_asal + step;
            y_brg     = y_tujuan + step;
            y_tiba    = y_brg + step;
            y_harga   = y_tiba + step;
            y_kuota   = y_harga + step;
        }
    }

    gotoXY(label_x, y_id);      printf("%-*s: %s", LABEL_W, "ID Jadwal", j->id_jadwal);
    gotoXY(label_x, y_kereta);  printf("%-*s: ", LABEL_W, "ID Kereta (contoh: KA01)");
    gotoXY(label_x, y_asal);    printf("%-*s: ", LABEL_W, "ID Stasiun Asal (contoh: STS001)");
    gotoXY(label_x, y_tujuan);  printf("%-*s: ", LABEL_W, "ID Stasiun Tujuan");
    gotoXY(label_x, y_brg);     printf("%-*s: ", LABEL_W, "Waktu Berangkat (DD-MM-YYYY HH:MM)");
    gotoXY(label_x, y_tiba);    printf("%-*s: ", LABEL_W, "Waktu Tiba (DD-MM-YYYY HH:MM)");
    gotoXY(label_x, y_harga);   printf("%-*s: ", LABEL_W, "Harga Tiket (Rp)");
    gotoXY(label_x, y_kuota);   printf("%-*s: ", LABEL_W, "Kuota Kursi (angka)");

    /* ESC harus langsung keluar dari edit */
    while (1) {
        gotoXY(field_x, y_kereta);
        input_text_prefill_filtered(id_kereta, sizeof(id_kereta), 10);
        if ((unsigned char)id_kereta[0] == 27) return;
        if (!ui_require_not_blank(id_kereta, pop_x, pop_y, pop_w, pop_h, "ID Kereta")) continue;
        break;
    }
    while (1) {
        gotoXY(field_x, y_asal);
        input_text_prefill_filtered(asal, sizeof(asal), 10);
        if ((unsigned char)asal[0] == 27) return;
        if (!ui_require_not_blank(asal, pop_x, pop_y, pop_w, pop_h, "ID Stasiun Asal")) continue;
        break;
    }
    while (1) {
        gotoXY(field_x, y_tujuan);
        input_text_prefill_filtered(tujuan, sizeof(tujuan), 10);
        if ((unsigned char)tujuan[0] == 27) return;
        if (!ui_require_not_blank(tujuan, pop_x, pop_y, pop_w, pop_h, "ID Stasiun Tujuan")) continue;
        break;
    }
    while (1) {
        gotoXY(field_x, y_brg);
        input_datetime_ddmmyyyy_hhmm_prefill_filtered(brg, sizeof(brg));
        if ((unsigned char)brg[0] == 27) return;
        if (!ui_require_not_blank(brg, pop_x, pop_y, pop_w, pop_h, "Waktu Berangkat")) continue;
        if (!looks_like_datetime_ddmmyyyy_hhmm(brg)) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Format Waktu Berangkat tidak valid.");
            continue;
        }
        break;
    }
    while (1) {
        gotoXY(field_x, y_tiba);
        input_datetime_ddmmyyyy_hhmm_prefill_filtered(tiba, sizeof(tiba));
        if ((unsigned char)tiba[0] == 27) return;
        if (!ui_require_not_blank(tiba, pop_x, pop_y, pop_w, pop_h, "Waktu Tiba")) continue;
        if (!looks_like_datetime_ddmmyyyy_hhmm(tiba)) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Format Waktu Tiba tidak valid.");
            continue;
        }
        break;
    }
    while (1) {
        gotoXY(field_x, y_harga);
        printf("Rp ");
        gotoXY(field_x + 3, y_harga);
        input_text_prefill_filtered(harga_buf, sizeof(harga_buf), 10);
        if ((unsigned char)harga_buf[0] == 27) return;
        if (!ui_require_not_blank(harga_buf, pop_x, pop_y, pop_w, pop_h, "Harga Tiket")) continue;
        if (atoi(harga_buf) <= 0) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Harga Tiket harus > 0.");
            continue;
        }
        break;
    }
    while (1) {
        gotoXY(field_x, y_kuota);
        input_text_prefill_filtered(kuota_buf, sizeof(kuota_buf), 10);
        if ((unsigned char)kuota_buf[0] == 27) return;
        if (!ui_require_not_blank(kuota_buf, pop_x, pop_y, pop_w, pop_h, "Kuota Kursi")) continue;
        if (atoi(kuota_buf) <= 0) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Kuota Kursi harus > 0.");
            continue;
        }
        break;
    }

    ui_form_message_clear(pop_x, pop_y, pop_w, pop_h);

    if (!kereta_exists_active(id_kereta)) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "ID Kereta tidak ditemukan/Nonaktif. Tekan tombol apa saja...");
        _getch();
        return;
    }
    if (!stasiun_exists_active_by_id(asal) || !stasiun_exists_active_by_id(tujuan) || strcmp(asal, tujuan) == 0) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "ID Stasiun Asal/Tujuan tidak valid (atau sama). Tekan tombol apa saja...");
        _getch();
        return;
    }
    if (!looks_like_datetime_ddmmyyyy_hhmm(brg) || !looks_like_datetime_ddmmyyyy_hhmm(tiba)) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "Format waktu tidak valid. Tekan tombol apa saja...");
        _getch();
        return;
    }

    int harga = atoi(harga_buf);
    int kuota = atoi(kuota_buf);
    if (harga <= 0 || kuota <= 0) {
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "Harga/Kuota harus > 0. Tekan tombol apa saja...");
        _getch();
        return;
    }

    jadwal_update(idx, id_kereta, asal, tujuan, brg, tiba, harga, kuota);

    ui_form_message(pop_x, pop_y, pop_w, pop_h, "Berhasil disimpan. Tekan tombol apa saja...");
    _getch();
}

static void jadwal_popup_delete_soft(int split_x, int content_w, int idx) {
    if (idx < 0 || idx >= g_jadwalCount) return;

    int pop_w = 0, pop_h = 12;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Hapus Jadwal (Soft Delete)");
ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Nonaktifkan jadwal %s ? (soft delete)", g_jadwal[idx].id_jadwal);

    gotoXY(pop_x + 3, pop_y + 5);
    print_padded(pop_x + 3, pop_y + 5, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            jadwal_delete(idx);
            gotoXY(pop_x + 3, pop_y + 7);
            print_padded(pop_x + 3, pop_y + 7, pop_w - 6, "Jadwal dinonaktifkan. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

/* ------------------- POPUP PEMBAYARAN ------------------- */

/* Form helper khusus transaksi: dipanggil ulang setelah user membuka menu referensi (F1/F2). */
static void pembayaran_draw_form_add(int pop_x, int pop_y, int pop_w, int pop_h) {
    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Transaksi Pembayaran Tiket");
    /* Actions + helper shortcut (khusus transaksi) */
    ui_draw_actions_bar(pop_x, pop_y, pop_w, pop_h,
                        "Aksi: [ENTER] Simpan   [ESC] Batal  |  [F1] Penumpang  [F2] Jadwal");

    const int LABEL_W = 34;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: ", LABEL_W, "ID Penumpang (contoh: PNP001)");
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "ID Jadwal (contoh: JDW001)");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "Jumlah Tiket (angka)");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "Metode (1=TUNAI 2=DEBIT 3=QRIS)");

    /* Area input akan muncul saat user sedang menginput (dibuat oleh fungsi input_*). */
}

/* ------------------- POPUP JADWAL (TAMBAH) ------------------- */
/* Helper repaint untuk form tambah jadwal, dipakai saat user membuka menu referensi (F1/F2). */
static void jadwal_draw_form_add(int pop_x, int pop_y, int pop_w, int pop_h) {
    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Tambah Jadwal Tiket");
    /* Shortcut lookup khusus form jadwal */
    ui_draw_actions_bar(pop_x, pop_y, pop_w, pop_h,
                        "Aksi: [ENTER] Simpan   [ESC] Batal  |  [F1] Kereta  [F2] Stasiun");

    const int LABEL_W = 34;
    int label_x = pop_x + 3;

    gotoXY(label_x, pop_y + 2);  printf("%-*s: ", LABEL_W, "ID Kereta (contoh: KA01)");
    gotoXY(label_x, pop_y + 4);  printf("%-*s: ", LABEL_W, "ID Stasiun Asal (contoh: STS001)");
    gotoXY(label_x, pop_y + 6);  printf("%-*s: ", LABEL_W, "ID Stasiun Tujuan");
    gotoXY(label_x, pop_y + 8);  printf("%-*s: ", LABEL_W, "Waktu Berangkat (DD-MM-YYYY HH:MM)");
    gotoXY(label_x, pop_y + 10); printf("%-*s: ", LABEL_W, "Waktu Tiba (DD-MM-YYYY HH:MM)");
    gotoXY(label_x, pop_y + 12); printf("%-*s: ", LABEL_W, "Harga Tiket (Rp)");
    gotoXY(label_x, pop_y + 14); printf("%-*s: ", LABEL_W, "Kuota Kursi (angka)");

    /* Input box akan muncul hanya saat field sedang diinput (dibuat oleh fungsi input_*). */
}


// Forward declaration (needed for F2 lookup in transaksi)
static void view_jadwal_tiket(int account_index);
static void pembayaran_popup_add(int account_index, int split_x, int content_w) {
    int pop_w = 0, pop_h = 26;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    pembayaran_draw_form_add(pop_x, pop_y, pop_w, pop_h);

    char id_penumpang[16], id_jadwal[20], qty_buf[16], metode_in[8];
    char metode[20];

    const int LABEL_W = 34;
    int label_x = pop_x + 3;
    int field_x = label_x + LABEL_W + 2;

reinput_penumpang:
    /* ---- Input ID Penumpang (WAJIB) dengan referensi ---- */
    while (1) {
        gotoXY(field_x, pop_y + 2);
        int r = input_text_filtered_fkeys(id_penumpang, sizeof(id_penumpang), 10);
        if (r == -1 || id_penumpang[0] == 27) return;
        if (r == 1) { /* F1 */
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_penumpang();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;

            ui_redraw_base_simple("Transaksi Pembayaran Tiket (CRD)");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            pembayaran_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (r == 2) { /* F2 */
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_jadwal_tiket(account_index);
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;

            ui_redraw_base_simple("Transaksi Pembayaran Tiket (CRD)");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            pembayaran_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (!ui_require_not_blank(id_penumpang, pop_x, pop_y, pop_w, pop_h, "ID Penumpang")) {
            continue;
        }
        break;
    }

reinput_jadwal:
    /* ---- Input ID Jadwal (WAJIB) dengan referensi ---- */
    while (1) {
        gotoXY(field_x, pop_y + 4);
        int r = input_text_filtered_fkeys(id_jadwal, sizeof(id_jadwal), 10);
        if (r == -1 || id_jadwal[0] == 27) return;
        if (r == 1) {
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_penumpang();
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;
            ui_redraw_base_simple("Transaksi Pembayaran Tiket (CRD)");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            pembayaran_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (r == 2) {
            int bx=g_bottom_panel_x, by=g_bottom_panel_y, bw=g_bottom_panel_w, bh=g_bottom_panel_h;
            view_jadwal_tiket(account_index);
            g_bottom_panel_x=bx; g_bottom_panel_y=by; g_bottom_panel_w=bw; g_bottom_panel_h=bh;
            ui_redraw_base_simple("Transaksi Pembayaran Tiket (CRD)");
            popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);
            pembayaran_draw_form_add(pop_x, pop_y, pop_w, pop_h);
            continue;
        }
        if (!ui_require_not_blank(id_jadwal, pop_x, pop_y, pop_w, pop_h, "ID Jadwal")) {
            continue;
        }
        break;
    }

    /* ---- Qty (WAJIB) ---- */
    while (1) {
        gotoXY(field_x, pop_y + 6);
        input_digits_only_filtered(qty_buf, sizeof(qty_buf), 10);
        if (qty_buf[0] == 27) return;
        if (!ui_require_not_blank(qty_buf, pop_x, pop_y, pop_w, pop_h, "Jumlah Tiket")) continue;
        if (atoi(qty_buf) <= 0) {
            Beep(700, 80);
            ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Jumlah tiket harus > 0.");
            continue;
        }
        break;
    }

    /* ---- Metode (WAJIB) ---- */
    while (1) {
        gotoXY(field_x, pop_y + 8);
        input_choice_digits_filtered(metode_in, sizeof(metode_in), "123", 0);
        if (metode_in[0] == 27) return;
        if (!ui_require_not_blank(metode_in, pop_x, pop_y, pop_w, pop_h, "Metode")) continue;
        break;
    }

    ui_form_message_clear(pop_x, pop_y, pop_w, pop_h);

    if (!penumpang_exists_active_by_id(id_penumpang)) {
        Beep(700, 80);
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Penumpang tidak ditemukan/Nonaktif. Isi ulang ID Penumpang.");
        goto reinput_penumpang;
    }
    int jidx = jadwal_find_index_by_id(id_jadwal);
    if (jidx < 0 || !g_jadwal[jidx].active) {
        Beep(700, 80);
        ui_form_message(pop_x, pop_y, pop_w, pop_h, "[PERINGATAN] Jadwal tidak ditemukan/Nonaktif. Isi ulang ID Jadwal.");
        goto reinput_jadwal;
    }

    int qty = atoi(qty_buf);

    if (metode_in[0] == '1') snprintf(metode, sizeof(metode), "TUNAI");
    else if (metode_in[0] == '2') snprintf(metode, sizeof(metode), "DEBIT");
    else snprintf(metode, sizeof(metode), "QRIS");

    char id_new[32];
    char err[128];
    const char *kry = (account_index >= 0 && account_index < g_accountCount) ? g_accounts[account_index].id_karyawan : "";

    int ok = pembayaran_create_auto(id_new, sizeof(id_new),
                                   "",
                                   id_penumpang, id_jadwal, qty,
                                   metode, "LUNAS",
                                   kry,
                                   err, sizeof(err));
    if (!ok) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Gagal: %s. Tekan tombol apa saja...", (err[0] ? err : "unknown"));
        ui_form_message(pop_x, pop_y, pop_w, pop_h, msg);
        _getch();
        return;
    }

    /* cari record baru untuk tampil total */
    int pidx = pembayaran_find_index_by_id(id_new);
    int total = (pidx >= 0) ? g_pembayaran[pidx].total_harga : 0;

    {
        char total_s[64];
        char msg[256];
        format_money_rp_int(total, total_s, (int)sizeof(total_s));
        snprintf(msg, sizeof(msg), "Transaksi sukses. ID=%s Total=%s. Tekan tombol apa saja...", id_new, total_s);
        ui_form_message(pop_x, pop_y, pop_w, pop_h, msg);
    }
    _getch();
}

static void pembayaran_popup_detail(int split_x, int content_w, int idx) {
    if (idx < 0 || idx >= g_pembayaranCount) return;
    int pop_w = 0, pop_h = 24;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

    draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Rincian Pembayaran");

    PembayaranTiket *p = &g_pembayaran[idx];

    const int LABEL_W = 18;
    int x = pop_x + 3;
    int y = pop_y + 1;

    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "ID Pembayaran", p->id_pembayaran);
    {
        char tgl_human[32];
        format_compact_date_id(p->tgl_pembayaran, tgl_human, (int)sizeof(tgl_human));
        gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Tgl Dibuat", tgl_human);
    }
    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Penumpang", ui_penumpang_nama_by_id(p->id_penumpang));
    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Tanggal Jadwal", ui_jadwal_tgl_by_id(p->id_jadwal));
    gotoXY(x, y++); printf("%-*s : %d", LABEL_W, "Jumlah Tiket", p->jumlah_tiket);
    {
        char hs[64], tt[64];
        format_money_rp_int(p->harga_satuan, hs, (int)sizeof(hs));
        format_money_rp_int(p->total_harga, tt, (int)sizeof(tt));
        gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Harga Satuan", hs);
        gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Total Harga", tt);
    }
    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Metode Pembayaran", p->metode_pembayaran);
    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Status Pembayaran", p->status_pembayaran);
    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Petugas (Akun)", p->id_karyawan);
    gotoXY(x, y++); printf("%-*s : %s", LABEL_W, "Status Data", p->active ? "Aktif" : "Nonaktif");

    gotoXY(pop_x + 3, pop_y + pop_h - 3);
    print_padded(pop_x + 3, pop_y + pop_h - 3, pop_w - 6, "Tekan tombol apa saja...");
    _getch();
}
static void pembayaran_popup_delete_soft(int split_x, int content_w, int idx) {
    if (idx < 0 || idx >= g_pembayaranCount) return;

    int pop_w = 0, pop_h = 12;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Hapus Pembayaran (Soft Delete)");
ui_actions_confirm_cancel(pop_x, pop_y, pop_w, pop_h);

    gotoXY(pop_x + 3, pop_y + 3);
    printf("Nonaktifkan transaksi %s ? (soft delete)", g_pembayaran[idx].id_pembayaran);

    gotoXY(pop_x + 3, pop_y + 5);
    print_padded(pop_x + 3, pop_y + 5, pop_w - 6, "Tekan [ENTER] untuk konfirmasi atau [ESC] untuk batal...");

    while (1) {
        int c = _getch();
        if (c == 27) return;
        if (c == 13) {
            pembayaran_delete(idx);
            gotoXY(pop_x + 3, pop_y + 7);
            print_padded(pop_x + 3, pop_y + 7, pop_w - 6, "Transaksi dinonaktifkan. Tekan [ESC] untuk kembali...");
            while (_getch() != 27) {}
            return;
        }
    }
}

/* ------------------- VIEW JADWAL ------------------- */

static void view_jadwal_tiket(int account_index) {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO=3, W_TGL=12, W_KER=6, W_ASAL=8, W_TUJUAN=8, W_BRG=16, W_TIBA=16, W_HRG=10, W_KUO=7, W_ST=10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
        W_NO=3; W_TGL=12; W_KER=7; W_ASAL=9; W_TUJUAN=9; W_BRG=16; W_TIBA=16; W_HRG=10; W_KUO=7; W_ST=10;
        int col_w[] = {W_NO,W_TGL,W_KER,W_ASAL,W_TUJUAN,W_BRG,W_TIBA,W_HRG,W_KUO,W_ST};
        const int min_w[] = {3,10,5,6,6,16,16,6,5,6};
        fit_columns(col_w, min_w, 10, usable_w);
        W_NO=col_w[0]; W_TGL=col_w[1]; W_KER=col_w[2]; W_ASAL=col_w[3]; W_TUJUAN=col_w[4];
        W_BRG=col_w[5]; W_TIBA=col_w[6]; W_HRG=col_w[7]; W_KUO=col_w[8]; W_ST=col_w[9];

        int total = g_jadwalCount;
        int idxs[MAX_RECORDS];
        for (int i = 0; i < total; i++) idxs[i] = i;
        if (total > 1) qsort(idxs, (size_t)total, sizeof(int), jadwal_cmp_idx);
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
        draw_layout_base(w, h, "Kelola Jadwal Tiket");

        int table_w = table_total_width(col_w, 10);
        int table_x = content_left;
        int table_y = 10;

        jadwal_print_hline(table_x, table_y, W_NO, W_TGL, W_KER, W_ASAL, W_TUJUAN, W_BRG, W_TIBA, W_HRG, W_KUO, W_ST);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No",
               W_TGL, "Tanggal",
               W_KER, "Kereta",
               W_ASAL, "Asal",
               W_TUJUAN, "Tujuan",
               W_BRG, "Berangkat",
               W_TIBA, "Tiba",
               W_HRG, "Harga",
               W_KUO, "Kuota",
               W_ST, "Status");
        jadwal_print_hline(table_x, table_y + 2, W_NO, W_TGL, W_KER, W_ASAL, W_TUJUAN, W_BRG, W_TIBA, W_HRG, W_KUO, W_ST);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;
            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int pos = start + r;
                int n = idxs[pos];
                const char *status = g_jadwal[n].active ? "Aktif" : "Nonaktif";
                char harga_s[32];
                char kuota_s[32];
                format_money_rp_int(g_jadwal[n].harga_tiket, harga_s, (int)sizeof(harga_s));
                snprintf(kuota_s, sizeof(kuota_s), "%d", g_jadwal[n].kuota_kursi);

                char tgl_brkt[16] = "";
                if ((int)strlen(g_jadwal[n].waktu_berangkat) >= 10) {
                    strncpy(tgl_brkt, g_jadwal[n].waktu_berangkat, 10);
                    tgl_brkt[10] = '\0';
                } else {
                    strncpy(tgl_brkt, g_jadwal[n].waktu_berangkat, sizeof(tgl_brkt)-1);
                    tgl_brkt[sizeof(tgl_brkt)-1] = '\0';
                }

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %*.*s | %*.*s | %-*.*s |",
                       W_NO, pos + 1,
                       W_TGL, W_TGL, tgl_brkt,
                       /* tampilkan NAMA master, input tetap ID */
                       W_KER, W_KER, ui_kereta_nama_by_kode(g_jadwal[n].id_kereta),
                       W_ASAL, W_ASAL, ui_stasiun_nama_by_id(g_jadwal[n].id_stasiun_asal),
                       W_TUJUAN, W_TUJUAN, ui_stasiun_nama_by_id(g_jadwal[n].id_stasiun_tujuan),
                       W_BRG, W_BRG, g_jadwal[n].waktu_berangkat,
                       W_TIBA, W_TIBA, g_jadwal[n].waktu_tiba,
                       W_HRG, W_HRG, harga_s,
                       W_KUO, W_KUO, kuota_s,
                       W_ST, W_ST, status);
            } else {
                gotoXY(table_x, y);
                printf("| %*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
                       W_NO, "",
                       W_TGL, "",
                       W_KER, "",
                       W_ASAL, "",
                       W_TUJUAN, "",
                       W_BRG, "",
                       W_TIBA, "",
                       W_HRG, "",
                       W_KUO, "",
                       W_ST, "");
            }
            set_highlight(0);
        }
        jadwal_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_TGL, W_KER, W_ASAL, W_TUJUAN, W_BRG, W_TIBA, W_HRG, W_KUO, W_ST);

        int info_y = table_y + 5 + ROWS_PER_PAGE;
        gotoXY(table_x, info_y);
        printf("Halaman %d/%d  Total: %d", page + 1, total_pages, total);

        gotoXY(table_x, info_y + 1);
        printf("Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian  [A] Tambah  [E] Edit  [D] Hapus  [ESC] Kembali");

        /* Siapkan panel bawah untuk form/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, info_y + 1, table_w, h);

        int ch = _getch();
        if (ch == 27) break; /* ESC */

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { if (rows_on_page > 0 && selected > 0) selected--; }        /* UP */
            else if (ext == 80) { if (rows_on_page > 0 && selected < rows_on_page - 1) selected++; } /* DOWN */
            else if (ext == 75) { if (page > 0) { page--; selected = 0; } }            /* LEFT */
            else if (ext == 77) { if (page < total_pages - 1) { page++; selected = 0; } } /* RIGHT */
        } else {
            char c = (char)tolower(ch);
            int idx = (rows_on_page > 0) ? idxs[start + selected] : -1;

            if (ch == 13) { /* Enter detail */
                if (idx >= 0) jadwal_popup_detail(split_x, content_w, idx);
            } else if (c == 'a') {
                jadwal_popup_add(account_index, split_x, content_w);
            } else if (c == 'e') {
                if (idx >= 0) jadwal_popup_edit(split_x, content_w, idx);
            } else if (c == 'd') {
                if (idx >= 0) jadwal_popup_delete_soft(split_x, content_w, idx);
            }
        }
    }
}

/* ------------------- VIEW PEMBAYARAN ------------------- */

static void view_pembayaran_tiket(int account_index) {
    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int W_NO=3, W_TGL=12, W_PNP=10, W_JDW=8, W_QTY=5, W_TOT=12, W_MET=10, W_STS=10;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);
        W_NO=3; W_TGL=12; W_PNP=10; W_JDW=8; W_QTY=5; W_TOT=12; W_MET=10; W_STS=10;
        int col_w[] = {W_NO,W_TGL,W_PNP,W_JDW,W_QTY,W_TOT,W_MET,W_STS};
        const int min_w[] = {3,10,8,6,3,8,6,6};
        fit_columns(col_w, min_w, 8, usable_w);
        W_NO=col_w[0]; W_TGL=col_w[1]; W_PNP=col_w[2]; W_JDW=col_w[3];
        W_QTY=col_w[4]; W_TOT=col_w[5]; W_MET=col_w[6]; W_STS=col_w[7];

        int total = g_pembayaranCount;
        int idxs[MAX_RECORDS];
        for (int i = 0; i < total; i++) idxs[i] = i;
        if (total > 1) qsort(idxs, (size_t)total, sizeof(int), pembayaran_cmp_idx);
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
        draw_layout_base(w, h, "Transaksi Pembayaran Tiket (CRD)");

        int table_w = table_total_width(col_w, 8);
        int table_x = content_left;
        int table_y = 10;

        pembayaran_print_hline(table_x, table_y, W_NO, W_TGL, W_PNP, W_JDW, W_QTY, W_TOT, W_MET, W_STS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No",
               W_TGL, "Tanggal",
               W_PNP, "Penumpang",
               W_JDW, "TglJdw",
               W_QTY, "Qty",
               W_TOT, "Total",
               W_MET, "Metode",
               W_STS, "Status");
        pembayaran_print_hline(table_x, table_y + 2, W_NO, W_TGL, W_PNP, W_JDW, W_QTY, W_TOT, W_MET, W_STS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;
            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int pos = start + r;
                int n = idxs[pos];
                const char *sts = g_pembayaran[n].active ? g_pembayaran[n].status_pembayaran : "Nonaktif";
                char pnp_s[32];
                char jdw_s[32];
                char total_s[32];
                snprintf(pnp_s, sizeof(pnp_s), "%s", ui_penumpang_nama_by_id(g_pembayaran[n].id_penumpang));
                snprintf(jdw_s, sizeof(jdw_s), "%s", ui_jadwal_tgl_by_id(g_pembayaran[n].id_jadwal));
                format_money_rp_int(g_pembayaran[n].total_harga, total_s, (int)sizeof(total_s));

                char tgl_trx[16];
                format_compact_date_id(g_pembayaran[n].tgl_pembayaran, tgl_trx, (int)sizeof(tgl_trx));
                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %*d | %*.*s | %-*.*s | %-*.*s |",
                       W_NO, pos + 1,
                       W_TGL, W_TGL, tgl_trx,
                       W_PNP, W_PNP, pnp_s,
                       W_JDW, W_JDW, jdw_s,
                       W_QTY, g_pembayaran[n].jumlah_tiket,
                       W_TOT, W_TOT, total_s,
                       W_MET, W_MET, g_pembayaran[n].metode_pembayaran,
                       W_STS, W_STS, sts);
            } else {
                gotoXY(table_x, y);
                printf("| %*s | %-*s | %-*s | %-*s | %*s | %*s | %-*s | %-*s |",
                       W_NO, "",
                       W_TGL, "",
                       W_PNP, "",
                       W_JDW, "",
                       W_QTY, "",
                       W_TOT, "",
                       W_MET, "",
                       W_STS, "");}
            set_highlight(0);
        }
        pembayaran_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_TGL, W_PNP, W_JDW, W_QTY, W_TOT, W_MET, W_STS);

        int info_y = table_y + 5 + ROWS_PER_PAGE;
        gotoXY(table_x, info_y);
        printf("Halaman %d/%d  Total: %d", page + 1, total_pages, total);

        gotoXY(table_x, info_y + 1);
        printf("Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian  [A] Transaksi Baru  [D] Hapus  [ESC] Kembali");

        /* Siapkan panel bawah untuk form/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, info_y + 1, table_w, h);

        int ch = _getch();
        if (ch == 27) break; /* ESC */

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { if (rows_on_page > 0 && selected > 0) selected--; }        /* UP */
            else if (ext == 80) { if (rows_on_page > 0 && selected < rows_on_page - 1) selected++; } /* DOWN */
            else if (ext == 75) { if (page > 0) { page--; selected = 0; } }            /* LEFT */
            else if (ext == 77) { if (page < total_pages - 1) { page++; selected = 0; } } /* RIGHT */
        } else {
            char c = (char)tolower(ch);
            int idx = (rows_on_page > 0) ? idxs[start + selected] : -1;

            if (ch == 13) { /* Enter detail */
                if (idx >= 0) pembayaran_popup_detail(split_x, content_w, idx);
            } else if (c == 'a') {
                pembayaran_popup_add(account_index, split_x, content_w);
            } else if (c == 'd') {
                if (idx >= 0) pembayaran_popup_delete_soft(split_x, content_w, idx);
            }
        }
    }
}




/* ------------------- LAPORAN TRANSAKSI (MANAGER) ------------------- */

typedef enum {
    REPORT_SORT_TANGGAL = 0,
    REPORT_SORT_TOTAL   = 1,
    REPORT_SORT_QTY     = 2
} ReportSortKey;

static int report_parse_ddmmyyyy(const char *s, int *d, int *m, int *y) {
    if (!s || !*s) return 0;
    int dd = 0, mm = 0, yy = 0;
    /* format wajib: DD-MM-YYYY */
    if (sscanf(s, "%d-%d-%d", &dd, &mm, &yy) != 3) return 0;
    if (dd < 1 || dd > 31) return 0;
    if (mm < 1 || mm > 12) return 0;
    if (yy < 1900 || yy > 3000) return 0;
    if (d) *d = dd;
    if (m) *m = mm;
    if (y) *y = yy;
    return 1;
}

/* Resolve label untuk laporan (READ-ONLY):
   - Penumpang: tampilkan NAMA, bukan ID
   - Jadwal: tampilkan tanggal keberangkatan (DD-MM-YYYY), bukan ID jadwal
*/
static void report_get_penumpang_label(const char *id_penumpang, char *out, int out_sz) {
    if (!out || out_sz <= 0) return;
    out[0] = '\0';

    if (!id_penumpang || id_penumpang[0] == '\0') {
        snprintf(out, (size_t)out_sz, "-");
        return;
    }

    for (int i = 0; i < g_penumpangCount; i++) {
        if (!g_penumpang[i].active) continue;
        if (strcmp(g_penumpang[i].id, id_penumpang) == 0) {
            snprintf(out, (size_t)out_sz, "%s", g_penumpang[i].nama);
            return;
        }
    }

    /* fallback: tampilkan ID kalau tidak ketemu */
    snprintf(out, (size_t)out_sz, "%s", id_penumpang);
}

static void report_get_jadwal_date_label(const char *id_jadwal, char *out, int out_sz) {
    if (!out || out_sz <= 0) return;
    out[0] = '\0';

    if (!id_jadwal || id_jadwal[0] == '\0') {
        snprintf(out, (size_t)out_sz, "-");
        return;
    }

    int jidx = jadwal_find_index_by_id(id_jadwal);
    if (jidx >= 0) {
        /* Ambil tanggal dari waktu_berangkat: "DD-MM-YYYY HH:MM" */
        const char *wb = g_jadwal[jidx].waktu_berangkat;
        if (wb && (int)strlen(wb) >= 10) {
            char tmp[16];
            memcpy(tmp, wb, 10);
            tmp[10] = '\0';
            snprintf(out, (size_t)out_sz, "%s", tmp);
            return;
        }
    }

    /* fallback: tampilkan ID jadwal kalau tidak ketemu */
    snprintf(out, (size_t)out_sz, "%s", id_jadwal);
}

static int report_date_key_ddmmyyyy(const char *s) {
    int dd, mm, yy;
    if (!report_parse_ddmmyyyy(s, &dd, &mm, &yy)) return 0;
    return (yy * 10000) + (mm * 100) + dd; /* YYYYMMDD */
}

static void report_get_current_month_year(int *out_month, int *out_year) {
    if (out_month) *out_month = 1;
    if (out_year)  *out_year  = 2026;

    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    if (!tmv) return;

    if (out_month) *out_month = tmv->tm_mon + 1;
    if (out_year)  *out_year  = tmv->tm_year + 1900;
}

static int g_report_sort_key = REPORT_SORT_TANGGAL;
static int g_report_sort_desc = 0;

static int report_cmp_pembayaran_idx(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;

    const PembayaranTiket *pa = &g_pembayaran[ia];
    const PembayaranTiket *pb = &g_pembayaran[ib];

    long va = 0, vb = 0;

    if (g_report_sort_key == REPORT_SORT_TOTAL) {
        va = (long)pa->total_harga;
        vb = (long)pb->total_harga;
    } else if (g_report_sort_key == REPORT_SORT_QTY) {
        va = (long)pa->jumlah_tiket;
        vb = (long)pb->jumlah_tiket;
    } else { /* REPORT_SORT_TANGGAL */
        va = (long)report_date_key_ddmmyyyy(pa->tgl_dibuat);
        vb = (long)report_date_key_ddmmyyyy(pb->tgl_dibuat);
    }

    if (va < vb) return g_report_sort_desc ?  1 : -1;
    if (va > vb) return g_report_sort_desc ? -1 :  1;

    /* tie-break: ID transaksi (ikut arah sort supaya konsisten) */
    int t = strcmp(pa->id_pembayaran, pb->id_pembayaran);
    return g_report_sort_desc ? -t : t;
}

static void laporan_print_hline(int x, int y,
                                int W_NO, int W_TGL, int W_TRX, int W_PNP, int W_JDW,
                                int W_QTY, int W_TOT, int W_MET, int W_STS) {
    gotoXY(x, y);
    printf("|");
    for (int i = 0; i < W_NO + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TGL + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TRX + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_PNP + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_JDW + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_QTY + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_TOT + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_MET + 2; i++) printf("=");
    printf("|");
    for (int i = 0; i < W_STS + 2; i++) printf("=");
    printf("|");
}

static int laporan_popup_filter(int split_x, int content_w, int *io_month, int *io_year) {
    int pop_w = 0, pop_h = 12;
    int pop_x, pop_y;
    popup_content_clamped(split_x, pop_h, &pop_x, &pop_y, &pop_w, &pop_h);

draw_popup_box(pop_x, pop_y, pop_w, pop_h, "Saring Laporan (Bulan & Tahun)");
ui_actions_save_cancel(pop_x, pop_y, pop_w, pop_h);

    int label_x = pop_x + 3;
    int field_x = label_x + 18;

    gotoXY(label_x, pop_y + 2); printf("Bulan (0=Semua): ");
    gotoXY(label_x, pop_y + 4); printf("Tahun (YYYY)  : ");

    gotoXY(label_x, pop_y + 7); printf("Catatan: tekan [ESC] untuk batal.");

    char m_buf[8], y_buf[8];
    memset(m_buf, 0, sizeof(m_buf));
    memset(y_buf, 0, sizeof(y_buf));

    /* tampilkan nilai saat ini sebagai referensi */
    if (io_month && io_year) {
        gotoXY(label_x, pop_y + 9);
        printf("Saring saat ini: %02d-%04d", *io_month, *io_year);
    }

    gotoXY(field_x, pop_y + 2);
    input_digits_only_filtered(m_buf, (int)sizeof(m_buf), 2);
    if (m_buf[0] == 27) return 0;

    gotoXY(field_x, pop_y + 4);
    input_digits_only_filtered(y_buf, (int)sizeof(y_buf), 4);
    if (y_buf[0] == 27) return 0;

    int m = atoi(m_buf);
    int y = atoi(y_buf);

    if (m < 0 || m > 12 || y < 1900 || y > 3000) {
        Beep(500, 180);
        gotoXY(label_x, pop_y + 10);
        printf("Input tidak valid. Bulan 0-12, Tahun 1900-3000.");
        Sleep(900);
        return 0;
    }

    if (io_month) *io_month = m;
    if (io_year)  *io_year  = y;
    return 1;
}

static const char* report_sort_name(int key) {
    if (key == REPORT_SORT_TOTAL) return "Total";
    if (key == REPORT_SORT_QTY)   return "Qty";
    return "Tanggal";
}

static void view_laporan_transaksi(int account_index) {
    (void)account_index;

    int ROWS_PER_PAGE = ROWS_PER_PAGE_FIXED;
    int page = 0;
    int selected = 0;

    int filter_month = 0, filter_year = 0;
    report_get_current_month_year(&filter_month, &filter_year);

    /* default: urutkan by tanggal ASC */
    g_report_sort_key = REPORT_SORT_TANGGAL;
    g_report_sort_desc = 0;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 140;
        int h = get_screen_height(); if (h <= 0) h = 30;

        int split_x = layout_split_x(w);
        int content_w = w - split_x;

        int content_left, content_right, usable_w;
        layout_content_bounds(w, split_x, &content_left, &content_right, &usable_w);

        int table_x = content_left;
        int table_y = 12;
        /* build list index hasil filter */
        int idxs[MAX_RECORDS];
        int n = 0;

        long total_pendapatan = 0;
        long total_tiket = 0;
        int  total_trx = 0;

        /* rekap metode */
        long tunai_sum=0, debit_sum=0, qris_sum=0, other_sum=0;
        int  tunai_cnt=0, debit_cnt=0, qris_cnt=0, other_cnt=0;

        for (int i = 0; i < g_pembayaranCount && n < MAX_RECORDS; i++) {
            if (!g_pembayaran[i].active) continue;

            int dd, mm, yy;
            if (!report_parse_ddmmyyyy(g_pembayaran[i].tgl_dibuat, &dd, &mm, &yy)) continue;

            if (yy != filter_year) continue;
            if (filter_month != 0 && mm != filter_month) continue;

            idxs[n++] = i;

            total_trx++;
            total_tiket += g_pembayaran[i].jumlah_tiket;
            total_pendapatan += g_pembayaran[i].total_harga;

            if (strcmp(g_pembayaran[i].metode_pembayaran, "TUNAI") == 0) {
                tunai_cnt++; tunai_sum += g_pembayaran[i].total_harga;
            } else if (strcmp(g_pembayaran[i].metode_pembayaran, "DEBIT") == 0) {
                debit_cnt++; debit_sum += g_pembayaran[i].total_harga;
            } else if (strcmp(g_pembayaran[i].metode_pembayaran, "QRIS") == 0) {
                qris_cnt++; qris_sum += g_pembayaran[i].total_harga;
            } else {
                other_cnt++; other_sum += g_pembayaran[i].total_harga;
            }
        }

        if (n > 1) qsort(idxs, (size_t)n, sizeof(int), report_cmp_pembayaran_idx);

        int total_pages = (n + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
        if (total_pages < 1) total_pages = 1;
        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;

        int start = page * ROWS_PER_PAGE;
        int end = start + ROWS_PER_PAGE;
        if (end > n) end = n;

        int rows_on_page = end - start;
        if (rows_on_page < 0) rows_on_page = 0;
        if (rows_on_page == 0) selected = 0;
        if (selected >= rows_on_page) selected = (rows_on_page > 0) ? rows_on_page - 1 : 0;
        if (selected < 0) selected = 0;

        /* Lebarkan kolom Penumpang & TglJdw karena sekarang menampilkan nama & tanggal */
        int W_NO=3, W_TGL=10, W_TRX=8, W_PNP=16, W_JDW=10, W_QTY=3, W_TOT=12, W_MET=8, W_STS=8;
        int col_w[] = {W_NO,W_TGL,W_TRX,W_PNP,W_JDW,W_QTY,W_TOT,W_MET,W_STS};
        const int min_w[] = {3,10,6,8,6,3,8,6,6};
        fit_columns(col_w, min_w, 9, usable_w);
	    int table_w = table_total_width(col_w, 9);
        W_NO=col_w[0]; W_TGL=col_w[1]; W_TRX=col_w[2]; W_PNP=col_w[3]; W_JDW=col_w[4];
        W_QTY=col_w[5]; W_TOT=col_w[6]; W_MET=col_w[7]; W_STS=col_w[8];

        cls();
        draw_layout_base(w, h, "Laporan Transaksi Pembayaran (Rekap Bulanan)");

        gotoXY(table_x, 8);
        {
            char bulan_label[16];
            if (filter_month == 0) snprintf(bulan_label, sizeof(bulan_label), "Semua");
            else snprintf(bulan_label, sizeof(bulan_label), "%02d", filter_month);

            printf("Saring: Bulan %s  Tahun %04d    Urut: %s %s    Data: %d",
                   bulan_label, filter_year,
                   report_sort_name(g_report_sort_key),
                   g_report_sort_desc ? "TURUN" : "NAIK",
                   n);
        }

        gotoXY(table_x, 9);
        {
            char pend_s[64];
            format_money_rp_int((long long)total_pendapatan, pend_s, (int)sizeof(pend_s));
            printf("Rekap: Transaksi=%d  Tiket=%ld  Pendapatan=%s",
                   total_trx, total_tiket, pend_s);
        }

        gotoXY(table_x, 10);
        {
            char tunai_s[64], debit_s[64], qris_s[64], other_s[64];
            format_money_rp_int((long long)tunai_sum, tunai_s, (int)sizeof(tunai_s));
            format_money_rp_int((long long)debit_sum, debit_s, (int)sizeof(debit_s));
            format_money_rp_int((long long)qris_sum,  qris_s,  (int)sizeof(qris_s));
            format_money_rp_int((long long)other_sum, other_s, (int)sizeof(other_s));
            printf("Metode: Tunai %d/%s  Debit %d/%s  QRIS %d/%s  Lain %d/%s",
                   tunai_cnt, tunai_s,
                   debit_cnt, debit_s,
                   qris_cnt, qris_s,
                   other_cnt, other_s);
        }

        /* Table header */
        laporan_print_hline(table_x, table_y, W_NO, W_TGL, W_TRX, W_PNP, W_JDW, W_QTY, W_TOT, W_MET, W_STS);
        gotoXY(table_x, table_y + 1);
        printf("| %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s |",
               W_NO, "No",
               W_TGL, "Tanggal",
               W_TRX, "ID",
               W_PNP, "Penumpang",
               W_JDW, "TglJdw",
               W_QTY, "Qty",
               W_TOT, "Total",
               W_MET, "Metode",
               W_STS, "Status");
        laporan_print_hline(table_x, table_y + 2, W_NO, W_TGL, W_TRX, W_PNP, W_JDW, W_QTY, W_TOT, W_MET, W_STS);

        for (int r = 0; r < ROWS_PER_PAGE; r++) {
            int y = table_y + 3 + r;
            int is_selected = (r < rows_on_page && r == selected);
            set_highlight(is_selected);

            if (r < rows_on_page) {
                int idx = idxs[start + r];
                const char *sts = g_pembayaran[idx].active ? g_pembayaran[idx].status_pembayaran : "Nonaktif";

                char total_s[32];
                format_money_rp_int(g_pembayaran[idx].total_harga, total_s, (int)sizeof(total_s));

                /* tampilkan penumpang = nama (bukan ID) dan jadwal = tanggal berangkat (bukan ID jadwal) */
                char pnp_label[64];
                char jdw_label[32];
                report_get_penumpang_label(g_pembayaran[idx].id_penumpang, pnp_label, (int)sizeof(pnp_label));
                report_get_jadwal_date_label(g_pembayaran[idx].id_jadwal, jdw_label, (int)sizeof(jdw_label));

                gotoXY(table_x, y);
                printf("| %*d | %-*.*s | %-*.*s | %-*.*s | %-*.*s | %*d | %*.*s | %-*.*s | %-*.*s |",
                       W_NO, start + r + 1,
                       W_TGL, W_TGL, g_pembayaran[idx].tgl_dibuat,
                       W_TRX, W_TRX, g_pembayaran[idx].id_pembayaran,
                       W_PNP, W_PNP, pnp_label,
                       W_JDW, W_JDW, jdw_label,
                       W_QTY, g_pembayaran[idx].jumlah_tiket,
                       W_TOT, W_TOT, total_s,
                       W_MET, W_MET, g_pembayaran[idx].metode_pembayaran,
                       W_STS, W_STS, sts);
            } else {
                gotoXY(table_x, y);
                printf("| %*s | %-*s | %-*s | %-*s | %-*s | %*s | %-*s | %-*s | %-*s |",
                       W_NO, "",
                       W_TGL, "",
                       W_TRX, "",
                       W_PNP, "",
                       W_JDW, "",
                       W_QTY, "",
                       W_TOT, "",
                       W_MET, "",
                       W_STS, "");
            }
            set_highlight(0);
        }

        laporan_print_hline(table_x, table_y + 3 + ROWS_PER_PAGE, W_NO, W_TGL, W_TRX, W_PNP, W_JDW, W_QTY, W_TOT, W_MET, W_STS);

        int info_y = table_y + 5 + ROWS_PER_PAGE;
        gotoXY(table_x, info_y);
        printf("Halaman %d/%d", page + 1, total_pages);

        gotoXY(table_x, info_y + 1);
        printf("Navigasi: [ATAS/BAWAH] Pilih  [KIRI/KANAN] Halaman  [ENTER] Rincian  [F] Saring  [S] Urut  [R] NAIK/TURUN  [ESC] Kembali");

        /* Siapkan panel bawah untuk form/Detail (di bawah navigasi) */
        bottom_panel_prepare(table_x, info_y + 1, table_w, h);

        int ch = _getch();
        if (ch == 27) break; /* ESC */

        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) { if (rows_on_page > 0 && selected > 0) selected--; }              /* UP */
            else if (ext == 80) { if (rows_on_page > 0 && selected < rows_on_page - 1) selected++; } /* DOWN */
            else if (ext == 75) { if (page > 0) { page--; selected = 0; } }                  /* LEFT */
            else if (ext == 77) { if (page < total_pages - 1) { page++; selected = 0; } }    /* RIGHT */
        } else {
            char c = (char)tolower(ch);

            if (ch == 13) {
                if (rows_on_page > 0) {
                    int idx = idxs[start + selected];
                    pembayaran_popup_detail(split_x, content_w, idx);
                }
            } else if (c == 'f') {
                /* popup filter */
                laporan_popup_filter(split_x, content_w, &filter_month, &filter_year);
                page = 0; selected = 0;
            } else if (c == 's') {
                /* cycle sort key */
                if (g_report_sort_key == REPORT_SORT_TANGGAL) g_report_sort_key = REPORT_SORT_TOTAL;
                else if (g_report_sort_key == REPORT_SORT_TOTAL) g_report_sort_key = REPORT_SORT_QTY;
                else g_report_sort_key = REPORT_SORT_TANGGAL;
                page = 0; selected = 0;
            } else if (c == 'r') {
                g_report_sort_desc = !g_report_sort_desc;
                page = 0; selected = 0;
            }
        }
    }
}


static void dashboard_main(int account_index) {
    if (account_index < 0 || account_index >= g_accountCount) return;
    Account *me = &g_accounts[account_index];

    while(1) {
        int w = get_screen_width(); if(w<=0) w=120;
        int h = get_screen_height(); if(h<=0) h=30;

        cls();
        draw_layout_base(w, h, "Dasbor");

        int split_x = layout_split_x(w);
        int content_w = w - split_x;
        int center_x = split_x + (content_w / 2);
        int center_y = h / 2;

        char welcome[100];
        snprintf(welcome, 100, "Selamat Datang, %s!", (me->nama[0] ? me->nama : me->email));
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
            menus[menu_count++] = "[ESC] Keluar Akun / Keluar";
        } else if (me->role == ROLE_DATA) {
            menus[menu_count++] = "[1] Kelola Penumpang";
            menus[menu_count++] = "[2] Kelola Stasiun";
            menus[menu_count++] = "[3] Kelola Kereta";
            menus[menu_count++] = "[ESC] Keluar Akun / Keluar";
        } else if (me->role == ROLE_TRANSAKSI) {
            menus[menu_count++] = "[1] Transaksi Pembayaran Tiket";
            menus[menu_count++] = "[2] Kelola Jadwal";
            menus[menu_count++] = "[ESC] Keluar Akun / Keluar";
        } else if (me->role == ROLE_MANAGER) {
            menus[menu_count++] = "[1] Laporan";
            menus[menu_count++] = "[ESC] Keluar Akun / Keluar";
        } else {
            menus[menu_count++] = "[ESC] Keluar Akun / Keluar";
        }

        for(int i=0; i<menu_count; i++) {
            gotoXY(box_x, start_y + (i*2));
            printf("%s", menus[i]);
        }

        gotoXY(center_x - 12, start_y + (menu_count*2) + 2);
        printf("Pilih Menu : ");

        int ch = _getch();
        if (ch == 27) { session_set(-1); return; } /* ESC */

        /* Aksi berdasarkan role */
        if (me->role == ROLE_SUPERADMIN) {
            if (ch == '1') {
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
            if (ch == '1') {
                view_pembayaran_tiket(account_index);
            } else if (ch == '2') {
                view_jadwal_tiket(account_index);
            }
        } else if (me->role == ROLE_MANAGER) {
            if (ch == '1') {
                view_laporan_transaksi(account_index);
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
        draw_layout_base(w, h, "Masuk Sistem");

        int split_x = layout_split_x(w);
        int center_x = split_x + ((w - split_x) / 2);
        int form_top = (h/2) - 5;

        int box_w = 50;
        int box_x = center_x - (box_w / 2);

        gotoXY(box_x, form_top); printf("Pengguna :");
        gotoXY(box_x, form_top + 1); for(int i=0; i<box_w; i++) printf("-");

        gotoXY(box_x, form_top + 4); printf("Sandi :");
        gotoXY(box_x, form_top + 5); for(int i=0; i<box_w; i++) printf("-");

        gotoXY(split_x + 5, h - 4);
        printf("[ESC] Keluar Program  |  [TAB] Lihat/Sembunyikan Sandi  |  [ENTER] Masuk");

        gotoXY(box_x, form_top + 2);
        CONSOLE_CURSOR_INFO ci;
        GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
        ci.bVisible = TRUE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        /* Login: gunakan input plain (tanpa kotak [ ]) */
        input_text_login_plain(user, (int)sizeof(user), 0);            // email (tanpa spasi)
        if (user[0] == 27) { cls(); exit(0); }

        gotoXY(box_x, form_top + 6);
        input_password_masked_login_plain(pass, 63, box_x, form_top + 6, box_w);
        if (pass[0] == 27) { cls(); exit(0); }

        ci.bVisible = FALSE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        int acc_idx = auth_find_account_index(user, pass);
        if (acc_idx >= 0) {
            gotoXY(center_x - 10, form_top + 9); printf(">> LOGIN BERHASIL! <<");
            Sleep(800);
            session_set(acc_idx);
            dashboard_main(acc_idx);
        } else {
            Beep(500, 300);
            gotoXY(center_x - 15, form_top + 9); printf(">> PENGGUNA / SANDI SALAH <<");
            Sleep(1500);
        }
    }
}

void ui_init(void) {
    akun_init();
    penumpang_init();
    stasiun_init();
    kereta_init();
    jadwal_init();
    pembayaran_init();
}

void ui_run() {
    login_screen();
}