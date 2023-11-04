#pragma once

#include "../mkb/mkb.h"
#include <bitset>

namespace savedata {

s32 init();

void save();

bool true_in_slot(u16 slot);
bool consecutive_true_from_slot(u16 slot, u16 count);
u8 best_stellar_rank();
void write_bool_to_slot(u16 slot, bool value);

enum {
    WIDESCREEN_MODE=380,
    BOOLEAN_2=381,
};

void load_default_save();

}// namespace savedata
