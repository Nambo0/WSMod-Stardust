#pragma once

#include "../mkb/mkb.h"
#include <bitset>

namespace achievement_display {
void add_achievement_to_display_queue(u8 id);
void init();
void tick();
}// namespace achievement_display