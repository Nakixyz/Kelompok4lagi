#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "modul_akun.h"
#include "globals.h"

#define FILE_AKUN "accounts.dat"

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
static void akun_save() {
    FILE *f = fopen(FILE_AKUN, "wb");
    if (f) {
        fwrite(g_accounts, sizeof(Account), g_accountCount, f);
        fclose(f);
    }
}

/* Load data saat aplikasi mulai */
void akun_init() {
    FILE *f = fopen(FILE_AKUN, "rb");
    if (f) {
        g_accountCount = (int)fread(g_accounts, sizeof(Account), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_accountCount = 0;
    }

    /* dipakai untuk seed awal (hanya saat file benar-benar kosong/tidak ada) */
    int was_empty = (g_accountCount == 0);

    /* Migrasi role lama (jika ada file versi sebelumnya) */
    int changed = 0;
    for (int i = 0; i < g_accountCount; i++) {
        int r = (int)g_accounts[i].role;

        /*
           Versi lama:
             0=SUPERADMIN, 1=PEMBAYARAN, 2=JADWAL, 3=DATA, 4=MANAGER
           Versi baru:
             0=SUPERADMIN, 1=TRANSAKSI, 2=DATA, 3=MANAGER
        */
        if (r == 1 || r == 2) { g_accounts[i].role = ROLE_TRANSAKSI; changed = 1; }
        else if (r == 3)      { g_accounts[i].role = ROLE_DATA;      changed = 1; }
        else if (r == 4)      { g_accounts[i].role = ROLE_MANAGER;   changed = 1; }
    }

    /* Pastikan akun 'admin' selalu ada dan selalu SUPERADMIN */
    int admin_idx = -1;
    for (int i = 0; i < g_accountCount; i++) {
        if (strcmp(g_accounts[i].username, "admin") == 0) {
            admin_idx = i;
            break;
        }
    }
    if (admin_idx >= 0) {
        if (!g_accounts[admin_idx].active || g_accounts[admin_idx].role != ROLE_SUPERADMIN) {
            g_accounts[admin_idx].active = 1;
            g_accounts[admin_idx].role = ROLE_SUPERADMIN;
            changed = 1;
        }
    } else if (g_accountCount < MAX_RECORDS) {
        Account admin;
        memset(&admin, 0, sizeof(admin));
        snprintf(admin.username, sizeof(admin.username), "%s", "admin");
        snprintf(admin.password, sizeof(admin.password), "%s", "admin");
        admin.role = ROLE_SUPERADMIN;
        admin.active = 1;
        g_accounts[g_accountCount++] = admin;
        changed = 1;
    }

    /* Seed data dummy hanya saat pertama kali run (file kosong/tidak ada) */
    if (was_empty) {
        /* Tambah 50 user dummy (role karyawan: data/transaksi/manager) */
        Role rotasi[] = { ROLE_DATA, ROLE_TRANSAKSI, ROLE_MANAGER };
        for (int i = 1; i <= 50 && g_accountCount < MAX_RECORDS; i++) {
            Account a;
            memset(&a, 0, sizeof(a));
            snprintf(a.username, sizeof(a.username), "user%02d", i);
            snprintf(a.password, sizeof(a.password), "pass%02d", i);

            a.role = rotasi[(i - 1) % 3];
            a.active = 1;
            g_accounts[g_accountCount++] = a;
        }
        akun_save();
    } else if (changed) {
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
