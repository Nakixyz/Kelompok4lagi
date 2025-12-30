#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include "ui_app.h"
#include "globals.h"

// Include Modul Logic
#include "modul_akun.h"
#include "modul_karyawan.h"
#include "modul_stasiun.h"
#include "modul_kereta.h"

/* === Forward declaration (prototype) === */
static void clear_line_fixed(int x, int y, int w);
static void show_inline_error(int x, int y, int w, const char *msg);
static int input_inline_name_unique_cancel(int x, int y, const char *label,
                                           char *buf, int sz,
                                           const char *exclude_id,
                                           int err_x, int err_y, int err_w);
static int input_inline_mdpl_3digits_plus(int x, int y, const char *label,
                                         int *out, int *is_set,
                                         int err_x, int err_y, int err_w);

static int input_inline_kota_prefix_cancel(int x, int y, const char *label,
                                           char *buf, int sz,
                                           int err_x, int err_y, int err_w);
static void redraw_inline_field(int cx, int y, int clear_w, const char *buf, int pos);
static int input_inline_prefill_edit(int x, int y, const char *label,
                                     char *buf, int sz,
                                     const char *def,
                                     int err_x, int err_y, int err_w,
                                     int clear_w);

/* --- Helper Dasar --- */
void cls() { system("cls"); }

void gotoXY(int x, int y) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);

    int maxX = csbi.dwSize.X - 1;
    int maxY = csbi.dwSize.Y - 1;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > maxX) x = maxX;
    if (y > maxY) y = maxY;

    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(h, coord);
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

/* ================== SIDEBAR ASCII (KERETA) ================== */
static const char *SIDEBAR_TRAIN_ART[] = {
"                       ...-###-.+###-... ...",
"                      .#################-.-.",
"                       ... .-+-.+###-.-..",
"                   +######+.      .",
"                    .-----     ...+####+++++#######+--.",
"                    --###-   .--##-.-+######---------..",
"                 --+--###--++++---------####-+#-.--#+",
"               .+#####.###-+############-###-+#.   #+",
"               -+-#--#--##+.############+.##-+#.  .#+",
"               -#---+#-+##+-#########++#+.##-+######+",
"                -####--###-+############-+##-+######+",
"                -+--+#######################++##++##+",
"              ....-.-+###+--+#+-++-+--#+-#.+#-+-+#--##.",
"             -#-#-#.+-##--##---#+-+-.#-+---+-+..#++.+-.",
"            .#+-++#.#-+#-++-+.-+##+-.+##-#--+.-----+++#.",
"            +#.#-+#.#-+##-++.##.-.......-+####++-.  .-#.",
"           .---+--+.+-......---+##+++------...-++####+-",
"              ..----++#++---.--+++---. ..--+#+-..--++##.",
"         ++###+--.-++###++-.    ..--++-..-+++##+--...",
"         .+###-.       .+#####-. .+#####+-",
"          .-++#####+-...---###+++ -.",
"              .---++##++--..",
"          -####+-..."
};
static const int SIDEBAR_TRAIN_ART_LINES =
    (int)(sizeof(SIDEBAR_TRAIN_ART) / sizeof(SIDEBAR_TRAIN_ART[0]));

/* print ASCII ke kolom kiri: auto-strip spasi depan + auto-crop sesuai lebar kolom */
static void draw_sidebar_train_art(int split_x, int header_h, int h) {
    int inner_left  = 2;
    int inner_right = split_x - 1;
    int inner_w = inner_right - inner_left + 1;
    if (inner_w <= 0) return;

    int start_y = header_h + 12;
    int max_y   = h - 3;
    int max_lines = max_y - start_y + 1;
    if (max_lines <= 0) return;

    // cari minimal leading spaces biar gambar "rapi"
    int min_lead = 9999;
    for (int i = 0; i < SIDEBAR_TRAIN_ART_LINES; i++) {
        const char *s = SIDEBAR_TRAIN_ART[i];
        int lead = 0;
        while (s[lead] == ' ') lead++;
        if (s[lead] != '\0' && lead < min_lead) min_lead = lead;
    }
    if (min_lead == 9999) min_lead = 0;

    int lines_to_print = SIDEBAR_TRAIN_ART_LINES;
    if (lines_to_print > max_lines) lines_to_print = max_lines;

    for (int i = 0; i < lines_to_print; i++) {
        const char *raw = SIDEBAR_TRAIN_ART[i];

        // strip min_lead
        int raw_len = (int)strlen(raw);
        const char *s = raw;
        if (raw_len > min_lead) s += min_lead;
        else s = "";

        int len = (int)strlen(s);

        gotoXY(inner_left, start_y + i);
        for (int k = 0; k < inner_w; k++) putchar(' ');

        if (len > inner_w) {
            int cut = (len - inner_w) / 2;
            s += cut;
            len = inner_w;
        }

        int pad = (inner_w - len) / 2; // posisi tengah
        gotoXY(inner_left + pad, start_y + i);
        printf("%.*s", len, s);
    }
}

