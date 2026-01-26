#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

/* Extern artinya: variabel ini ada, tapi isinya ada di file .c */
extern Account g_accounts[MAX_RECORDS];
extern int g_accountCount;

extern Karyawan g_karyawan[MAX_RECORDS];
extern int g_karyawanCount;

extern Penumpang g_penumpang[MAX_RECORDS];
extern int g_penumpangCount;

extern Stasiun g_stasiun[MAX_RECORDS];
extern int g_stasiunCount;

extern Kereta g_kereta[MAX_RECORDS];
extern int g_keretaCount;


extern JadwalTiket g_jadwal[MAX_RECORDS];
extern int g_jadwalCount;

extern PembayaranTiket g_pembayaran[MAX_RECORDS];
extern int g_pembayaranCount;

#endif
