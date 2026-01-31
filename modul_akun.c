#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "modul_akun.h"
#include "globals.h"

#define FILE_AKUN "accounts.dat"

/*
   Format file akun menggunakan header + version.

   v2 (legacy) : header AKUNV2 + struct Account lama (username,password,role,active)
   v1 (lebih lama): tanpa header, langsung array struct Account lama

   v3 (sekarang): header AKUNV3 + struct Account baru:
     email, nama, id_karyawan(PEG###), password, role, active

   Migrasi:
   - username legacy -> email = username + "@kai.id" (jika belum ada '@')
   - nama default = username
   - id_karyawan di-generate berurutan PEG001 dst (stabil di file setelah disimpan)
*/

#define AKUN_MAGIC_LEN 8
static const unsigned char AKUN_MAGIC_V3[AKUN_MAGIC_LEN] = { 'A','K','U','N','V','3','\0','\0' };
static const unsigned char AKUN_MAGIC_V2[AKUN_MAGIC_LEN] = { 'A','K','U','N','V','2','\0','\0' };

#define AKUN_VERSION_V3 3u

typedef struct {
    unsigned char magic[AKUN_MAGIC_LEN];
    uint32_t version;
    uint32_t count;
} AkunFileHeader;

/* Legacy struct untuk baca file lama (v1/v2) */
typedef struct {
    char username[MAX_INPUT];
    char password[MAX_INPUT];
    Role role;
    int active;
} AccountLegacyV2;