/* --- GAMBAR LAYOUT DASAR --- */
void draw_layout_base(int w, int h, const char* section_title) {
    int split_x = w / 6;
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
    draw_sidebar_train_art(split_x, header_h, h);


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

        int split_x = w / 6;
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

static void show_cursor(int visible) {
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    ci.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
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

/* === Forward declaration (prototype) === */
static void uppercase_inplace(char *s);
static int is_alpha_space_only(const char *s);
static int is_valid_kode_2_3huruf(const char *kode);
static void normalize_name(const char *src, char *dst, size_t dst_sz);
static int stasiun_nama_exists(const char *nama, const char *exclude_id);
static int stasiun_kode_exists(const char *kode, const char *exclude_id);
static int input_inline_kode_2_3(int x,int y,const char *label, char *buf,int sz,
                                int allow_empty, int err_x,int err_y,int err_w);

// Lebar isi kolom
#define COL_NO     4
#define COL_KODE   10
#define COL_NAMA   24
#define COL_MDPL   10
#define COL_KOTA   16
#define COL_ALAMAT 22
#define COL_STATUS 12

static void print_center_field(const char *text, int w) {
    int len = (int)strlen(text);
    if (len > w) len = w;

    int left  = (w - len) / 2;
    int right = w - len - left;

    for (int i = 0; i < left;  i++) putchar(' ');
    printf("%.*s", len, text);
    for (int i = 0; i < right; i++) putchar(' ');
}

static void draw_table_hline(int x, int y) {
    gotoXY(x, y);
    printf("+");
    for (int i = 0; i < COL_NO + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_KODE + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_NAMA + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_MDPL + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_KOTA + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_ALAMAT + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < COL_STATUS + 2; i++) printf("-");
    printf("+");
}

static void print_table_header(int x, int y) {
    gotoXY(x, y);

    putchar('|'); putchar(' ');
    print_center_field("NO", COL_NO);
    putchar(' '); putchar('|'); putchar(' ');

    print_center_field("KODE", COL_KODE);
    putchar(' '); putchar('|'); putchar(' ');

    print_center_field("NAMA", COL_NAMA);
    putchar(' '); putchar('|'); putchar(' ');

    print_center_field("MDPL", COL_MDPL);
    putchar(' '); putchar('|'); putchar(' ');

    print_center_field("KOTA", COL_KOTA);
    putchar(' '); putchar('|'); putchar(' ');

    print_center_field("ALAMAT", COL_ALAMAT);
    putchar(' '); putchar('|'); putchar(' ');

    print_center_field("STATUS", COL_STATUS);
    putchar(' '); putchar('|');
}

static WORD g_attr_normal = 0;

static void init_console_attr_once() {
    if (g_attr_normal != 0) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    g_attr_normal = csbi.wAttributes;
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
                            const char *kode,
                            const char *nama,
                            const char *mdpl,
                            const char *kota,
                            const char *alamat,
                            const char *status,
                            int selected) {
    char no_str[16];
    if (no <= 0) strcpy(no_str, "");
    else snprintf(no_str, sizeof(no_str), "%d", no);

    gotoXY(x, y);
    set_highlight(selected);

    printf("| %-*.*s | %-*.*s | %-*.*s | %*.*s | %-*.*s | %-*.*s | %-*.*s |",
   COL_NO, COL_NO, no_str,
   COL_KODE, COL_KODE, (no<=0?"":(kode?kode:"")),
   COL_NAMA, COL_NAMA, (no<=0?"":(nama?nama:"")),
   COL_MDPL, COL_MDPL, (no<=0?"":(mdpl?mdpl:"")),
   COL_KOTA, COL_KOTA, (no<=0?"":(kota?kota:"")),
   COL_ALAMAT, COL_ALAMAT, (no<=0?"":(alamat?alamat:"")),
   COL_STATUS, COL_STATUS, (no<=0?"":(status?status:"")));


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

static int input_inline_cancel(int x, int y, const char *label, char *buf, int sz) {
    gotoXY(x, y);
    printf("%s", label);
    int cx = x + (int)strlen(label);
    gotoXY(cx, y);

    int i = 0;
    buf[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 13) { // ENTER
            buf[i] = '\0';
            return 1;
        }
        if (ch == 8) { // BACKSPACE
            if (i > 0) {
                i--;
                buf[i] = '\0';
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
            if (ch == '|') { Beep(600, 40); continue; }
            buf[i++] = (char)ch;
            buf[i] = '\0';
            putchar((char)ch);
        }
    }
}

static int input_inline_name_unique_cancel(int x, int y, const char *label,
                                          char *buf, int sz,
                                          const char *exclude_id,
                                          int err_x, int err_y, int err_w)
{
    gotoXY(x, y);
    printf("%s", label);

    int cx = x + (int)strlen(label);
    gotoXY(cx, y);

    int i = 0;
    buf[0] = '\0';

    while (1) {
        int ch = _getch();
        if (ch == 0 || ch == 224) { _getch(); continue; }

        if (ch == 13) { // enter
            buf[i] = '\0';

            if (i == 0) {
                show_inline_error(err_x, err_y, err_w, "");
                return 1;
            }

            if (stasiun_nama_exists(buf, exclude_id)) {
                Beep(700, 80);
                show_inline_error(err_x, err_y, err_w, "ERROR: Nama stasiun sudah ada. Ganti nama.");
                continue; // tetap di input nama (loop)
            }

            show_inline_error(err_x, err_y, err_w, "");
            return 1;
        }

        if (ch == 8) { //backpace
            if (i > 0) {
                i--;
                buf[i] = '\0';
                gotoXY(cx + i, y); putchar(' ');
                gotoXY(cx + i, y);
            }

            if (i > 0 && stasiun_nama_exists(buf, exclude_id)) {
                show_inline_error(err_x, err_y, err_w, "ERROR: Nama stasiun sudah ada. Ganti nama.");
            } else {
                show_inline_error(err_x, err_y, err_w, "");
            }
            continue;
        }

        if (ch == '0' && i == 0) { //cancel
            buf[0] = '\0';
            show_inline_error(err_x, err_y, err_w, "");
            return 0;
        }

        if (ch == ' ' || isalpha((unsigned char)ch)) {
            if (i < sz - 1) {
                if (ch != ' ') ch = toupper((unsigned char)ch);
                buf[i++] = (char)ch;
                buf[i] = '\0';
                putchar((char)ch);

                if (stasiun_nama_exists(buf, exclude_id)) {
                    show_inline_error(err_x, err_y, err_w, "ERROR: Nama stasiun sudah ada. Ganti nama.");
                } else {
                    show_inline_error(err_x, err_y, err_w, "");
                }
            }
        } else {
            Beep(600, 40);
        }
    }
}

static void clear_line_fixed(int x, int y, int w) {
    gotoXY(x, y);
    for (int i = 0; i < w; i++) putchar(' ');
}

static void show_inline_error(int x, int y, int w, const char *msg) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    COORD old = csbi.dwCursorPosition;

    clear_line_fixed(x, y, w);
    gotoXY(x, y);
    printf("%.*s", w, msg ? msg : "");

    SetConsoleCursorPosition(h, old);
}

// ========== PREFILL EDIT INPUT ==========
static void redraw_inline_field(int cx, int y, int clear_w, const char *buf, int pos) {
    gotoXY(cx, y);
    for (int i = 0; i < clear_w; i++) putchar(' ');

    gotoXY(cx, y);
    printf("%.*s", clear_w, buf ? buf : "");

    if (pos < 0) pos = 0;
    if (pos > clear_w) pos = clear_w;

    gotoXY(cx + pos, y);
}

static int input_inline_prefill_edit(int x, int y, const char *label,
                                     char *buf, int sz,
                                     const char *def,
                                     int err_x, int err_y, int err_w,
                                     int clear_w)
{
    gotoXY(x, y);
    printf("%s", label);

    int cx = x + (int)strlen(label);
    gotoXY(cx, y);

    char def_copy[512];
    def_copy[0] = '\0';

    if (def && def[0]) {
        strncpy(def_copy, def, sizeof(def_copy) - 1);
        def_copy[sizeof(def_copy) - 1] = '\0';
    }

    buf[0] = '\0';
    if (def_copy[0]) {
        strncpy(buf, def_copy, sz - 1);
        buf[sz - 1] = '\0';
    }
    int len = (int)strlen(buf);
    int pos = len;

    if (len > clear_w) {
        len = clear_w;
        buf[len] = '\0';
    }
    pos = len;

    redraw_inline_field(cx, y, clear_w, buf, pos);

    while (1) {
        int ch = _getch();

        if (ch == 13) {
            show_inline_error(err_x, err_y, err_w, "");
            return 1;
        }

        if (ch == 0 || ch == 224) {
            int k = _getch();

            if (k == 75) {            // LEFT
                if (pos > 0) pos--;
                gotoXY(cx + pos, y);
            } else if (k == 77) {     // RIGHT
                if (pos < len) pos++;
                gotoXY(cx + pos, y);
            } else if (k == 71) {     // HOME
                pos = 0;
                gotoXY(cx, y);
            } else if (k == 79) {     // END
                pos = len;
                gotoXY(cx + pos, y);
            } else if (k == 83) {     // DELETE
                if (pos < len) {
                    memmove(buf + pos, buf + pos + 1, (size_t)(len - pos));
                    len--;
                    buf[len] = '\0';
                    redraw_inline_field(cx, y, clear_w, buf, pos);
                }
            }
            continue;
        }

        // BACKSPACE
        if (ch == 8) {
            if (pos > 0) {
                memmove(buf + pos - 1, buf + pos, (size_t)(len - pos + 1)); // termasuk '\0'
                pos--;
                len--;
                redraw_inline_field(cx, y, clear_w, buf, pos);
            }
            continue;
        }

        if (ch == '0' && len == 0) {
            buf[0] = '\0';
            show_inline_error(err_x, err_y, err_w, "");
            return 0;
        }

        if (ch >= 32 && ch <= 126) {
            if (ch == '|') { Beep(600, 40); continue; }
            if (len >= sz - 1 || len >= clear_w) { Beep(500, 60); continue; }

            memmove(buf + pos + 1, buf + pos, (size_t)(len - pos + 1));
            buf[pos] = (char)ch;
            pos++;
            len++;
            buf[len] = '\0';

            redraw_inline_field(cx, y, clear_w, buf, pos);
        }
    }
}

static int input_inline_mdpl_3digits_plus(int x, int y, const char *label,
                                         int *out, int *is_set,
                                         int err_x, int err_y, int err_w)
{
    char buf[4];
    int i = 0;

    gotoXY(x, y);
    printf("%s", label);

    int cx = x + (int)strlen(label);
    gotoXY(cx, y);
    putchar('+');
    cx++;

    *is_set = 0;
    *out = 0;

    while (1) {
        int ch = _getch();
        if (ch == 0 || ch == 224) { _getch(); continue; }

        if (ch == 13) { // ENTER
            if (i == 0) {
                show_inline_error(err_x, err_y, err_w, "");
                *is_set = 0;
                *out = 0;
                return 1;
            }

            buf[i] = '\0';
            *out = atoi(buf);
            *is_set = 1;
            show_inline_error(err_x, err_y, err_w, "");
            return 1;
        }

        if (ch == 8) { // BACKSPACE
            if (i > 0) {
                i--;
                gotoXY(cx + i, y); putchar(' ');
                gotoXY(cx + i, y);
            }
            show_inline_error(err_x, err_y, err_w, "");
            continue;
        }

        if (ch == '0' && i == 0) { // cancel
            show_inline_error(err_x, err_y, err_w, "");
            return 0;
        }

        if (!isdigit((unsigned char)ch)) {
            Beep(600, 40);
            show_inline_error(err_x, err_y, err_w, "ERROR: MDPL hanya angka (0-9).");
            continue;
        }

        if (i >= 3) {
            Beep(500, 60);
            show_inline_error(err_x, err_y, err_w, "ERROR: MDPL maksimal 3 angka.");
            continue;
        }

        buf[i++] = (char)ch;
        putchar((char)ch);
        show_inline_error(err_x, err_y, err_w, "");
    }
}

static int input_inline_kota_prefix_cancel(int x, int y, const char *label,
                                           char *buf, int sz,
                                           int err_x, int err_y, int err_w)
{
    const char *prefix = "Kota ";
    int pLen = (int)strlen(prefix);

    gotoXY(x, y);
    printf("%s", label);

    int cx = x + (int)strlen(label);
    gotoXY(cx, y);
    printf("%s", prefix);
    cx += pLen;

    char rest[64];
    int i = 0;
    rest[0] = '\0';
    buf[0] = '\0';

    while (1) {
        int ch = _getch();

        if (ch == 0 || ch == 224) { _getch(); continue; }

        if (ch == 13) { // ENTER
            if (i == 0) {
                buf[0] = '\0';
                show_inline_error(err_x, err_y, err_w, "");
                return 1;
            }

            rest[i] = '\0';
            if (pLen + i + 1 > sz) {
                Beep(500, 60);
                show_inline_error(err_x, err_y, err_w, "ERROR: Kota terlalu panjang.");
                continue;
            }

            strcpy(buf, prefix);
            strcat(buf, rest);

            show_inline_error(err_x, err_y, err_w, "");
            return 1;
        }

        if (ch == 8) { // BACKSPACE
            if (i > 0) {
                i--;
                rest[i] = '\0';
                gotoXY(cx + i, y); putchar(' ');
                gotoXY(cx + i, y);
            }
            show_inline_error(err_x, err_y, err_w, "");
            continue;
        }

        if (ch == '0' && i == 0) { // cancel
            buf[0] = '\0';
            show_inline_error(err_x, err_y, err_w, "");
            return 0;
        }

        if (ch == ' ' || isalpha((unsigned char)ch)) {
            if (i < (int)sizeof(rest) - 1 && (pLen + i + 1) < sz) {
                rest[i++] = (char)ch;
                rest[i] = '\0';
                putchar((char)ch);
                show_inline_error(err_x, err_y, err_w, "");
            }
        } else {
            Beep(600, 40);
            show_inline_error(err_x, err_y, err_w, "ERROR: Kota hanya huruf A-Z.");
        }
    }
}

static int input_inline_kode_2_3(int x,int y,const char *label,
                                 char *buf,int sz,
                                 int allow_empty,
                                 int err_x,int err_y,int err_w)
{
    gotoXY(x,y);
    printf("%s",label);

    int cx = x + (int)strlen(label);
    gotoXY(cx,y);

    int i = 0;
    buf[0] = '\0';

    while (1) {
        int ch = _getch();

        // buang scan code arrow/function key
        if (ch == 0 || ch == 224) { _getch(); continue; }

        // ENTER
        if (ch == 13) {
            // edit mode: boleh kosong -> "tetap"
            if (i == 0 && allow_empty) {
                show_inline_error(err_x, err_y, err_w, "");
                return 1;
            }

            // wajib 2-3 huruf
            if (i < 2 || i > 3) {
                Beep(500, 60);
                show_inline_error(err_x, err_y, err_w,
                                  "ERROR: Kode harus 2 atau 3 huruf (A-Z).");
                continue;
            }

            buf[i] = '\0';
            show_inline_error(err_x, err_y, err_w, "");
            return 1;
        }

        // BACKSPACE
        if (ch == 8) {
            if (i > 0) {
                i--;
                buf[i] = '\0';
                gotoXY(cx + i, y); putchar(' ');
                gotoXY(cx + i, y);
            }
            show_inline_error(err_x, err_y, err_w, "");
            continue;
        }

        // cancel: '0' kalau masih kosong
        if (ch == '0' && i == 0) {
            buf[0] = '\0';
            show_inline_error(err_x, err_y, err_w, "");
            return 0;
        }

        // harus huruf A-Z
        if (!isalpha((unsigned char)ch)) {
            Beep(700, 80);
            show_inline_error(err_x, err_y, err_w, "ERROR: Kode hanya huruf A-Z.");
            continue;
        }

        // maksimal 3 huruf
        if (i >= 3 || i >= sz - 1) {
            Beep(500, 60);
            show_inline_error(err_x, err_y, err_w, "ERROR: Maksimal 3 huruf.");
            continue;
        }

        // masukin huruf (uppercase)
        ch = toupper((unsigned char)ch);
        buf[i++] = (char)ch;
        buf[i] = '\0';
        putchar((char)ch);

        show_inline_error(err_x, err_y, err_w, "");
    }
}

static void build_alamat_with_jl_prefix(const char *rest, char *out, size_t outsz)
{
    if (!out || outsz == 0) return;
    out[0] = '\0';

    if (!rest) return;

    while (*rest == ' ') rest++;

    // kalau user ngetik Jl. ..., buang "Jl." nya biar gak dobel
    if (_strnicmp(rest, "Jl.", 3) == 0) {
        rest += 3;
        while (*rest == ' ') rest++;
    }

    if (*rest == '\0') return;

    snprintf(out, outsz, "Jl. %s", rest);
}

static int count_stasiun_by_mode(int mode) {
    int total = 0;
    for (int i = 0; i < g_stasiunCount; i++) {
        if (mode == 0 && g_stasiun[i].active) total++;
        else if (mode == 1 && !g_stasiun[i].active) total++;
    }
    return total;
}

static int calc_table_width() {
    // sesuai draw_table_hline() kamu: "+" + (COL+2 dashes + "+") tiap kolom
    int w = 1; // plus pertama
    w += (COL_NO + 2) + 1;
    w += (COL_KODE + 2) + 1;
    w += (COL_NAMA + 2) + 1;
    w += (COL_MDPL + 2) + 1;
    w += (COL_KOTA + 2) + 1;
    w += (COL_ALAMAT + 2) + 1;
    w += (COL_STATUS + 2) + 1;
    return w; // sudah termasuk '+' terakhir
}

// isi cache data per halaman: row_idx[r] = index asli g_stasiun, row_has[r]=1 kalau ada data
static void build_page_cache(int page, int per_page, int row_idx[], int row_has[]) {
    for (int r = 0; r < per_page; r++) { row_idx[r] = -1; row_has[r] = 0; }

    int start = page * per_page;
    int end   = start + per_page;

    int shown = 0;
    int filled = 0;

    for (int i = 0; i < g_stasiunCount; i++) {   // TIDAK FILTER
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
                            int row_idx[], int row_has[])
{
    int y = ty + 3 + r;
    int start = page * per_page;

    if (row_has[r]) {
        int i = row_idx[r];

        char mdplbuf[16];
        snprintf(mdplbuf, sizeof(mdplbuf), "%+dm", g_stasiun[i].mdpl);

        const char *status = g_stasiun[i].active ? "Aktif" : "Non Aktif";

        print_table_row(tx, y,
            start + r + 1,
            g_stasiun[i].kode,
            g_stasiun[i].nama,
            mdplbuf,
            g_stasiun[i].kota,
            g_stasiun[i].alamat,
            status,
            selected
        );
    } else {
        print_table_row(tx, y, 0, "", "", "", "", "", "", selected);
    }
}

static void draw_table_full(int tx, int ty, int page, int per_page, int cursor,
                            int table_w, int row_idx[], int row_has[]) {

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

static int is_alpha_space_only(const char *s) {
    if (!s) return 0;
    int has_letter = 0;

    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        if (*p == ' ') continue;
        if (isalpha(*p)) { has_letter = 1; continue; }
        return 0;
    }
    return has_letter;
}

static int is_valid_kode_2_3huruf(const char *kode) {
    if (!kode) return 0;
    int len = (int)strlen(kode);
    if (len < 2 || len > 3) return 0;

    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)kode[i];
        if (!isalpha(c) || !isupper(c)) return 0;
    }
    return 1;
}

static void uppercase_inplace(char *s) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        s[i] = (char)toupper((unsigned char)s[i]);
    }
}

