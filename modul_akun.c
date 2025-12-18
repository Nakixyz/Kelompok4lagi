#include <stdio.h>
#include <string.h>
#include "modul_akun.h"
#include "globals.h"

#define FILE_AKUN "accounts.dat"

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
        g_accountCount = fread(g_accounts, sizeof(Account), MAX_RECORDS, f);
        fclose(f);
    } else {
        g_accountCount = 0;
    }

    /* Seed Data Dummy jika kosong */
    if (g_accountCount == 0) {
        // Tambah 1 superadmin default
        Account admin;
        strcpy(admin.username, "admin");
        strcpy(admin.password, "admin");
        admin.role = ROLE_SUPERADMIN;
        admin.active = 1;
        g_accounts[g_accountCount++] = admin;

        // Tambah 50 user dummy
        for (int i = 1; i <= 50; i++) {
            Account a;
            snprintf(a.username, sizeof(a.username), "user%02d", i);
            snprintf(a.password, sizeof(a.password), "pass%02d", i);

            // Rotasi role (1=Bayar, 2=Jadwal, 3=Data, 4=Manager)
            a.role = (i % 4) + 1;
            a.active = 1;
            g_accounts[g_accountCount++] = a;
        }
        akun_save();
    }
}

int akun_create(const char* username, const char* password, Role role) {
    if (g_accountCount >= MAX_RECORDS) return 0;

    // Cek duplikat username
    for(int i=0; i<g_accountCount; i++) {
        if(g_accounts[i].active && strcmp(g_accounts[i].username, username) == 0) {
            return 0;
        }
    }

    Account a;
    strncpy(a.username, username, MAX_INPUT);
    strncpy(a.password, password, MAX_INPUT);
    a.role = role;
    a.active = 1;

    g_accounts[g_accountCount++] = a;
    akun_save();
    return 1;
}

void akun_delete(int index) {
    if (index >= 0 && index < g_accountCount) {
        g_accounts[index].active = 0;
        akun_save();
    }
}

void akun_change_password(int index, const char* new_pass) {
    if (index >= 0 && index < g_accountCount) {
        strncpy(g_accounts[index].password, new_pass, MAX_INPUT);
        akun_save();
    }
}
