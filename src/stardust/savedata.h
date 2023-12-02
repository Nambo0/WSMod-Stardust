#pragma once

#include "../mkb/mkb.h"
#include <bitset>

namespace savedata {

static constexpr u32 CLEAR_BADGE_START = 0;
static constexpr u32 STUNT_BADGE_START = 100;
static constexpr u32 SWEEP_BADGE_START = 200;
static constexpr u32 STAGE_CHALLENGES_START = 300;
static constexpr u32 WIDESCREEN_MODE = 380;

void init();

void save();

bool true_in_slot(u16 slot);
bool consecutive_true_from_slot(u16 slot, u16 count);
bool consecutive_false_from_slot(u16 slot, u16 count);

u32 latest_played_world();

u16 stellar_best_run_total();
u8 best_stellar_rank();
u16 get_stellar_level(u8 level);
void update_stellar_bunch_counts(u8* bunch_counts);
void erase_all_data();
void debug_display_mode();
bool is_debug_display_mode();
void write_bool_to_slot(u16 slot, bool value);

void load_default_save();

}// namespace savedata
