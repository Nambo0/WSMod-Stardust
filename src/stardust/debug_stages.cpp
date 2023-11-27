#include "debug_stages.h"

#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../mkb/mkb.h"
#include "../stardust/validate.h"
#include "internal/tickable.h"

namespace debug_stages {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-debug-stages",
        .description = "Debug Category Features",
        .enabled = true,
        .tick = tick,
        .on_goal = on_goal, ))

void skip_stage() {
    // Cause a bonus finish
    mkb::mode_info.ball_mode |= 1 << 6;
    mkb::mode_info.stage_time_frames_remaining = 1;
    // Special case for Stellar W2 draft's frozen timer
    if (mkb::g_current_stage_id == 267) mkb::mode_info.stage_time_frames_remaining = -100;
}


void tick() {
    if (mkb::current_stage_id == 205) {
        if (mkb::mode_info.entered_goal_type == 1) {
            mkb::mode_info.cm_course_stage_num = 21;
            mkb::mode_info.cm_stage_id = 246;
        }
        if (mkb::mode_info.entered_goal_type == 2) {
            mkb::mode_info.cm_course_stage_num = 90;
            mkb::mode_info.cm_stage_id = 321;
        }
    }
}

void on_goal() {
}

}// namespace debug_stages