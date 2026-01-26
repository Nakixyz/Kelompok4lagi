#include <stdio.h>
#include "stasiun.h"
#include "stasiun_shared.h"

void stasiun_delete(int index) {
    if (index >= 0 && index < g_stasiunCount) {
        g_stasiun[index].active = 0;
        stasiun_save();
    }
}
