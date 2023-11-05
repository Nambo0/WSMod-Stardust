#pragma once

namespace achievement {
void init();
void tick();
void on_goal();
bool detect_stunt_pilot();
void claim_achievement(int);
}// namespace achievement