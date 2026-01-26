cmake_minimum_required(VERSION 4.0)
project(Kelompok4lagi C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(master)
include_directories(master/Karyawan)
include_directories(master/kereta)
include_directories(master/penumpang)
include_directories(master/stasiun)
include_directories(transaksi)
include_directories(transaksi/jadwal)
include_directories(transaksi/pembayaran)

add_executable(Kelompok4lagi
        master/Karyawan/karyawan.h
        master/Karyawan/karyawan_C.c
        master/Karyawan/karyawan_D.c
        master/Karyawan/karyawan_R.c
        master/Karyawan/karyawan_shared.h
        master/Karyawan/karyawan_U.c
        master/kereta/kereta.h
        master/kereta/kereta_C.c
        master/kereta/kereta_D.c
        master/kereta/kereta_R.c
        master/kereta/kereta_shared.h
        master/kereta/kereta_U.c
        master/penumpang/penumpang.h
        master/penumpang/penumpang_C.c
        master/penumpang/penumpang_D.c
        master/penumpang/penumpang_R.c
        master/penumpang/penumpang_shared.h
        master/penumpang/penumpang_U.c
        master/stasiun/stasiun.h
        master/stasiun/stasiun_C.c
        master/stasiun/stasiun_D.c
        master/stasiun/stasiun_R.c
        master/stasiun/stasiun_shared.h
        master/stasiun/stasiun_U.c
        transaksi/jadwal/jadwal.h
        transaksi/jadwal/jadwal_C.c
        transaksi/jadwal/jadwal_D.c
        transaksi/jadwal/jadwal_R.c
        transaksi/jadwal/jadwal_shared.h
        transaksi/jadwal/jadwal_U.c
        transaksi/pembayaran/pembayaran.h
        transaksi/pembayaran/pembayaran_C.c
        transaksi/pembayaran/pembayaran_D.c
        transaksi/pembayaran/pembayaran_R.c
        transaksi/pembayaran/pembayaran_shared.h
        globals.c
        globals.h
        main.c
        modul_akun.c
        modul_akun.h
        modul_laporan.c
        modul_laporan.h
        types.h
        ui_app.c
        ui_app.h)
