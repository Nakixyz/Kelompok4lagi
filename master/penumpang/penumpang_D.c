#include <stdio.h>
#include "penumpang.h"
#include "penumpang_shared.h"

void penumpang_delete(int index) {
    if (index >= 0 && index < g_penumpangCount) {
        g_penumpang[index].active = 0;
        penumpang_save();
    }
}