static int is_blank_str(const char *s) {
    if (!s) return 1;
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

static int has_whitespace(const char *s) {
    if (!s) return 0;
    while (*s) {
        if (isspace((unsigned char)*s)) return 1;
        s++;
    }
    return 0;
}

static void str_to_lower(char *s) {
    if (!s) return;
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static int ends_with_kai_id(const char *email_lower) {
    if (!email_lower) return 0;
    const char *at = strrchr(email_lower, '@');
    if (!at) return 0;
    return (strcmp(at, "@kai.id") == 0);
}

/*
   Normalisasi email untuk login:
   - jika tidak ada '@', tambahkan "@kai.id"
   - lowercase
   Return: 1 jika hasil valid domain @kai.id, 0 jika domain selain kai.id.
*/
int akun_normalize_login_email(const char *input, char *out, int out_sz) {
    if (!out || out_sz <= 0) return 0;
    out[0] = '\0';
    if (!input) return 0;

    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", input);

    /* trim spasi ujung (harusnya tidak ada dari UI login, tapi aman) */
    size_t len = strlen(tmp);
    while (len > 0 && isspace((unsigned char)tmp[len - 1])) tmp[--len] = '\0';
    size_t start = 0;
    while (tmp[start] && isspace((unsigned char)tmp[start])) start++;
    if (start > 0) memmove(tmp, tmp + start, strlen(tmp + start) + 1);

    if (tmp[0] == '\0') return 0;

    /* auto append domain jika user mengetik tanpa '@' (contoh: admin -> admin@kai.id) */
    if (!strchr(tmp, '@')) {
        char tmp2[256];
        snprintf(tmp2, sizeof(tmp2), "%s@kai.id", tmp);
        snprintf(tmp, sizeof(tmp), "%s", tmp2);
    }

    str_to_lower(tmp);
    if (!ends_with_kai_id(tmp)) return 0;

    snprintf(out, (size_t)out_sz, "%s", tmp);
    return 1;
}

static int is_valid_employee_role(Role r) {
    return (r == ROLE_DATA || r == ROLE_TRANSAKSI || r == ROLE_MANAGER);
}

static int akun_find_by_email_lower(const char *email_lower) {
    if (!email_lower) return -1;
    for (int i = 0; i < g_accountCount; i++) {
        char e[128];
        snprintf(e, sizeof(e), "%s", g_accounts[i].email);
        str_to_lower(e);
        if (strcmp(e, email_lower) == 0) return i;
    }
    return -1;
}

static void akun_dedup_by_email(int *changed_flag) {
    for (int i = 0; i < g_accountCount; i++) {
        if (!g_accounts[i].active) continue;
        char ei[128];
        snprintf(ei, sizeof(ei), "%s", g_accounts[i].email);
        str_to_lower(ei);
        for (int j = i + 1; j < g_accountCount; j++) {
            if (!g_accounts[j].active) continue;
            char ej[128];
            snprintf(ej, sizeof(ej), "%s", g_accounts[j].email);
            str_to_lower(ej);
            if (strcmp(ei, ej) == 0) {
                g_accounts[j].active = 0;
                if (changed_flag) *changed_flag = 1;
            }
        }
    }
}

static int parse_peg_num(const char *s) {
    if (!s) return -1;
    if (strlen(s) < 4) return -1;
    if (!(s[0] == 'P' && s[1] == 'E' && s[2] == 'G')) return -1;
    int n = 0;
    int any = 0;
    for (const char *p = s + 3; *p; p++) {
        if (!isdigit((unsigned char)*p)) return -1;
        any = 1;
        n = n * 10 + (*p - '0');
        if (n > 1000000) break;
    }
    return any ? n : -1;
}

static void format_peg_id(int num, char *out, int out_sz) {
    if (!out || out_sz <= 0) return;
    if (num < 0) num = 0;
    /* tetap pakai PEG%03d, jika >999 akan menjadi PEG1000 (masih valid) */
    snprintf(out, (size_t)out_sz, "PEG%03d", num);
}

static int next_peg_number(void) {
    int maxn = 0;
    for (int i = 0; i < g_accountCount; i++) {
        int n = parse_peg_num(g_accounts[i].id_karyawan);
        if (n > maxn) maxn = n;
    }
    return maxn + 1;
}

static void akun_save(void) {
    FILE *f = fopen(FILE_AKUN, "wb");
    if (!f) return;

    AkunFileHeader h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, AKUN_MAGIC_V3, AKUN_MAGIC_LEN);
    h.version = AKUN_VERSION_V3;
    h.count = (uint32_t)g_accountCount;

    fwrite(&h, sizeof(h), 1, f);
    fwrite(g_accounts, sizeof(Account), (size_t)g_accountCount, f);
    fclose(f);
}

static void migrate_from_legacy(const AccountLegacyV2 *old_arr, int old_count) {
    g_accountCount = 0;
    if (!old_arr || old_count <= 0) return;

    for (int i = 0; i < old_count && g_accountCount < MAX_RECORDS; i++) {
        Account a;
        memset(&a, 0, sizeof(a));

        char email_norm[128];
        if (!akun_normalize_login_email(old_arr[i].username, email_norm, (int)sizeof(email_norm))) {
            /* jika username legacy kosong/aneh, lewati */
            continue;
        }

        snprintf(a.email, sizeof(a.email), "%s", email_norm);
        /* nama legacy: pakai username mentah */
        snprintf(a.nama, sizeof(a.nama), "%s", old_arr[i].username);
        snprintf(a.password, sizeof(a.password), "%s", old_arr[i].password);
        a.role = old_arr[i].role;
        a.active = old_arr[i].active;
        a.id_karyawan[0] = '\0'; /* isi nanti */

        g_accounts[g_accountCount++] = a;
    }

    /* normalisasi role legacy (jaga-jaga jika masih skema lama):
       skema lama: 0=SUPERADMIN, 1=PEMBAYARAN, 2=JADWAL, 3=DATA, 4=MANAGER
       skema baru: 0=SUPERADMIN, 1=TRANSAKSI, 2=DATA, 3=MANAGER
    */
    int legacy_role_scheme = 0;
    for (int i = 0; i < g_accountCount; i++) {
        if ((int)g_accounts[i].role == 4) { legacy_role_scheme = 1; break; }
    }
    if (legacy_role_scheme) {
        for (int i = 0; i < g_accountCount; i++) {
            int r = (int)g_accounts[i].role;
            if (r == 1 || r == 2) g_accounts[i].role = ROLE_TRANSAKSI;
            else if (r == 3)      g_accounts[i].role = ROLE_DATA;
            else if (r == 4)      g_accounts[i].role = ROLE_MANAGER;
        }
    }

    /* Generate PEG id untuk semua (stabil) */
    int peg = 1;
    for (int i = 0; i < g_accountCount; i++) {
        format_peg_id(peg++, g_accounts[i].id_karyawan, (int)sizeof(g_accounts[i].id_karyawan));
    }
}

void akun_init(void) {
    int changed = 0;
    int loaded_v3 = 0;

    FILE *f = fopen(FILE_AKUN, "rb");
    if (f) {
        AkunFileHeader h;
        size_t n = fread(&h, sizeof(h), 1, f);
        if (n == 1 && memcmp(h.magic, AKUN_MAGIC_V3, AKUN_MAGIC_LEN) == 0 && h.version == AKUN_VERSION_V3) {
            loaded_v3 = 1;
            uint32_t want = h.count;
            if (want > (uint32_t)MAX_RECORDS) want = (uint32_t)MAX_RECORDS;
            g_accountCount = (int)fread(g_accounts, sizeof(Account), (size_t)want, f);
        } else if (n == 1 && memcmp(h.magic, AKUN_MAGIC_V2, AKUN_MAGIC_LEN) == 0 && h.version == 2u) {
            /* v2: struct legacy */
            uint32_t want = h.count;
            if (want > (uint32_t)MAX_RECORDS) want = (uint32_t)MAX_RECORDS;
            AccountLegacyV2 tmp[MAX_RECORDS];
            int got = (int)fread(tmp, sizeof(AccountLegacyV2), (size_t)want, f);
            migrate_from_legacy(tmp, got);
            changed = 1;
        } else {
            /* v1: tanpa header (raw legacy) */
            fseek(f, 0, SEEK_SET);
            AccountLegacyV2 tmp[MAX_RECORDS];
            int got = (int)fread(tmp, sizeof(AccountLegacyV2), MAX_RECORDS, f);
            migrate_from_legacy(tmp, got);
            changed = 1;
        }
        fclose(f);
    } else {
        g_accountCount = 0;
    }

    /* Normalisasi email ke lowercase + validasi domain; jika bukan @kai.id -> nonaktifkan */
    for (int i = 0; i < g_accountCount; i++) {
        char norm[128];
        if (!akun_normalize_login_email(g_accounts[i].email, norm, (int)sizeof(norm))) {
            if (g_accounts[i].active) { g_accounts[i].active = 0; changed = 1; }
            continue;
        }
        if (strcmp(g_accounts[i].email, norm) != 0) {
            snprintf(g_accounts[i].email, sizeof(g_accounts[i].email), "%s", norm);
            changed = 1;
        }
        if (g_accounts[i].id_karyawan[0] == '\0') {
            format_peg_id(next_peg_number(), g_accounts[i].id_karyawan, (int)sizeof(g_accounts[i].id_karyawan));
            changed = 1;
        }
    }

    akun_dedup_by_email(&changed);

    /* Ensure default accounts (sesuai permintaan) */
    struct {
        const char *email;
        const char *nama;
        const char *password;
        Role role;
    } defs[] = {
        { "admin@kai.id",   "admin",   "admin",   ROLE_SUPERADMIN },
        { "kasir@kai.id",   "kasir",   "kasir",   ROLE_TRANSAKSI  },
        { "manager@kai.id", "manager", "manager", ROLE_MANAGER    },
        { "data@kai.id",    "data",    "data",    ROLE_DATA       },
    };

    for (int d = 0; d < (int)(sizeof(defs)/sizeof(defs[0])); d++) {
        char email_l[128];
        akun_normalize_login_email(defs[d].email, email_l, (int)sizeof(email_l));
        int idx = akun_find_by_email_lower(email_l);
        if (idx >= 0) {
            /* pastikan aktif & role sesuai utk default (khusus admin wajib superadmin) */
            if (!g_accounts[idx].active) { g_accounts[idx].active = 1; changed = 1; }
            if (g_accounts[idx].role != defs[d].role) { g_accounts[idx].role = defs[d].role; changed = 1; }
            if (is_blank_str(g_accounts[idx].nama)) { snprintf(g_accounts[idx].nama, sizeof(g_accounts[idx].nama), "%s", defs[d].nama); changed = 1; }
            if (g_accounts[idx].id_karyawan[0] == '\0') { format_peg_id(next_peg_number(), g_accounts[idx].id_karyawan, (int)sizeof(g_accounts[idx].id_karyawan)); changed = 1; }
        } else if (g_accountCount < MAX_RECORDS) {
            Account a;
            memset(&a, 0, sizeof(a));
            snprintf(a.email, sizeof(a.email), "%s", email_l);
            snprintf(a.nama, sizeof(a.nama), "%s", defs[d].nama);
            snprintf(a.password, sizeof(a.password), "%s", defs[d].password);
            a.role = defs[d].role;
            a.active = 1;
            format_peg_id(next_peg_number(), a.id_karyawan, (int)sizeof(a.id_karyawan));
            g_accounts[g_accountCount++] = a;
            changed = 1;
        }
    }

    /* Pastikan hanya 1 admin aktif */
    {
        char admin_l[128];
        akun_normalize_login_email("admin@kai.id", admin_l, (int)sizeof(admin_l));
        int admin_idx = akun_find_by_email_lower(admin_l);
        for (int i = 0; i < g_accountCount; i++) {
            if (i == admin_idx) continue;
            char ei[128];
            akun_normalize_login_email(g_accounts[i].email, ei, (int)sizeof(ei));
            if (strcmp(ei, admin_l) == 0 && g_accounts[i].active) {
                g_accounts[i].active = 0;
                changed = 1;
            }
        }
    }

    /* Save jika ada perubahan atau file belum v3 */
    if (changed || !loaded_v3) {
        akun_save();
    }
}

int akun_create(const char* email, const char* nama, const char* password, Role role) {
    if (g_accountCount >= MAX_RECORDS) return 0;
    if (is_blank_str(email) || is_blank_str(nama) || is_blank_str(password)) return 0;
    if (has_whitespace(email) || has_whitespace(password)) return 0;
    if (!is_valid_employee_role(role)) return 0; /* hanya role karyawan */

    char email_norm[128];
    if (!akun_normalize_login_email(email, email_norm, (int)sizeof(email_norm))) return 0;

    /* Cek duplikat email (aktif) */
    for (int i = 0; i < g_accountCount; i++) {
        if (!g_accounts[i].active) continue;
        char e[128];
        snprintf(e, sizeof(e), "%s", g_accounts[i].email);
        str_to_lower(e);
        if (strcmp(e, email_norm) == 0) return 0;
    }

    Account a;
    memset(&a, 0, sizeof(a));
    snprintf(a.email, sizeof(a.email), "%s", email_norm);
    snprintf(a.nama, sizeof(a.nama), "%s", nama);
    snprintf(a.password, sizeof(a.password), "%s", password);
    a.role = role;
    a.active = 1;
    format_peg_id(next_peg_number(), a.id_karyawan, (int)sizeof(a.id_karyawan));

    g_accounts[g_accountCount++] = a;
    akun_save();
    return 1;
}

void akun_delete(int index) {
    if (index < 0 || index >= g_accountCount) return;
    if (strcmp(g_accounts[index].email, "admin@kai.id") == 0) return; /* admin tidak boleh dihapus */
    g_accounts[index].active = 0;
    akun_save();
}

void akun_change_password(int index, const char* new_pass) {
    if (index < 0 || index >= g_accountCount) return;
    if (is_blank_str(new_pass)) return;
    if (has_whitespace(new_pass)) return;

    snprintf(g_accounts[index].password, sizeof(g_accounts[index].password), "%s", new_pass);
    akun_save();
}

int akun_update_email(int index, const char* new_email) {
    if (index < 0 || index >= g_accountCount) return 0;
    if (!g_accounts[index].active) return 0;
    if (strcmp(g_accounts[index].email, "admin@kai.id") == 0) return 0;
    if (is_blank_str(new_email)) return 0;
    if (has_whitespace(new_email)) return 0;

    char email_norm[128];
    if (!akun_normalize_login_email(new_email, email_norm, (int)sizeof(email_norm))) return 0;

    for (int i = 0; i < g_accountCount; i++) {
        if (i == index) continue;
        if (!g_accounts[i].active) continue;
        char e[128];
        snprintf(e, sizeof(e), "%s", g_accounts[i].email);
        str_to_lower(e);
        if (strcmp(e, email_norm) == 0) return 0;
    }

    snprintf(g_accounts[index].email, sizeof(g_accounts[index].email), "%s", email_norm);
    akun_save();
    return 1;
}

int akun_update_nama(int index, const char* new_nama) {
    if (index < 0 || index >= g_accountCount) return 0;
    if (!g_accounts[index].active) return 0;
    if (is_blank_str(new_nama)) return 0;
    snprintf(g_accounts[index].nama, sizeof(g_accounts[index].nama), "%s", new_nama);
    akun_save();
    return 1;
}

int akun_update_role(int index, Role new_role) {
    if (index < 0 || index >= g_accountCount) return 0;
    if (!g_accounts[index].active) return 0;
    if (strcmp(g_accounts[index].email, "admin@kai.id") == 0) return 0;
    if (!is_valid_employee_role(new_role)) return 0;

    g_accounts[index].role = new_role;
    akun_save();
    return 1;
}
