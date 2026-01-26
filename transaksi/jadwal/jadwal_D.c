#include <stdio.h>
#include "jadwal.h"
#include "jadwal_shared.h"

void jadwal_delete(int index) {
    if (index < 0 || index >= g_jadwalCount) return;
    g_jadwal[index].active = 0;
    jadwal_save();
}
