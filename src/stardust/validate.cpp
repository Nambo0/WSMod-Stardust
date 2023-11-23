#include "validate.h"

#include "../internal/tickable.h"
#include "../mkb/mkb.h"

namespace validate {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-validate",
        .description = "Validate",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick, ))

bool currently_valid = false;
static s16 last_frame = 0;

bool is_currently_valid() {
    return currently_valid;
}

static void track_validity() {
    // Invalidate non-continuous timers (catches practice mod savestates & frozen timers)
    if ((mkb::mode_info.stage_time_frames_remaining > last_frame || mkb::mode_info.stage_time_frames_remaining < last_frame - 1)
    && mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
        currently_valid = false;
    }

    // Renew attempt at 59.98 (or extended timer equivalent)
    if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 1) {
        currently_valid = true;
    }

    last_frame = mkb::mode_info.stage_time_frames_remaining;
}

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

void init() {}

void tick() {
    track_validity();

    // For testing
    /*
    if (mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT || mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN){
        if(ball.banana_count == 0){
            if(currently_valid) ball.banana_count = 2;
            else ball.banana_count = 1;
        }
    }
    */
}

}// namespace validate