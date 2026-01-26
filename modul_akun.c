#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "modul_akun.h"
#include "globals.h"

#define FILE_AKUN "accounts.dat"

/*
   Format file akun dinaikkan ke v2 dengan header.
   Versi lama menyimpan array Account mentah tanpa versi.

   Catatan migrasi:
   - Skema role lama: 0=SUPERADMIN, 1=PEMBAYARAN, 2=JADWAL, 3=DATA, 4=MANAGER
   - Skema role baru: 0=SUPERADMIN, 1=TRANSAKSI, 2=DATA, 3=MANAGER

   Masalah bug yang dilaporkan user: role "DATA" (nilai 2 di skema baru)
   terkonversi menjadi TRANSAKSI saat startup karena migrasi dijalankan
   tanpa mendeteksi versi file.

   Solusi:
   - Tambah header + versi.
   - Migrasi role lama hanya dijalankan jika file benar-benar legacy.
     Heuristik aman: migrasi hanya bila ditemukan role=4 (hanya ada di skema lama).
   - File legacy tanpa header akan di-upgrade otomatis ke v2 setelah load.
*/

#define AKUN_MAGIC_LEN 8
static const unsigned char AKUN_MAGIC[AKUN_MAGIC_LEN] = { 'A','K','U','N','V','2','\0','\0' };
#define AKUN_VERSION 2u

typedef struct {
    unsigned char magic[AKUN_MAGIC_LEN];
    uint32_t version;
    uint32_t count;
} AkunFileHeader;

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

static int is_valid_employee_role(Role r) {
    return (r == ROLE_DATA || r == ROLE_TRANSAKSI || r == ROLE_MANAGER);
}

/* Helper untuk menyimpan array ke file */
/* =========================
   Helper akun (internal)
   ========================= */
static int akun_find_by_username(const char *uname) {
    for (int i = 0; i < g_accountCount; i++) {
        if (strcmp(g_accounts[i].username, uname) == 0) return i;
    }
    return -1;
}

/* Deduplicate berdasarkan username (soft remove duplikat).
   Jika ada pointer changed_flag, set ke 1 jika ada yang dinonaktifkan. */
static void akun_dedup_by_username(int *changed_flag) {
    for (int i = 0; i < g_accountCount; i++) {
        if (!g_accounts[i].active) continue;
        for (int j = i + 1; j < g_accountCount; j++) {
            if (!g_accounts[j].active) continue;
            if (strcmp(g_accounts[i].username, g_accounts[j].username) == 0) {
                g_accounts[j].active = 0;
                if (changed_flag) *changed_flag = 1;
            }
        }
    }
}

void akun_save() {
    FILE *f = fopen(FILE_AKUN, "wb");
    if (f) {
        AkunFileHeader h;
        memset(&h, 0, sizeof(h));
        memcpy(h.magic, AKUN_MAGIC, AKUN_MAGIC_LEN);
        h.version = AKUN_VERSION;
        h.count = (uint32_t)g_accountCount;

        fwrite(&h, sizeof(h), 1, f);
        fwrite(g_accounts, sizeof(Account), (size_t)g_accountCount, f);
        fclose(f);
    }
}

