#ifndef MASTER_KERETA_H
#define MASTER_KERETA_H

#include <stddef.h>
#include "types.h"

void kereta_init(void);

void kereta_create(const char* kode, const char* nama, const char* kelas,
                   int kapasitas, int gerbong, const char* status);

void kereta_create_auto(char *out_id, size_t out_sz,
                        const char* nama, const char* kelas,
                        int kapasitas, int gerbong, const char* status);

void kereta_update(int index, const char* nama, const char* kelas,
                   int kapasitas, int gerbong, const char* status);

void kereta_delete(int index);

#endif
