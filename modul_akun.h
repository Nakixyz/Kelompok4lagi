#ifndef MODUL_AKUN_H
#define MODUL_AKUN_H

#include "types.h"

void akun_init();

/* CRUD basic */
int  akun_create(const char* username, const char* password, Role role);
void akun_delete(int index);

/* Update */
void akun_change_password(int index, const char* new_pass);
int  akun_update_username(int index, const char* new_username);
int  akun_update_role(int index, Role new_role);

#endif
