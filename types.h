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


/* ================= TRANSAKSI ================= */

/* Transaksi 1: Jadwal Tiket */
typedef struct {
    char id_jadwal[20];           /* contoh: JDW001 */
    char tgl_dibuat[16];        /* format: DD-MM-YYYY (diinput saat buat jadwal) */
    char id_kereta[10];           /* pakai Kereta.kode (mis: KA01) */
    char id_stasiun_asal[10];     /* pakai Stasiun.id (mis: STS01) */
    char id_stasiun_tujuan[10];   /* pakai Stasiun.id (mis: STS02) */
    char waktu_berangkat[32];     /* format: DD-MM-YYYY HH:MM */
    char waktu_tiba[32];          /* format: DD-MM-YYYY HH:MM */
    int  harga_tiket;
    int  kuota_kursi;             /* kuota tersedia saat ini */
    char id_karyawan[15];         /* karyawan yang membuat */
    int  active;                  /* soft delete */
} JadwalTiket;

/* Transaksi 2: Pembayaran Tiket
   Catatan: Primary key = ID transaksi (mis. TRX001) yang digenerate otomatis.
   Tanggal dibuat tetap disimpan di tgl_pembayaran (datetime). */
typedef struct {
    char id_pembayaran[32];       /* primary key: TRX### (dibuat otomatis) */
    char tgl_dibuat[16];          /* format: DD-MM-YYYY (diinput saat transaksi) */
    char id_penumpang[15];
    char id_jadwal[20];
    char tgl_pembayaran[32];      /* format: YYYYMMDDHHMMSS (dibuat otomatis; audit) */
    int  jumlah_tiket;
    int  harga_satuan;            /* diambil dari JadwalTiket.harga_tiket */
    int  total_harga;             /* harga_satuan * jumlah_tiket */
    char metode_pembayaran[20];   /* contoh: TUNAI / DEBIT / QRIS */
    char status_pembayaran[20];   /* contoh: LUNAS / PENDING */
    char id_karyawan[15];         /* karyawan transaksi yang memproses */
    int  active;                  /* soft delete */
} PembayaranTiket;


#endif