cmake_minimum_required(VERSION 4.0)
project(TiketKereta C)

set(CMAKE_C_STANDARD 23)

add_executable(TiketKereta
        main.c
        globals.c
        modul_karyawan.c
        ui_app.c
        modul_akun.c
        modul_stasiun.c
        modul_kereta.c
)
