#include <stdio.h>
#include "kereta.h"
#include "kereta_shared.h"

void kereta_delete(int index) {
    if (index >= 0 && index < g_keretaCount) {
        /* soft delete: nonaktifkan record, status operasional tidak diubah */
        g_kereta[index].active = 0;
        kereta_save();
    }
}
