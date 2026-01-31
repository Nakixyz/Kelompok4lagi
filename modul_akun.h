#ifndef MODUL_AKUN_H
#define MODUL_AKUN_H

#include "types.h"

void akun_init(void);

/* CRUD basic */
int  akun_create(const char* email, const char* nama, const char* password, Role role);
void akun_delete(int index);

/* Update */
void akun_change_password(int index, const char* new_pass);
int  akun_update_email(int index, const char* new_email);
int  akun_update_nama(int index, const char* new_nama);
int  akun_update_role(int index, Role new_role);

/* Helper: normalisasi email login (tambahkan @kai.id jika tidak ada '@') */
int  akun_normalize_login_email(const char *input, char *out, int out_sz);

#endif
