#ifndef MODUL_KERETA_H
#define MODUL_KERETA_H

void kereta_init();
void kereta_create(const char* kode, const char* nama, const char* kelas, int gerbong);
void kereta_delete(int index);

#endif