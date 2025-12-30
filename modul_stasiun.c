#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "modul_stasiun.h"
#include "globals.h"

#define FILE_STASIUN "stasiuns_v2.dat"

typedef struct {
    const char *nama;
    const char *kode;   // singkatan
    int mdpl;
    const char *kota;
    const char *alamat; // boleh NULL kalau mau auto-generate
} StasiunPreset;

/* ==========================
   PRESET DATA (HARUS DI ATAS)
   ========================== */
static const StasiunPreset PRESET_STASIUN[] = {
    {"JAKARTA",         "JKT",  8,   "Kota Jakarta",         "Jl. Stasiun Jakarta No. 1, Kota Jakarta"},
    {"SURABAYA",        "SBY",  5,   "Kota Surabaya",        "Jl. Stasiun Surabaya No. 1, Kota Surabaya"},
    {"BANDUNG",         "BDG",  709, "Kota Bandung",         "Jl. Stasiun Bandung No. 1, Kota Bandung"},
    {"MEDAN",           "MDN",  2,   "Kota Medan",           "Jl. Stasiun Medan No. 1, Kota Medan"},
    {"BEKASI",          "BKS",  19,  "Kota Bekasi",          "Jl. Stasiun Bekasi No. 1, Kota Bekasi"},
    {"DEPOK",           "DPK",  50,  "Kota Depok",           "Jl. Stasiun Depok No. 1, Kota Depok"},
    {"TANGERANG",       "TGR",  12,  "Kota Tangerang",       "Jl. Stasiun Tangerang No. 1, Kota Tangerang"},
    {"SEMARANG",        "SMG",  4,   "Kota Semarang",        "Jl. Stasiun Semarang No. 1, Kota Semarang"},
    {"PALEMBANG",       "PLB",  8,   "Kota Palembang",       "Jl. Stasiun Palembang No. 1, Kota Palembang"},
    {"MAKASSAR",        "MKS",  6,   "Kota Makassar",        "Jl. Stasiun Makassar No. 1, Kota Makassar"},

    {"BOGOR",           "BGR",  265, "Kota Bogor",           "Jl. Stasiun Bogor No. 1, Kota Bogor"},
    {"BATAM",           "BTM",  7,   "Kota Batam",           "Jl. Stasiun Batam No. 1, Kota Batam"},
    {"PEKANBARU",       "PKU",  12,  "Kota Pekanbaru",       "Jl. Stasiun Pekanbaru No. 1, Kota Pekanbaru"},
    {"BANDAR LAMPUNG",  "BDL",  10,  "Kota Bandar Lampung",  "Jl. Stasiun Bandar Lampung No. 1, Kota Bandar Lampung"},
    {"PADANG",          "PDG",  5,   "Kota Padang",          "Jl. Stasiun Padang No. 1, Kota Padang"},
    {"MALANG",          "MLG",  445, "Kota Malang",          "Jl. Stasiun Malang No. 1, Kota Malang"},
    {"SAMARINDA",       "SMD",  8,   "Kota Samarinda",       "Jl. Stasiun Samarinda No. 1, Kota Samarinda"},
    {"DENPASAR",        "DPS",  12,  "Kota Denpasar",        "Jl. Stasiun Denpasar No. 1, Kota Denpasar"},
    {"TASIKMALAYA",     "TSM",  351, "Kota Tasikmalaya",     "Jl. Stasiun Tasikmalaya No. 1, Kota Tasikmalaya"},
    {"PONTIANAK",       "PTK",  1,   "Kota Pontianak",       "Jl. Stasiun Pontianak No. 1, Kota Pontianak"},

    {"BANJARMASIN",     "BJM",  3,   "Kota Banjarmasin",     "Jl. Stasiun Banjarmasin No. 1, Kota Banjarmasin"},
    {"BALIKPAPAN",      "BPP",  6,   "Kota Balikpapan",      "Jl. Stasiun Balikpapan No. 1, Kota Balikpapan"},
    {"JAMBI",           "JMB",  11,  "Kota Jambi",           "Jl. Stasiun Jambi No. 1, Kota Jambi"},
    {"MANADO",          "MND",  7,   "Kota Manado",          "Jl. Stasiun Manado No. 1, Kota Manado"},
    {"MATARAM",         "MTR",  10,  "Kota Mataram",         "Jl. Stasiun Mataram No. 1, Kota Mataram"},
    {"KUPANG",          "KPG",  55,  "Kota Kupang",          "Jl. Stasiun Kupang No. 1, Kota Kupang"},
    {"AMBON",           "AMB",  9,   "Kota Ambon",           "Jl. Stasiun Ambon No. 1, Kota Ambon"},
    {"JAYAPURA",        "JYP",  3,   "Kota Jayapura",        "Jl. Stasiun Jayapura No. 1, Kota Jayapura"},
    {"CIREBON",         "CRE",  5,   "Kota Cirebon",         "Jl. Stasiun Cirebon No. 1, Kota Cirebon"},
    {"SUKABUMI",        "SKB",  584, "Kota Sukabumi",        "Jl. Stasiun Sukabumi No. 1, Kota Sukabumi"},

    {"KEDIRI",          "KDR",  67,  "Kota Kediri",          "Jl. Stasiun Kediri No. 1, Kota Kediri"},
    {"PROBOLINGGO",     "PBL",  14,  "Kota Probolinggo",     "Jl. Stasiun Probolinggo No. 1, Kota Probolinggo"},
    {"TEGAL",           "TGL",  3,   "Kota Tegal",           "Jl. Stasiun Tegal No. 1, Kota Tegal"},
    {"SERANG",          "SRG",  14,  "Kota Serang",          "Jl. Stasiun Serang No. 1, Kota Serang"},
    {"BONTANG",         "BNT",  5,   "Kota Bontang",         "Jl. Stasiun Bontang No. 1, Kota Bontang"},
    {"BINJAI",          "BNJ",  28,  "Kota Binjai",          "Jl. Stasiun Binjai No. 1, Kota Binjai"},
    {"PEMATANGSIANTAR", "PMS",  400, "Kota Pematangsiantar", "Jl. Stasiun Pematangsiantar No. 1, Kota Pematangsiantar"},
    {"TANJUNGPINANG",   "TPI",  15,  "Kota Tanjungpinang",   "Jl. Stasiun Tanjungpinang No. 1, Kota Tanjungpinang"},
    {"PANGKALPINANG",   "PKP",  13,  "Kota Pangkalpinang",   "Jl. Stasiun Pangkalpinang No. 1, Kota Pangkalpinang"},
    {"GORONTALO",       "GTO",  23,  "Kota Gorontalo",       "Jl. Stasiun Gorontalo No. 1, Kota Gorontalo"},

    {"PALU",            "PLU",  6,   "Kota Palu",            "Jl. Stasiun Palu No. 1, Kota Palu"},
    {"KENDARI",         "KDI",  18,  "Kota Kendari",         "Jl. Stasiun Kendari No. 1, Kota Kendari"},
    {"TARAKAN",         "TRK",  6,   "Kota Tarakan",         "Jl. Stasiun Tarakan No. 1, Kota Tarakan"},
    {"LHOKSEUMAWE",     "LSM",  3,   "Kota Lhokseumawe",     "Jl. Stasiun Lhokseumawe No. 1, Kota Lhokseumawe"},
    {"BANDA ACEH",      "BAC",  10,  "Kota Banda Aceh",      "Jl. Stasiun Banda Aceh No. 1, Kota Banda Aceh"},
    {"SINGKAWANG",      "SKW",  9,   "Kota Singkawang",      "Jl. Stasiun Singkawang No. 1, Kota Singkawang"},
    {"PRABUMULIH",      "PRB",  50,  "Kota Prabumulih",      "Jl. Stasiun Prabumulih No. 1, Kota Prabumulih"},
    {"LUBUKLINGGAU",    "LLG",  129, "Kota Lubuklinggau",    "Jl. Stasiun Lubuklinggau No. 1, Kota Lubuklinggau"},
    {"MAGELANG",        "MGL",  380, "Kota Magelang",        "Jl. Stasiun Magelang No. 1, Kota Magelang"},
    {"PURWOKERTO",      "PWT",  75,  "Kota Purwokerto",      "Jl. Stasiun Purwokerto No. 1, Kota Purwokerto"},
};

