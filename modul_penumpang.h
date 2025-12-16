#ifndef MODUL_PENUMPANG_H
#define MODUL_PENUMPANG_H

void penumpang_init();
void penumpang_create(const char* id, const char* nama, const char* email, const char* notelp,
    const char* tgl_lahir, const char* jk);
void penumpang_delete(int index);

#endif //MODUL_PENUMPANG_H