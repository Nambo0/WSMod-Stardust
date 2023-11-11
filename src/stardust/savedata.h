#pragma once

#include "../mkb/mkb.h"
#include <bitset>

namespace savedata {

s32 init();

void save();

bool true_in_slot(u16 slot);
bool consecutive_true_from_slot(u16 slot, u16 count);
u16 stellar_best_run_total();
u8 best_stellar_rank();
void update_stellar_bunch_counts(u8* bunch_counts);
void write_bool_to_slot(u16 slot, bool value);

enum {
    CLEAR_BADGE_START=0,
    STUNT_BADGE_START=100,
    SWEEP_BADGE_START=200,
    STAGE_CHALLENGES_START=300,
    WIDESCREEN_MODE=380,
};

void load_default_save();

}// namespace savedata
