#ifndef MODUL_STASIUN_H
#define MODUL_STASIUN_H

void stasiun_init();
void stasiunCreate();
void stasiun_create(const char* id,const char* kode,const char* nama,const char* kota,const char* alama);
void stasiunRead();
void stasiun_read_by_id(const char* id);
void stasiunUpdate();
void stasiun_update(const char* id,const char* kode,const char* nama,const char* kota,const char* alamat);
void stasiunDelete();
void stasiun_delete_by_id(const char* id);
int stasiun_find_index_by_id(const char* id);

#endif