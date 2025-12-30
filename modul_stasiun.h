#ifndef MODUL_STASIUN_H
#define MODUL_STASIUN_H

void stasiun_init();
void stasiun_create(const char* id,const char* kode,const char* nama, int mdpl, const char* kota,const char* alama);
void stasiun_update(const char* id,const char* kode,const char* nama, int mdpl, const char* kota,const char* alamat);
void stasiun_delete_by_id(const char* id);
void stasiun_restore_by_id(const char* id);
int stasiun_find_index_by_id_all(const char* id);
int stasiun_find_index_by_id(const char* id);

#endif