static void set_breadcrumb(int w, const char *text) {
    int split_x = w / 6;
    int header_h = 7;

    int x = split_x + 2;
    int y = header_h + 1;

    int maxw = (w - 3) - x;
    if (maxw < 0) maxw = 0;

    gotoXY(x, y);
    for (int i = 0; i < maxw; i++) putchar(' ');

    gotoXY(x, y);
    printf(" %.*s", maxw - 1, text ? text : "");
}

static void normalize_name(const char *src, char *dst, size_t dst_sz) {
    size_t j = 0;
    int in_space = 1;

    if (!src || !dst || dst_sz == 0) return;

    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_sz; i++) {
        unsigned char c = (unsigned char)src[i];

        if (isalpha(c)) {
            if (in_space == 0) {
            } else {
                if (j > 0 && j + 1 < dst_sz) dst[j++] = ' ';
                in_space = 0;
            }
            dst[j++] = (char)toupper(c);
        } else if (c == ' ') {
            in_space = 1;
        }
    }
    dst[j] = '\0';
}

static int stasiun_nama_exists(const char *nama, const char *exclude_id) {
    char key[80], cur[80];
    normalize_name(nama, key, sizeof(key));
    if (key[0] == '\0') return 0;

    for (int i = 0; i < g_stasiunCount; i++) {
        if (!g_stasiun[i].active) continue;
        if (exclude_id && strcmp(g_stasiun[i].id, exclude_id) == 0) continue;

        normalize_name(g_stasiun[i].nama, cur, sizeof(cur));
        if (strcmp(key, cur) == 0) return 1;
    }
    return 0;
}

