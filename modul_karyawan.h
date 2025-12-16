#ifndef MODUL_KARYAWAN_H
#define MODUL_KARYAWAN_H

#include <stddef.h>
#include "types.h"

void karyawan_init();
void karyawan_create(const char* id, const char* nama, const char* email, const char* jabatan);
void karyawan_create_auto(char *out_id, size_t out_sz,
                          const char* nama, const char* email, const char* jabatan);
void karyawan_delete(int index);
void karyawan_update(int index, const char* nama, const char* email, const char* jabatan);

#endif
