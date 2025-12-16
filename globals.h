#ifndef GLOBALS_Hn
#define GLOBALS_H
#include "types.h"

/* Extern artinya: variabel ini ada, tapi isinya ada di file .c */
extern Account g_accounts[MAX_RECORDS];
extern int g_accountCount;

extern Karyawan g_karyawan[MAX_RECORDS];
extern int g_karyawanCount;

extern Stasiun g_stasiun[MAX_RECORDS];
extern int g_stasiunCount;

extern Kereta g_kereta[MAX_RECORDS];
extern int g_keretaCount;

#endif