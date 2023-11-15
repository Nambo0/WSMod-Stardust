#pragma once

namespace badge {
void set_display_badges_next_frame_true(int type);
bool detect_sweep();
void tick();
void on_goal();
}// namespace badge