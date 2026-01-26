#include <stdio.h>
#include "pembayaran.h"
#include "pembayaran_shared.h"

void pembayaran_delete(int index) {
    if (index < 0 || index >= g_pembayaranCount) return;
    g_pembayaran[index].active = 0;
    pembayaran_save();
}
