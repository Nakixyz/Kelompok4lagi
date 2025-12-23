#ifndef TYPES_H
#define TYPES_H

#define MAX_INPUT 64
#define MAX_RECORDS 200

typedef enum {
    /* 0 = superadmin (khusus admin) */
    ROLE_SUPERADMIN = 0,

    /* 3 role untuk akun karyawan */
    ROLE_TRANSAKSI  = 1,  /* pembayaran tiket + jadwal */
    ROLE_DATA       = 2,  /* kelola penumpang, kereta, stasiun */
    ROLE_MANAGER    = 3   /* kelola laporan (CRUD belum dibuat) */
} Role;

/* Master 1: Akun */
typedef struct {
    char username[MAX_INPUT];
    char password[MAX_INPUT];
    Role role;
    int active;
} Account;

/* Master 2: Karyawan */
typedef struct {
    char id[15];
    char nama[50];
    char email[50];
    char jabatan[30];
    int active;
} Karyawan;

/* Master 3: Penumpang */
typedef struct {
    char id[15];
    char nama[50];
    char email[50];
    char no_telp[20];
    char tanggal_lahir[16];   /* contoh: 01-01-2000 */
    char jenis_kelamin[16];   /* L / P */
    int active;
} Penumpang;

/* Master 4: Stasiun */
typedef struct {
    char id[10];
    char kode[10];
    char nama[50];
    int  mdpl;
    char kota[50];
    char alamat[80];
    char status[16];
    int active;
} Stasiun;

/* Master 5: Kereta */
typedef struct {
    char kode[10];
    char nama[50];
    char kelas[20];
    int  kapasitas;
    int  gerbong;
    char status[16];
    int active;
} Kereta;

#endif
