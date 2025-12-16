 #ifndef MODUL_AKUN_H
#define MODUL_AKUN_H

#include "types.h"

void akun_init(); // Load file + Seed 50 data
int akun_create(const char* username, const char* password, Role role);
void akun_delete(int index);
void akun_change_password(int index, const char* new_pass);

#endif