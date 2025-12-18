#ifndef TYPES_H
#define TYPES_H

#define MAX_INPUT 64
#define MAX_RECORDS 200

typedef enum {
    ROLE_SUPERADMIN = 0,
    ROLE_PEMBAYARAN,
    ROLE_JADWAL,
    ROLE_DATA,
    ROLE_MANAGER
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

/* Master 3: Stasiun */
typedef struct {
    char id[10];
    char nama[50];
    char kota[50];
    int active;
} Stasiun;

/* Master 4: Kereta */
typedef struct {
    char kode[10];
    char nama[50];
    char kelas[20];
    int gerbong;
    int active;
} Kereta;

#endif