/* Load data saat aplikasi mulai */
void akun_init(void) {
    int file_has_header = 0;

    FILE *f = fopen(FILE_AKUN, "rb");
    if (f) {
        AkunFileHeader h;
        size_t n = fread(&h, sizeof(h), 1, f);
        if (n == 1 && memcmp(h.magic, AKUN_MAGIC, AKUN_MAGIC_LEN) == 0 && h.version == AKUN_VERSION) {
            file_has_header = 1;
            uint32_t want = h.count;
            if (want > (uint32_t)MAX_RECORDS) want = (uint32_t)MAX_RECORDS;
            g_accountCount = (int)fread(g_accounts, sizeof(Account), (size_t)want, f);
        } else {
            /* Legacy (tanpa header): reset ke awal lalu baca mentah */
            fseek(f, 0, SEEK_SET);
            g_accountCount = (int)fread(g_accounts, sizeof(Account), MAX_RECORDS, f);
        }
        fclose(f);
    } else {
        g_accountCount = 0;
    }

    /* dipakai untuk seed awal (hanya saat file benar-benar kosong/tidak ada) */
    int was_empty = (g_accountCount == 0);

    /* Upgrade format legacy -> v2 (header), dilakukan setelah load sukses */
    int upgrade_format = (!file_has_header && g_accountCount > 0);

    /* Migrasi role lama (hanya untuk file legacy yang benar-benar skema lama) */
    int changed = 0;
    int legacy_role_scheme = 0;
    if (!file_has_header) {
        for (int i = 0; i < g_accountCount; i++) {
            int r = (int)g_accounts[i].role;
            if (r == 4) { legacy_role_scheme = 1; break; }
        }
    }

    if (legacy_role_scheme) {
        for (int i = 0; i < g_accountCount; i++) {
            int r = (int)g_accounts[i].role;
            if (r == 1 || r == 2) { g_accounts[i].role = ROLE_TRANSAKSI; changed = 1; }
            else if (r == 3)      { g_accounts[i].role = ROLE_DATA;      changed = 1; }
            else if (r == 4)      { g_accounts[i].role = ROLE_MANAGER;   changed = 1; }
        }
    }

    
    /* =========================
       ENSURE DEFAULT ACCOUNTS
       - Tidak ada dummy massal akun.
       - Pastikan akun demo dasar ada: admin (SUPERADMIN), kasir (TRANSAKSI), manager (MANAGER)
       - Pastikan SUPERADMIN 'admin' hanya 1 (dedup berdasarkan username)
       ========================= */

    /* Dedup username: jika ada username sama, nonaktifkan duplikat (keep yang pertama aktif) */
    akun_dedup_by_username(&changed);

    /* Pastikan 'admin' ada dan SUPERADMIN */
    int admin_idx = akun_find_by_username("admin");
    if (admin_idx >= 0) {
        if (!g_accounts[admin_idx].active || g_accounts[admin_idx].role != ROLE_SUPERADMIN) {
            g_accounts[admin_idx].active = 1;
            g_accounts[admin_idx].role = ROLE_SUPERADMIN;
            changed = 1;
        }
    } else if (g_accountCount < MAX_RECORDS) {
        Account a;
        memset(&a, 0, sizeof(a));
        snprintf(a.username, sizeof(a.username), "admin");
        snprintf(a.password, sizeof(a.password), "admin");
        a.role = ROLE_SUPERADMIN;
        a.active = 1;
        g_accounts[g_accountCount++] = a;
        changed = 1;
        admin_idx = g_accountCount - 1;
    }

    /* Pastikan hanya 1 akun SUPERADMIN 'admin' yang aktif (jaga kalau file sudah terlanjur dobel) */
    for (int i = 0; i < g_accountCount; i++) {
        if (i == admin_idx) continue;
        if (g_accounts[i].active && strcmp(g_accounts[i].username, "admin") == 0) {
            g_accounts[i].active = 0;
            changed = 1;
        }
    }

    /* kasir: transaksi */
    if (akun_find_by_username("kasir") < 0 && g_accountCount < MAX_RECORDS) {
        Account a;
        memset(&a, 0, sizeof(a));
        snprintf(a.username, sizeof(a.username), "kasir");
        snprintf(a.password, sizeof(a.password), "kasir");
        a.role = ROLE_TRANSAKSI;
        a.active = 1;
        g_accounts[g_accountCount++] = a;
        changed = 1;
    }

    /* manager: laporan */
    if (akun_find_by_username("manager") < 0 && g_accountCount < MAX_RECORDS) {
        Account a;
        memset(&a, 0, sizeof(a));
        snprintf(a.username, sizeof(a.username), "manager");
        snprintf(a.password, sizeof(a.password), "manager");
        a.role = ROLE_MANAGER;
        a.active = 1;
        g_accounts[g_accountCount++] = a;
        changed = 1;
    }

    /* Simpan jika ada perubahan, atau sekadar upgrade format ke v2 */
    if (was_empty || changed || upgrade_format) {
        akun_save();
    }
}


int akun_create(const char* username, const char* password, Role role) {
    if (g_accountCount >= MAX_RECORDS) return 0;
    if (is_blank_str(username) || is_blank_str(password)) return 0;
    if (has_whitespace(username) || has_whitespace(password)) return 0;
    if (!is_valid_employee_role(role)) return 0; /* hanya role karyawan */

    /* Cek duplikat username (aktif) */
    for (int i = 0; i < g_accountCount; i++) {
        if (g_accounts[i].active && strcmp(g_accounts[i].username, username) == 0) {
            return 0;
        }
    }

    Account a;
    memset(&a, 0, sizeof(a));
    snprintf(a.username, sizeof(a.username), "%s", username);
    snprintf(a.password, sizeof(a.password), "%s", password);
    a.role = role;
    a.active = 1;

    g_accounts[g_accountCount++] = a;
    akun_save();
    return 1;
}

void akun_delete(int index) {
    if (index < 0 || index >= g_accountCount) return;
    if (strcmp(g_accounts[index].username, "admin") == 0) return; /* admin tidak boleh dihapus */
    g_accounts[index].active = 0; /* Soft delete */
    akun_save();
}

void akun_change_password(int index, const char* new_pass) {
    if (index < 0 || index >= g_accountCount) return;
    if (is_blank_str(new_pass)) return;
    if (has_whitespace(new_pass)) return;

    snprintf(g_accounts[index].password, sizeof(g_accounts[index].password), "%s", new_pass);
    akun_save();
}

int akun_update_username(int index, const char* new_username) {
    if (index < 0 || index >= g_accountCount) return 0;
    if (!g_accounts[index].active) return 0;
    if (strcmp(g_accounts[index].username, "admin") == 0) return 0;
    if (is_blank_str(new_username)) return 0;
    if (has_whitespace(new_username)) return 0;

    /* Cek duplikat */
    for (int i = 0; i < g_accountCount; i++) {
        if (i == index) continue;
        if (g_accounts[i].active && strcmp(g_accounts[i].username, new_username) == 0) return 0;
    }

    snprintf(g_accounts[index].username, sizeof(g_accounts[index].username), "%s", new_username);
    akun_save();
    return 1;
}

int akun_update_role(int index, Role new_role) {
    if (index < 0 || index >= g_accountCount) return 0;
    if (!g_accounts[index].active) return 0;
    if (strcmp(g_accounts[index].username, "admin") == 0) return 0;
    if (!is_valid_employee_role(new_role)) return 0;

    g_accounts[index].role = new_role;
    akun_save();
    return 1;
}