/* hitung jumlah preset: aman & simple */
static int preset_count(void) {
    return (int)(sizeof(PRESET_STASIUN) / sizeof(PRESET_STASIUN[0]));
}

static void to_upper_copy(const char *src, char *dst, size_t dstsz) {
    size_t i = 0;
    for (; src && src[i] && i + 1 < dstsz; i++) {
        dst[i] = (char)toupper((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

static const StasiunPreset* find_preset_by_nama(const char *nama) {
    if (!nama || !*nama) return NULL;

    char key[64];
    to_upper_copy(nama, key, sizeof(key));

    for (int i = 0; i < preset_count(); i++) {
        if (strcmp(key, PRESET_STASIUN[i].nama) == 0) return &PRESET_STASIUN[i];
    }
    return NULL;
}

static void stasiun_save(void) {
    /* kalau print ini ganggu UI, boleh hapus */
    /* printf("Saving to: %s\n", FILE_STASIUN); */

    FILE *f = fopen(FILE_STASIUN, "wb");
    if (f) {
        fwrite(g_stasiun, sizeof(Stasiun), g_stasiunCount, f);
        fclose(f);
    }
}

void stasiun_init(void) {
    FILE *f = fopen(FILE_STASIUN, "rb");
    if (f) {
        g_stasiunCount = (int)fread(g_stasiun, sizeof(Stasiun), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_stasiunCount = 0;
    }

    /* Seed 50 Data Dummy */
    if (g_stasiunCount == 0) {
        int pc = preset_count();

        for (int i = 1; i <= 50 && g_stasiunCount < MAX_RECORDS; i++) {
            const StasiunPreset *p = &PRESET_STASIUN[(i - 1) % pc];

            Stasiun s;
            memset(&s, 0, sizeof(s));

            snprintf(s.id, sizeof(s.id), "STS%03d", i);

            strncpy(s.nama, p->nama, sizeof(s.nama) - 1);
            strncpy(s.kode, p->kode, sizeof(s.kode) - 1);
            s.mdpl = p->mdpl;
            strncpy(s.kota, p->kota, sizeof(s.kota) - 1);

            if (p->alamat && p->alamat[0]) {
                strncpy(s.alamat, p->alamat, sizeof(s.alamat) - 1);
            } else {
                snprintf(s.alamat, sizeof(s.alamat), "Jl. Stasiun %s, %s", s.nama, s.kota);
            }

            s.active = 1;
            g_stasiun[g_stasiunCount++] = s;
        }
        stasiun_save();
    }
}

static void sanitize_field(char *s, size_t cap) {
    if (!s || cap == 0) return;

    // hapus \r \n
    for (size_t i = 0; s[i] && i < cap; i++) {
        if (s[i] == '\r' || s[i] == '\n') {
            s[i] = '\0';
            break;
        }
    }

    // trim spasi depan
    size_t start = 0;
    while (start < cap && (s[start] == ' ' || s[start] == '\t')) start++;

    if (start > 0) {
        size_t len = strnlen(s, cap);
        if (start < len) memmove(s, s + start, len - start + 1);
        else s[0] = '\0';
    }

    // trim spasi belakang
    size_t n = strnlen(s, cap);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t')) {
        s[--n] = '\0';
    }

    // rapihin spasi ganda jadi 1
    char out[512];
    size_t j = 0;
    int in_space = 0;

    for (size_t i = 0; s[i] && j + 1 < sizeof(out); i++) {
        char c = s[i];
        if (c == ' ' || c == '\t') {
            if (!in_space) out[j++] = ' ';
            in_space = 1;
        } else {
            out[j++] = c;
            in_space = 0;
        }
    }
    out[j] = '\0';

    // copy balik sesuai kapasitas s
    strncpy(s, out, cap - 1);
    s[cap - 1] = '\0';
}

void stasiun_create(const char* id, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat) {
    if (g_stasiunCount >= MAX_RECORDS) return;

    Stasiun s;
    memset(&s, 0, sizeof(Stasiun));

    strncpy(s.id, id, sizeof(s.id) - 1);
    strncpy(s.kode, kode, sizeof(s.kode) - 1);
    strncpy(s.nama, nama, sizeof(s.nama) - 1);
    s.mdpl = mdpl;
    strncpy(s.kota, kota, sizeof(s.kota) - 1);
    strncpy(s.alamat, alamat, sizeof(s.alamat) - 1);

    s.id[sizeof(s.id)-1] = '\0';
    s.kode[sizeof(s.kode)-1] = '\0';
    s.nama[sizeof(s.nama)-1] = '\0';
    s.kota[sizeof(s.kota)-1] = '\0';
    s.alamat[sizeof(s.alamat)-1] = '\0';

    sanitize_field(s.nama, sizeof(s.nama));
    sanitize_field(s.kode, sizeof(s.kode));
    sanitize_field(s.kota, sizeof(s.kota));
    sanitize_field(s.alamat, sizeof(s.alamat));

    s.active = 1;
    g_stasiun[g_stasiunCount++] = s;
    stasiun_save();
}


void stasiun_update(const char* id, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat) {
    int index = stasiun_find_index_by_id(id);
    if (index == -1) return;

    strncpy(g_stasiun[index].kode, kode, sizeof(g_stasiun[index].kode) - 1);
    g_stasiun[index].kode[sizeof(g_stasiun[index].kode) - 1] = '\0';

    strncpy(g_stasiun[index].nama, nama, sizeof(g_stasiun[index].nama) - 1);
    g_stasiun[index].nama[sizeof(g_stasiun[index].nama) - 1] = '\0';

    g_stasiun[index].mdpl = mdpl;

    strncpy(g_stasiun[index].kota, kota, sizeof(g_stasiun[index].kota) - 1);
    g_stasiun[index].kota[sizeof(g_stasiun[index].kota) - 1] = '\0';

    strncpy(g_stasiun[index].alamat, alamat, sizeof(g_stasiun[index].alamat) - 1);
    g_stasiun[index].alamat[sizeof(g_stasiun[index].alamat) - 1] = '\0';

    sanitize_field(g_stasiun[index].kode, sizeof(g_stasiun[index].kode));
    sanitize_field(g_stasiun[index].nama, sizeof(g_stasiun[index].nama));
    sanitize_field(g_stasiun[index].kota, sizeof(g_stasiun[index].kota));
    sanitize_field(g_stasiun[index].alamat, sizeof(g_stasiun[index].alamat));

    stasiun_save();
}

void stasiun_delete_by_id(const char* id) {
    int index = stasiun_find_index_by_id(id);
    if (index == -1) return;

    g_stasiun[index].active = 0;
    stasiun_save();
}

void stasiun_restore_by_id(const char* id) {
    int index = stasiun_find_index_by_id_all(id);
    if (index == -1) return;

    g_stasiun[index].active = 1;
    stasiun_save();
}

/* HELPER: FIND BY ID (PK) */
int stasiun_find_index_by_id_all(const char* id) {
    for (int i = 0; i < g_stasiunCount; i++) {
        if (strcmp(g_stasiun[i].id, id) == 0) return i;
    }
    return -1;
}

int stasiun_find_index_by_id(const char* id) {
    for (int i = 0; i < g_stasiunCount; i++) {
        if (g_stasiun[i].active && strcmp(g_stasiun[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}
