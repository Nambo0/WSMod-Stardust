#include "achievement.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "mkb/mkb.h"
#include "../stardust/validate.h"
#include "../stardust/savedata.h"

namespace achievement {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-achievements",
        .description = "Achievements",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick, ))

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

static u16 DT_last_completion_speed = 0;// For 1-8 Double Time
static bool DT_back_to_back = false;    // For 1-8 Double Time
static bool flipped_yet = false;        // For 9-3 Flip Switches

void claim_achievement(int id) {
    // ID 1 = Slot 300, and so on
    u32 claimed_slot = 300 + id - 1;
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
    }
}

/* Very useful code to display the IG # of a specific anim ID we wanna test
for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
        if(mkb::stagedef->coli_header_list[i].anim_group_id == 3 &&
        mkb::itemgroups[i].anim_frame == 0){
            ball.banana_count = i;
            break;
        }
    }
*/

void tick() {

    // Detect stage challenges
    if (validate::is_currently_valid()) {
        switch (mkb::g_current_stage_id) {
            // DOUBLE TAKE | 1-8 Double Time  -  Clear the stunt goal at both spinning speeds on back to back attempts (ID: 1)
            case 4: {
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    mkb::mode_info.entered_goal_type == mkb::Red) {
                    if (mkb::itemgroups[3].playback_state == 0) {// IG #3 is one of the animated ones, playback state 0 is 1x speed
                        if (DT_last_completion_speed == 2) {
                            claim_achievement(1);
                        }
                        else {
                            DT_last_completion_speed = 1;
                            DT_back_to_back = true;
                        }
                    }
                    if (mkb::itemgroups[3].playback_state == 3) {// Playback state 3 is 2x speed
                        if (DT_last_completion_speed == 1) {
                            claim_achievement(1);
                        }
                        else {
                            DT_last_completion_speed = 2;
                            DT_back_to_back = true;
                        }
                    }
                }
                // Requires back to back finishes
                if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 1) {
                    if (DT_back_to_back) {
                        DT_back_to_back = false;
                    }
                    else {
                        DT_last_completion_speed = 0;
                    }
                }
                break;
            }
            // UP, UP, AND AWAY | 2-6 Liftoff  -  Soar higher than the highest cloud onstage (ID: 2)
            case 12: {
                if ((mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) &&
                    ball.pos.y >= 360) {
                    claim_achievement(2);
                }
                break;
            }
            // DEFUSED | 3-10 Detonation  -  Clear the blue goal without activating the bomb switch (ID: 3)
            case 17: {
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    mkb::mode_info.entered_goal_type == mkb::Blue &&
                    mkb::itemgroups[5].anim_frame == 0) {// IG #5 is one of the animated ones
                    claim_achievement(3);
                }
                break;
            }
            // I WANNA BE THE BACK GOAL | 4-9 Avoidance  -  Enter the blue goal from the back side (ID: 4)
            case 26: {
                if (mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT &&
                    validate::is_currently_valid() &&
                    mkb::mode_info.entered_goal_type == mkb::Blue &&
                    ball.vel.z > 0) {
                    claim_achievement(4);
                }
                break;
            }
            // BEHIND LOCKED DOORS | 5-6 Door Dash  -  Clear the blue goal without opening any doors (ID: 5)
            case 33: {
                // Door IG #s: 15, 17, 19
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    mkb::mode_info.entered_goal_type == mkb::Blue &&
                    mkb::itemgroups[15].anim_frame == 0 &&
                    mkb::itemgroups[17].anim_frame == 0 &&
                    mkb::itemgroups[19].anim_frame == 0) {// IG #5 is one of the animated ones
                    claim_achievement(5);
                }
                break;
            }
            // MONOCHROMATIC | 6-1 Recolor  -  Clear any goal without entering a color-changing portal (ID: 6)
            // ^^^ included in hardcode.cpp for wormhole-tracking reasons
            // TARGET MASTER | 7-10 Break the Targets  -  Break all 8 targets and finish with time bonus (150s) (ID: 7)
            case 48: {
                bool all_broken = true;
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    if (mkb::stagedef->coli_header_list[i].anim_group_id >= 1 &&
                        mkb::stagedef->coli_header_list[i].anim_group_id <= 10 &&
                        mkb::itemgroups[i].anim_frame == 0) {
                        all_broken = false;
                    }
                }
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    all_broken &&
                    mkb::mode_info.stage_time_frames_remaining >= 150 * 60) {
                    claim_achievement(7);
                }
                break;
            }
            // POTASSIUM ALLERGY | 8-4 Frequencies  -  Clear the stage without collecting any bananas (ID: 8)
            case 52: {
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    mkb::mode_info.bananas_remaining == 45) {
                    claim_achievement(8);
                }
                break;
            }
            // FLIP WIZARD | 9-3 Flip Switches  -  Clear the stage without flipping the switches once (ID: 9)
            case 61: {
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    if (mkb::stagedef->coli_header_list[i].anim_group_id >= 1 &&
                        mkb::stagedef->coli_header_list[i].anim_group_id <= 4 &&
                        mkb::itemgroups[i].playback_state == 0) {
                        flipped_yet = true;
                    }
                }
                if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 1) {
                    flipped_yet = false;
                }
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    !flipped_yet) {
                    claim_achievement(9);
                }
                break;
            }
            // STARSTRUCK | 10-10 Impact  -  Finish in the stunt goal after it shoots into the sky (ID: 10)
            case 350: {
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    validate::is_currently_valid() &&
                    mkb::mode_info.entered_goal_type == mkb::Red &&
                    mkb::mode_info.stage_time_frames_remaining <= 15180) {
                    claim_achievement(10);
                }
                break;
            }
        }
    }
}

// static patch::Tramp<decltype(&mkb::g_something_with_triangle_collision)> g_something_with_triangle_collision_tramp;

void init() {
    /*
    patch::hook_function(g_something_with_triangle_collision_tramp, mkb::g_something_with_triangle_collision, [](mkb::PhysicsBall *param_1, mkb::StagedefColiTri *param_2) {
        param_2->rot_from_xy.y = ball.banana_count;
        g_something_with_triangle_collision_tramp.dest(param_1, param_2);
      });
    */
}

}// namespace achievement