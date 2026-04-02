#include "debug_stages.h"

#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../mkb/mkb.h"
#include "../stardust/achievement.h"
#include "../stardust/validate.h"
#include "internal/tickable.h"

namespace debug_stages {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-debug-stages",
        .description = "Debug Category Features",
        .enabled = true,
        .tick = tick,))

void skip_stage() {
    // Cause a bonus finish
    mkb::mode_info.ball_mode |= 1 << 6;
    mkb::mode_info.stage_time_frames_remaining = 1;
    // Special case for Stellar W2 draft's frozen timer
    if (mkb::g_current_stage_id == 267) mkb::mode_info.stage_time_frames_remaining = -100;
    // Claim achievement if end of zone
    // 31) HEY GOOBZ PLAY DEBUG | Complete or skip through any of the debug sub-categories
    if (mkb::g_current_stage_id == 245     // Credit Card
        || mkb::g_current_stage_id == 73   // Candy Clog
        || mkb::g_current_stage_id == 71) {// Precession
        achievement::claim_achievement(31);
    }
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

/*// SILENT SUPERNOVA STUFF
// Displays "WORLD X-#" and "WORLD Y-#" on first spin-in
void loadin_world_sprintf(char * buffer, char * format, ...){
    if((mkb::mode_flags & mkb::MF_PLAYING_MASTER_NOEX_COURSE) != mkb::MF_NONE) {
        mkb::sprintf(buffer,"WORLD X]%d", mkb::mode_info.g_selected_world_stage_idx + 1);
    }
    else if((mkb::mode_flags & mkb::MF_PLAYING_MASTER_EX_COURSE) != mkb::MF_NONE) {
        mkb::sprintf(buffer,"WORLD Y]%d", mkb::mode_info.g_selected_world_stage_idx + 1);
    }
    else mkb::sprintf(buffer,format);
}

void init_main_game() {
    // World X & Y code
    patch::write_branch_bl(reinterpret_cast<void*>(0x8032BD48), reinterpret_cast<void*>(loadin_world_sprintf)); // Loadin
}*/

}// namespace debug_stages