static int stasiun_kode_exists(const char *kode, const char *exclude_id) {
    if (!kode || kode[0] == '\0') return 0;

    char key[10];
    strncpy(key, kode, sizeof(key) - 1);
    key[sizeof(key) - 1] = '\0';
    uppercase_inplace(key);

    for (int i = 0; i < g_stasiunCount; i++) {
        if (!g_stasiun[i].active) continue;
        if (exclude_id && strcmp(g_stasiun[i].id, exclude_id) == 0) continue;

        if (strcmp(g_stasiun[i].kode, key) == 0) return 1;
    }
    return 0;
}

typedef struct {
    char nama_norm[80];
    int mdpl;
    char kota[64];
    char alamat[160];
} StationAutoRow;

static StationAutoRow *g_auto = NULL;
static int g_auto_count = 0;

static void rstrip_newline(char *s) {
    if (!s) return;
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static void sanitize_field(char *s) {
    if (!s) return;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;

        if (c == '|') *s = '/';
        else if (c == '\t') *s = ' ';
        else if (c == '\r' || c == '\n') *s = ' ';
        else if (c < 32) *s = ' ';
    }
}

int station_autofill_load_csv(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    free(g_auto);
    g_auto = NULL;
    g_auto_count = 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        rstrip_newline(line);
        if (line[0] == '\0') continue;

        // format: NAMA|MDPL|KOTA|ALAMAT
        char *nama = strtok(line, "|");
        char *mdpl = strtok(NULL, "|");
        char *kota = strtok(NULL, "|");
        char *alamat = strtok(NULL, "");

        if (!nama || !mdpl || !kota || !alamat) continue;

        sanitize_field(kota);
        sanitize_field(alamat);

        StationAutoRow row;
        normalize_name(nama, row.nama_norm, sizeof(row.nama_norm));
        row.mdpl = atoi(mdpl);

        strncpy(row.kota, kota, sizeof(row.kota)-1);
        row.kota[sizeof(row.kota)-1] = '\0';

        strncpy(row.alamat, alamat, sizeof(row.alamat)-1);
        row.alamat[sizeof(row.alamat)-1] = '\0';

        StationAutoRow *tmp = realloc(g_auto, (g_auto_count + 1) * sizeof(StationAutoRow));
        if (!tmp) break;
        g_auto = tmp;
        g_auto[g_auto_count++] = row;
    }

    fclose(f);
    return g_auto_count;
}

