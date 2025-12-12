#ifndef MODUL_KARYAWAN_H
#define MODUL_KARYAWAN_H

#include "types.h"

void karyawan_init();      // Load file + Seed 50 data
void karyawan_create(const char* id, const char* nama, const char* email, const char* jabatan);
void karyawan_delete(int index);

#endif