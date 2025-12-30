#ifndef MODELS_H
#define MODELS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ================= KONFIGURASI UMUM ================= */
#define MAX_INPUT       64
#define MAX_RECORDS     200

/* ================= 1. MASTER ACCOUNT ================= */
#define FILE_AKUN "accounts.dat"

typedef enum {
    ROLE_SUPERADMIN = 0,
    ROLE_PEMBAYARAN,
    ROLE_JADWAL,
    ROLE_DATA,
    ROLE_MANAGER
} Role;

typedef struct {
    char username[MAX_INPUT];
    char password[MAX_INPUT];
    Role role;
    int active; /* 1=aktif, 0=hapus */
} EmployeeAccount;

static EmployeeAccount g_accounts[MAX_RECORDS];
static int g_accountCount = 0;

static const char *roleToString(Role r) {
    switch (r) {
        case ROLE_SUPERADMIN: return "Superadmin";
        case ROLE_PEMBAYARAN: return "Pembayaran";
        case ROLE_JADWAL: return "Jadwal";
        case ROLE_DATA: return "Data";
        case ROLE_MANAGER: return "Manager";
        default: return "Unknown";
    }
}

static void akun_save() {
    FILE *f = fopen(FILE_AKUN, "wb");
    if (f) {
        fwrite(g_accounts, sizeof(EmployeeAccount), g_accountCount, f);
        fclose(f);
    }
}

static void akun_load() {
    FILE *f = fopen(FILE_AKUN, "rb");
    if (!f) { g_accountCount = 0; return; }
    g_accountCount = fread(g_accounts, sizeof(EmployeeAccount), MAX_RECORDS, f);
    fclose(f);
}

static void akun_seed() {
    if (g_accountCount > 0) return;
    for (int i = 1; i <= 50; i++) {
        EmployeeAccount e;
        snprintf(e.username, sizeof(e.username), "user%02d", i);
        snprintf(e.password, sizeof(e.password), "pass%02d", i);
        e.role = (i % 4) + 1; // Rotasi role
        e.active = 1;
        g_accounts[g_accountCount++] = e;
    }
    akun_save();
}

/* ================= 2. MASTER KARYAWAN ================= */
#define FILE_KARYAWAN "karyawan.dat"

typedef struct {
    char id[15];
    char nama[50];
    char email[50];
    char jabatan[30];
    int active;
} Karyawan;

static Karyawan g_karyawan[MAX_RECORDS];
static int g_karyawanCount = 0;

static void karyawan_save() {
    FILE *f = fopen(FILE_KARYAWAN, "wb");
    if (f) {
        fwrite(g_karyawan, sizeof(Karyawan), g_karyawanCount, f);
        fclose(f);
    }
}

static void karyawan_load() {
    FILE *f = fopen(FILE_KARYAWAN, "rb");
    if (!f) { g_karyawanCount = 0; return; }
    g_karyawanCount = fread(g_karyawan, sizeof(Karyawan), MAX_RECORDS, f);
    fclose(f);
}

static void karyawan_seed() {
    if (g_karyawanCount > 0) return;
    for (int i = 1; i <= 50; i++) {
        Karyawan k;
        snprintf(k.id, sizeof(k.id), "PEG%03d", i);
        snprintf(k.nama, sizeof(k.nama), "Karyawan %02d", i);
        snprintf(k.email, sizeof(k.email), "pegawai%02d@kai.id", i);
        snprintf(k.jabatan, sizeof(k.jabatan), (i%2==0)?"Staff":"Supervisor");
        k.active = 1;
        g_karyawan[g_karyawanCount++] = k;
    }
    karyawan_save();
}

/* ================= 3. MASTER STASIUN ================= */
#define FILE_STASIUN "stasiun.dat"

typedef struct {
    char id[10];
    char kode[10];
    char nama[50];
    int  mdpl;
    char kota[30];
    char alamat[100];
    int active;
} Stasiun;


static Stasiun g_stasiun[MAX_RECORDS];
static int g_stasiunCount = 0;

static void stasiun_save() {
    FILE *f = fopen(FILE_STASIUN, "wb");
    if (f) { fwrite(g_stasiun, sizeof(Stasiun), g_stasiunCount, f); fclose(f); }
}

static void stasiun_load() {
    FILE *f = fopen(FILE_STASIUN, "rb");
    if (!f) { g_stasiunCount = 0; return; }
    g_stasiunCount = fread(g_stasiun, sizeof(Stasiun), MAX_RECORDS, f);
    fclose(f);
}

static void stasiun_seed() {
    if (g_stasiunCount > 0) return;
    for (int i = 1; i <= 50; i++) {
        Stasiun s;
        snprintf(s.id, sizeof(s.id), "STS%02d", i);
        snprintf(s.nama, sizeof(s.nama), "Stasiun Kota %02d", i);
        snprintf(s.kota, sizeof(s.kota), "Kota %c%c", 'A'+(i%5), 'A'+(i%3));
        s.active = 1;
        g_stasiun[g_stasiunCount++] = s;
    }
    stasiun_save();
}

/* ================= 4. MASTER KERETA ================= */
#define FILE_KERETA "kereta.dat"

typedef struct {
    char kode[10];
    char nama[50];
    char kelas[20]; // Eksekutif, Bisnis, Ekonomi
    int gerbong;
    int active;
} Kereta;

static Kereta g_kereta[MAX_RECORDS];
static int g_keretaCount = 0;

static void kereta_save() {
    FILE *f = fopen(FILE_KERETA, "wb");
    if (f) { fwrite(g_kereta, sizeof(Kereta), g_keretaCount, f); fclose(f); }
}

static void kereta_load() {
    FILE *f = fopen(FILE_KERETA, "rb");
    if (!f) { g_keretaCount = 0; return; }
    g_keretaCount = fread(g_kereta, sizeof(Kereta), MAX_RECORDS, f);
    fclose(f);
}

static void kereta_seed() {
    if (g_keretaCount > 0) return;
    const char *cls[] = {"Eksekutif", "Bisnis", "Ekonomi"};
    for (int i = 1; i <= 50; i++) {
        Kereta k;
        snprintf(k.kode, sizeof(k.kode), "KA%03d", i);
        snprintf(k.nama, sizeof(k.nama), "Argo %02d", i);
        snprintf(k.kelas, sizeof(k.kelas), "%s", cls[i%3]);
        k.gerbong = 6 + (i%5);
        k.active = 1;
        g_kereta[g_keretaCount++] = k;
    }
    kereta_save();
}

/* ================= INIT ALL DATA ================= */
static void init_all_data() {
    akun_load();     akun_seed();
    karyawan_load(); karyawan_seed();
    stasiun_load();  stasiun_seed();
    kereta_load();   kereta_seed();
}

#endif