static int station_autofill_lookup(const char *nama_input,
                                   int *out_mdpl,
                                   char *out_kota, int kota_sz,
                                   char *out_alamat, int alamat_sz)
{
    if (!nama_input || !out_mdpl || !out_kota || !out_alamat) return 0;

    char key[80];
    normalize_name(nama_input, key, sizeof(key));
    if (key[0] == '\0') return 0;

    for (int i = 0; i < g_auto_count; i++) {
        if (strcmp(key, g_auto[i].nama_norm) == 0) {
            *out_mdpl = g_auto[i].mdpl;

            strncpy(out_kota, g_auto[i].kota, kota_sz - 1);
            out_kota[kota_sz - 1] = '\0';

            strncpy(out_alamat, g_auto[i].alamat, alamat_sz - 1);
            out_alamat[alamat_sz - 1] = '\0';

            return 1;
        }
    }
    return 0;
}

static int is_blank_or_dots(const char *s) {
    if (!s) return 1;

    // skip spasi
    while (*s == ' ' || *s == '\t') s++;

    // kosong
    if (*s == '\0') return 1;

    if (strcmp(s, ".") == 0) return 1;
    if (strcmp(s, "..") == 0) return 1;

    return 0;
}
static int starts_with_icase(const char *s, const char *prefix) {
    while (*prefix && *s) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
        s++; prefix++;
    }
    return (*prefix == '\0');
}

static void ensure_kota_prefix(const char *in, char *out, size_t outsz) {
    if (!out || outsz == 0) return;
    out[0] = '\0';
    if (!in) return;

    while (*in == ' ' || *in == '\t') in++;

    if (*in == '\0') return;

    if (starts_with_icase(in, "Kota ")) {
        strncpy(out, in, outsz - 1);
        out[outsz - 1] = '\0';
        return;
    }

    if (starts_with_icase(in, "Kab.") ||
        starts_with_icase(in, "Kab ") ||
        starts_with_icase(in, "Kabupaten ")) {
        strncpy(out, in, outsz - 1);
        out[outsz - 1] = '\0';
        return;
        }
    snprintf(out, outsz, "Kota %s", in);
}

