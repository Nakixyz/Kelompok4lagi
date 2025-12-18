#ifndef MODUL_PENUMPANG_H
#define MODUL_PENUMPANG_H

void penumpang_init();
void penumpang_create(const char* id, const char* nama, const char* email, const char* notelp,
    const char* tgl_lahir, const char* jk);
    void penumpang_create_auto(char *out_id, size_t out_sz,
                          const char* nama, const char* email, const char* no_telp, const char* tgl_lahir,
                          const char* jk);
void penumpang_delete(int index);
void penumpang_update(int index, const char* nama, const char* email, const char* no_telp, const char* tgl_lahir,
                          const char* jk);

#endif //MODUL_PENUMPANG_H