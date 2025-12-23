#ifndef MODUL_PENUMPANG_H
#define MODUL_PENUMPANG_H

#include <stddef.h>

void penumpang_init();
void penumpang_create(const char* id, const char* nama, const char* email,
                      const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin);
void penumpang_create_auto(char *out_id, size_t out_sz,
                           const char* nama, const char* email,
                           const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin);
void penumpang_update(int index, const char* nama, const char* email,
                      const char* no_telp, const char* tanggal_lahir, const char* jenis_kelamin);
void penumpang_delete(int index);

#endif