static void draw_alamat_line(int ax, int y, int aw, const char *alamat_final) {
    clear_line_fixed(ax, y, aw);
    gotoXY(ax, y);
    printf("Alamat : %.*s", aw - (int)strlen("Alamat : "), alamat_final ? alamat_final : "");
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
    int need_panel_redraw = 1;  // redraw panel bawah
    int need_action_clear = 1;  // bersihkan area aksi

    int last_w = -1, last_h = -1;

    while (1) {
        int w = get_screen_width();  if (w <= 0) w = 120;
        int h = get_screen_height(); if (h <= 0) h = 30;

        if (w != last_w || h != last_h) {
            last_w = w; last_h = h;
            need_full_layout = 1;
        }

        int split_x = w / 6;
        int content_x1 = split_x + 2;
        int content_x2 = w - 3;
        int content_w  = content_x2 - content_x1 + 1;

        // hitung total & pages
        int total = g_stasiunCount;
        int total_pages = (total + per_page - 1) / per_page;
        if (total_pages < 1) total_pages = 1;

        if (page >= total_pages) page = total_pages - 1;
        if (page < 0) page = 0;
        if (cursor < 0) cursor = 0;
        if (cursor > per_page - 1) cursor = per_page - 1;

        // posisi tabel
        table_w = calc_table_width();
        tx = content_x1 + (content_w - table_w) / 2;
        if (tx < content_x1) tx = content_x1;
        ty = 9;

        if (need_full_layout) {
            cls();
            draw_layout_base(w, h, "Kelola Stasiun");
            set_breadcrumb(w, "Kelola Stasiun >");
            need_full_layout = 0;
            need_table_redraw = 1;
            need_panel_redraw = 1;
            need_action_clear = 1;
        }

        if (need_table_redraw) {
            build_page_cache(page, per_page, row_idx, row_has);
            draw_table_full(tx, ty, page, per_page, cursor, table_w, row_idx, row_has);
            need_table_redraw = 0;
            need_panel_redraw = 1;
            need_action_clear = 1;
        }

        // ===== PANEL BAWAH =====
        int info_y  = ty + 3 + per_page + 1;
        int panel_x = tx;
        int panel_w = table_w;

        char pageText[64];
        snprintf(pageText, sizeof(pageText), "Halaman: %d/%d", page + 1, total_pages);

        const char *help1 = "[UP/DOWN] Pilih   [LEFT/RIGHT] Halaman";
        const char *help2 = "[ENTER] Detail  [E] Edit  [X] Hapus  [A] Tambah  [0] Kembali";
        const char *help3 = "Aksi: pilih data dulu.";

        int panel_body_lines = 3;

        if (need_panel_redraw) {
            clear_rect(panel_x, info_y, panel_w, panel_body_lines + 2);

            draw_hline_box(panel_x, info_y, panel_w);
            draw_text_in_box(panel_x, info_y + 1, panel_w, help1);
            draw_text_in_box(panel_x, info_y + 2, panel_w, help2);
            draw_text_in_box(panel_x, info_y + 3, panel_w, help3);
            draw_hline_box(panel_x, info_y + 1 + panel_body_lines, panel_w);

            need_panel_redraw = 0;
        }

        // --- HALAMAN: kanan bawah, di luar panel ---
        {
            int y_page = h - 3;
            int right_limit = w - 3;
            int left_limit  = split_x + 2;

            int clear_w = 30;
            int x_clear = right_limit - clear_w + 1;
            if (x_clear < left_limit) x_clear = left_limit;

            gotoXY(x_clear, y_page);
            for (int i = x_clear; i <= right_limit; i++) putchar(' ');

            int x_page = right_limit - (int)strlen(pageText) + 1;
            if (x_page < left_limit) x_page = left_limit;

            gotoXY(x_page, y_page);
            printf("%s", pageText);
        }

        int ax = tx;
        int ay = info_y + (panel_body_lines + 2) + 1;
        int aw = table_w;
        int ah = 11;

        if (need_action_clear) {
            clear_rect(ax, ay, aw, ah);
            need_action_clear = 0;
        }

        int ch = _getch();

        // ===== ARROW KEYS =====
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

            // kalau pindah page
            if (page != old_page) {
                need_table_redraw = 1;
                continue;
            }

            // kalau cuma pindah cursor
            if (cursor != old_cursor) {
                repaint_one_row(tx, ty, page, per_page, old_cursor, 0, row_idx, row_has);
                repaint_one_row(tx, ty, page, per_page, cursor,     1, row_idx, row_has);
                // panel & action area JANGAN di-clear -> hilang blink
            }
            continue;
        }

        if (ch == '0') return;

    // ===== TAMBAH =====
if (ch == 'a' || ch == 'A') {

    while (1) {
        clear_rect(ax, ay, aw, ah);
        set_breadcrumb(w, "Kelola Stasiun > Tambah");

        gotoXY(ax, ay);
        printf("Tambah Stasiun (Tekan 0 untuk kembali saat input kosong)");

        // ===== deklarasi variabel =====
        char id[10], kode[10], nama[50], kota[30];
        char alamat_final[160];

        int mdpl = 0;
        int mdpl_set = 0;

        char kota_auto[64]   = "";
        char alamat_auto[160]= "";

        generate_next_stasiun_id(id);

        int err_x = ax;
        int err_y = ay + 9;
        int err_w = aw;

        show_cursor(1);

        // ===== KODE =====
ulang_kode_tambah:
        clear_line_fixed(ax, ay + 3, aw);
        if (!input_inline_kode_2_3(ax, ay + 3, "Kode   : ", kode, sizeof(kode), 0, err_x, err_y, err_w)) {
            show_cursor(0);
            need_action_clear = 1;
            break;
        }
        if (stasiun_kode_exists(kode, NULL)) {
            show_inline_error(err_x, err_y, err_w, "ERROR : Kode stasiun sudah ada.");
            Beep(500, 80);
            _getch();
            show_inline_error(err_x, err_y, err_w, "");
            goto ulang_kode_tambah;
        }

        // ===== NAMA =====
ulang_nama_tambah:
        if (!input_inline_name_unique_cancel(ax, ay + 4, "Nama   : ", nama, sizeof(nama),
                                             NULL, err_x, err_y, err_w)) {
            show_cursor(0);
            need_action_clear = 1;
            break;
        }
        if (nama[0] == '\0') {
            show_inline_error(err_x, err_y, err_w, "ERROR : Nama wajib diisi.");
            Beep(500, 80);
            _getch();
            show_inline_error(err_x, err_y, err_w, "");
            goto ulang_nama_tambah;
        }

        int found;
        int auto_mdpl_ok;
        int auto_kota_ok;
        int auto_alamat_ok;

        found = station_autofill_lookup(
            nama, &mdpl,
            kota_auto, sizeof(kota_auto),
            alamat_auto, sizeof(alamat_auto)
        );

        auto_mdpl_ok   = (found && mdpl > 0);
        auto_kota_ok   = (found && !is_blank_or_dots(kota_auto));
        auto_alamat_ok = (found && !is_blank_or_dots(alamat_auto));

        // reset hasil akhir
        mdpl_set = 0;
        kota[0] = '\0';
        alamat_final[0] = '\0';

        clear_line_fixed(ax, ay + 5, aw);

        // tampilkan hasil autofill
        if (found) {
            if (auto_mdpl_ok) {
                mdpl_set = 1;
                gotoXY(ax, ay + 5);
                printf("MDPL   : +%dm", mdpl);
            } else {
                gotoXY(ax, ay + 5);
                printf("MDPL   : (kosong, isi manual)");
            }
        }

        if (kota[0] == '\0' && auto_kota_ok) {
            ensure_kota_prefix(kota_auto, kota, sizeof(kota));
        }

        if (kota[0] == '\0') {
            strcpy(kota, "Kota Indonesia");
        }
        clear_line_fixed(ax, ay + 6, aw);
        gotoXY(ax, ay + 6);
        printf("Kota   : %s", kota);

        if (auto_alamat_ok) {
            strncpy(alamat_final, alamat_auto, sizeof(alamat_final)-1);
            alamat_final[sizeof(alamat_final)-1] = '\0';
        }

        if (alamat_final[0] == '\0') {
            snprintf(alamat_final, sizeof(alamat_final),
                     "Stasiun %s, %s", nama, (kota[0] ? kota : "Indonesia"));
        }
        draw_alamat_line(ax, ay + 7, aw, alamat_final);
        if (!mdpl_set) {
            clear_line_fixed(ax, ay + 5, aw);
            if (!input_inline_mdpl_3digits_plus(ax, ay + 5, "MDPL   : ", &mdpl, &mdpl_set, err_x, err_y, err_w)) {
                show_cursor(0); need_action_clear = 1; break;
            }
            if (!mdpl_set) {
                show_inline_error(err_x, err_y, err_w, "ERROR: MDPL wajib diisi 1-3 angka.");
                _getch();
                continue;
            }
        }

        show_cursor(1);

        char alamat_def[160];
        strncpy(alamat_def, alamat_final, sizeof(alamat_def)-1);
        alamat_def[sizeof(alamat_def)-1] = '\0';

        int right_limit = w - 3;
        int cx_field    = ax + (int)strlen("Alamat : ");
        int clear_w     = right_limit - cx_field + 1;
        if (clear_w < 0) clear_w = 0;

        clear_line_fixed(ax, ay + 7, aw);
        if (!input_inline_prefill_edit(
                ax, ay + 7,
                "Alamat : ",
                alamat_final, sizeof(alamat_final),
                alamat_def,
                err_x, err_y, err_w,
                clear_w
            )) {
            show_cursor(0);
            need_action_clear = 1;
            break;
            }

        show_cursor(0);

        // ===== SIMPAN DATA =====
        stasiun_create(id, kode, nama, mdpl, kota, alamat_final);

        show_inline_error(err_x, err_y, err_w, "OK: Berhasil tambah data. Tekan apa saja...");
        _getch();
        show_inline_error(err_x, err_y, err_w, "");   // atau clear_line_fixed(err_x, err_y, err_w);

        need_table_redraw = 1;
        need_panel_redraw = 1;
        need_action_clear = 1;
        set_breadcrumb(w, "Kelola Stasiun >");
        break;
    }

    continue;
}

        if (!row_has[cursor]) continue;

        int idx = row_idx[cursor];
        if (idx < 0 || idx >= g_stasiunCount) continue;

        if (ch == 'r' || ch == 'R') {
            clear_rect(ax, ay, aw, ah);
            set_breadcrumb(w, "Kelola Stasiun > Riwayat");

            if (g_stasiun[idx].active) {
                gotoXY(ax, ay); printf("Data sudah AKTIF. Tekan apa saja...");
                _getch();
                need_action_clear = 1;
                set_breadcrumb(w, "Kelola Stasiun >");
                continue;
            }

            gotoXY(ax, ay);     printf("Aktifkan lagi stasiun ini? (Y/N)");
            gotoXY(ax, ay + 2); printf("Kode   : %s", g_stasiun[idx].kode);
            gotoXY(ax, ay + 3); printf("Nama   : %s", g_stasiun[idx].nama);

            int c = _getch();
            if (c == 'y' || c == 'Y') {
                stasiun_restore_by_id(g_stasiun[idx].id);
                gotoXY(ax, ay + 5); printf("OK: Status jadi AKTIF. Tekan apa saja...");
                _getch();
                need_table_redraw = 1;
                need_panel_redraw = 1;
            }

            need_action_clear = 1;
            set_breadcrumb(w, "Kelola Stasiun >");
            continue;
        }

        // ===== DETAIL =====
            if (ch == 13) {
                clear_rect(ax, ay, aw, ah);
                set_breadcrumb(w, "Kelola Stasiun > Detail");

                char mdplbuf[16];
                snprintf(mdplbuf, sizeof(mdplbuf), "%+dm", g_stasiun[idx].mdpl);

                gotoXY(ax, ay);     printf("Detail Stasiun (Tekan apa saja untuk kembali)");
                gotoXY(ax, ay + 3); printf("Kode   : %s", g_stasiun[idx].kode);
                gotoXY(ax, ay + 4); printf("Nama   : %s", g_stasiun[idx].nama);
                gotoXY(ax, ay + 5); printf("MDPL   : %s", mdplbuf);
                gotoXY(ax, ay + 6); printf("Kota   : %s", g_stasiun[idx].kota);
                gotoXY(ax, ay + 7); printf("Alamat : %s", g_stasiun[idx].alamat);

                _getch();
                set_breadcrumb(w, "Kelola Stasiun >");
                need_action_clear = 1;
                continue;
            }

        // ===== HAPUS =====
        if (ch == 'x' || ch == 'X') {
            clear_rect(ax, ay, aw, ah);
            set_breadcrumb(w, "Kelola Stasiun > Hapus");

            // (opsional) kalau sudah non aktif, jangan diproses lagi
            if (!g_stasiun[idx].active) {
                gotoXY(ax, ay); printf("Data sudah NON AKTIF. Tekan apa saja...");
                _getch();
                need_action_clear = 1;
                set_breadcrumb(w, "Kelola Stasiun >");
                continue;
            }

            gotoXY(ax, ay);     printf("Ubah status jadi NON AKTIF? (Y/N)");
            gotoXY(ax, ay + 2); printf("Kode   : %s", g_stasiun[idx].kode);
            gotoXY(ax, ay + 3); printf("Nama   : %s", g_stasiun[idx].nama);
            gotoXY(ax, ay + 4); printf("Kota   : %s", g_stasiun[idx].kota);
            gotoXY(ax, ay + 5); printf("Alamat : %s", g_stasiun[idx].alamat);

            int c2 = _getch();
            if (c2 == 'y' || c2 == 'Y') {

                gotoXY(ax, ay + 7); printf("Yakin? Data tidak hilang, hanya NON AKTIF. (Y/N)");
                int c3 = _getch();

                if (c3 == 'y' || c3 == 'Y') {
                    stasiun_delete_by_id(g_stasiun[idx].id);

                    gotoXY(ax, ay + 9); printf("OK: Status jadi NON AKTIF. Tekan apa saja...");
                    _getch();

                    need_table_redraw = 1;
                    need_panel_redraw = 1;
                }
            }

            need_action_clear = 1;
            set_breadcrumb(w, "Kelola Stasiun >");
            continue;
        }

        // ===== EDIT =====
if (ch == 'e' || ch == 'E') {

    // kalau non-aktif: jangan bisa edit
    if (!g_stasiun[idx].active) {
        clear_rect(ax, ay, aw, ah);
        set_breadcrumb(w, "Kelola Stasiun > Edit");
        gotoXY(ax, ay + 2);
        printf("Tidak bisa EDIT, data NON AKTIF. Tekan R untuk aktifkan.");

        int key = _getch();
        if (key == 'r' || key == 'R') {
            stasiun_restore_by_id(g_stasiun[idx].id);
            need_table_redraw = 1;
            need_panel_redraw = 1;
        }

        need_action_clear = 1;
        set_breadcrumb(w, "Kelola Stasiun >");
        continue;
    }

    while (1) {
        clear_rect(ax, ay, aw, ah);
        set_breadcrumb(w, "Kelola Stasiun > Edit");

        gotoXY(ax, ay);
        printf("Edit Stasiun (kosongkan untuk tetap) | (Tekan 0 untuk kembali saat input kosong)");

        char kode[10]   = "";
        char nama[50]   = "";
        char kota[30]   = "";
        char alamat[100]= "";

        int err_x = ax;
        int err_y = ay + 9;
        int err_w = aw;

        show_cursor(1);

        ulang_kode_edit:
 clear_line_fixed(ax, ay + 3, aw);

        if (!input_inline_kode_2_3(ax, ay + 3, "Kode Baru   : ", kode, sizeof(kode), 1, err_x, err_y, err_w)) {
            show_cursor(0);
            need_action_clear = 1;
            set_breadcrumb(w, "Kelola Stasiun >");
            break; // batal edit
                                    }

        // kalau user isi kode baru, wajib unik
        if (kode[0] != '\0' && stasiun_kode_exists(kode, g_stasiun[idx].id)) {
            show_inline_error(err_x, err_y, err_w, "ERROR : Kode stasiun sudah ada.");
            Beep(500, 80);
            _getch();
            show_inline_error(err_x, err_y, err_w, "");
            goto ulang_kode_edit; // ulang kode baru saja
        }

        if (!input_inline_name_unique_cancel(ax, ay + 4, "Nama Baru   : ",
                                     nama, sizeof(nama),
                                     g_stasiun[idx].id,  // exclude dirinya sendiri
                                     err_x, err_y, err_w)) {
            show_cursor(0);
            need_action_clear = 1;
            set_breadcrumb(w, "Kelola Stasiun >");
            break;
        }

int mdpl_set = 0;
int mdpl_from_csv = 0;
int kota_from_csv = 0;
int alamat_from_csv = 0;

int mdpl = g_stasiun[idx].mdpl;

{
    const char *key_nama = (nama[0] != '\0') ? nama : g_stasiun[idx].nama;

    int mdpl_auto = 0;
    char kota_auto[64] = "";
    char alamat_auto[160] = "";

    if (station_autofill_lookup(key_nama, &mdpl_auto,
                                kota_auto, sizeof(kota_auto),
                                alamat_auto, sizeof(alamat_auto))) {

        if (g_stasiun[idx].mdpl == 0 && mdpl_auto > 0) {
            mdpl = mdpl_auto;
            mdpl_from_csv = 1;

            clear_line_fixed(ax, ay + 5, aw);
            gotoXY(ax, ay + 5);
            printf("MDPL Baru   : +%dm ", mdpl);
        }

        if (g_stasiun[idx].kota[0] == '\0' && !is_blank_or_dots(kota_auto)) {
            ensure_kota_prefix(kota_auto, kota, sizeof(kota));
            kota_from_csv = 1;

            clear_line_fixed(ax, ay + 6, aw);
            gotoXY(ax, ay + 6);
            printf("Kota Baru   : %s ", kota);
        }

        if (g_stasiun[idx].alamat[0] == '\0' && !is_blank_or_dots(alamat_auto)) {
            strncpy(alamat, alamat_auto, sizeof(alamat)-1);
            alamat[sizeof(alamat)-1] = '\0';
            alamat_from_csv = 1;

            clear_line_fixed(ax, ay + 7, aw);
            gotoXY(ax, ay + 7);
            printf("Alamat Baru : %s ", alamat);
        }
    }
}

if (!mdpl_from_csv) {
    int mdpl_new = 0;
    if (!input_inline_mdpl_3digits_plus(ax, ay + 5, "MDPL Baru   : ",
                                       &mdpl_new, &mdpl_set,
                                       err_x, err_y, err_w)) {
        show_cursor(0);
        need_action_clear = 1;
        set_breadcrumb(w, "Kelola Stasiun >");
        break;
    }
    if (mdpl_set) mdpl = mdpl_new;
}

if (!kota_from_csv) {
    if (!input_inline_kota_prefix_cancel(ax, ay + 6, "Kota Baru   : ",
                                         kota, sizeof(kota),
                                         err_x, err_y, err_w)) {
        show_cursor(0);
        need_action_clear = 1;
        set_breadcrumb(w, "Kelola Stasiun >");
        break;
    }
}

if (!alamat_from_csv) {
    char alamat_rest[120];
    if (!input_inline_cancel(ax, ay + 7, "Alamat Baru : Jl. ", alamat_rest, sizeof(alamat_rest))) {
        show_cursor(0);
        need_action_clear = 1;
        set_breadcrumb(w, "Kelola Stasiun >");
        break;
    }

    if (alamat_rest[0] != '\0') build_alamat_with_jl_prefix(alamat_rest, alamat, sizeof(alamat));
    else alamat[0] = '\0';
}

        if (kode[0] == '\0')   strncpy(kode,   g_stasiun[idx].kode,   sizeof(kode)-1);
        if (nama[0] == '\0')   strncpy(nama,   g_stasiun[idx].nama,   sizeof(nama)-1);
        if (kota[0] == '\0')   strncpy(kota,   g_stasiun[idx].kota,   sizeof(kota)-1);
        if (alamat[0] == '\0') strncpy(alamat, g_stasiun[idx].alamat, sizeof(alamat)-1);

        // normalisasi
        uppercase_inplace(kode);
        if (!is_valid_kode_2_3huruf(kode)) {
            show_inline_error(err_x, err_y, err_w, "ERROR: Kode harus 2 atau 3 huruf A-Z.");
            _getch();
            continue;
        }
        if (!is_alpha_space_only(nama)) {
            show_inline_error(err_x, err_y, err_w, "ERROR: Nama hanya huruf A-Z.");
            _getch();
            continue;
        }
        if (!is_alpha_space_only(kota)) {
            show_inline_error(err_x, err_y, err_w, "ERROR: Kota hanya huruf A-Z.");
            _getch();
            continue;
        }
        if (stasiun_nama_exists(nama, g_stasiun[idx].id)) {
            show_inline_error(err_x, err_y, err_w, "ERROR: Nama stasiun sudah ada.");
            _getch();
            continue;
        }

        stasiun_update(g_stasiun[idx].id, kode, nama, mdpl, kota, alamat);

        show_inline_error(err_x, err_y, err_w, "OK: Berhasil update. Tekan apa saja...");
        _getch();

        need_table_redraw = 1;
        need_panel_redraw = 1;
        need_action_clear = 1;
        set_breadcrumb(w, "Kelola Stasiun >");
        break; // keluar loop edit
    }

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

        int split_x = w / 6;
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

        int split_x = w / 6;
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
    station_autofill_load_csv("stasiun.csv");
}

void ui_run() {
    login_screen();
}
