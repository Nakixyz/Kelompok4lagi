#ifndef MODUL_STASIUN_H
#define MODUL_STASIUN_H

#include <stddef.h>

void stasiun_init();
void stasiun_create(const char* id, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat, const char* status);
void stasiun_create_auto(char *out_id, size_t out_sz,
                         const char* kode, const char* nama,
                         int mdpl, const char* kota, const char* alamat, const char* status);
void stasiun_update(int index, const char* kode, const char* nama,
                    int mdpl, const char* kota, const char* alamat, const char* status);
void stasiun_delete(int index);

#endif
