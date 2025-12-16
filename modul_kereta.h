#ifndef MODUL_KERETA_H
#define MODUL_KERETA_H

#include "types.h"

/* Data Layer */
void kereta_init();
void kereta_create(const char* kode, const char* nama, const char* kelas, int gerbong);
void kereta_update(int index, const char* nama, const char* kelas, int gerbong);
void kereta_delete(int index);

/* UI Entry (dipanggil dari ui_app / dashboard) */
void view_kereta();

#endif
