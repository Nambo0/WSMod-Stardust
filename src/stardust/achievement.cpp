#include "achievement.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../internal/utils.h"
#include "../stardust/achievement_display.h"
#include "../stardust/badge.h"
#include "../stardust/savedata.h"
#include "../stardust/validate.h"
#include "mkb/mkb.h"

namespace achievement {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-achievements",
        .description = "Achievements",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick,
        .on_goal = on_goal, ))

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

static u16 DT_last_completion_speed = 0;// For 1-8 Double Time
static bool DT_back_to_back = false;    // For 1-8 Double Time
static bool flipped_yet = false;        // For 9-3 Flip Switches
static u8 last_stellar_goal = 0;        // For Finish Him
static u16 last_completed_stage_id = 0; // For You-Da-Bacon
static u8 completions_in_a_row = 0;     // For You-Da-Bacon
static bool last_attempt_won = false;   // For You-Da-Bacon
static bool went_very_fast = false;     // For AAAAA

void claim_achievement(int id) {
    // ID 1 = Slot 300, and so on
    u32 claimed_slot = savedata::STAGE_CHALLENGES_START + id - 1;
    if (!savedata::true_in_slot(claimed_slot)) {
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
        achievement_display::add_achievement_to_display_queue(id);
    }
}

bool detect_beat_the_game() {
    if (mkb::unlock_info.g_movies_watched == 0x0fff) return true;
    if (mkb::mode_info.g_selected_world_idx == 9 && mkb::g_amount_of_beaten_stages_in_world == 9) return true;
    else return false;
}
bool detect_stunt_trainee() {
    for (u8 i = 0; i < 100; i++) {
        if (savedata::true_in_slot(savedata::STUNT_BADGE_START + i)) return true;
    }
    return false;
}
bool detect_stunt_pilot() {
    for (u8 world = 0; world < 10; world++) {
        bool world_has_stunt_badge = false;
        for (u8 stage = 0; stage < 10; stage++) {
            if (savedata::true_in_slot(savedata::STUNT_BADGE_START + 10 * world + stage)) {
                world_has_stunt_badge = true;
                continue;
            }
        }
        if (!world_has_stunt_badge) return false;
    }
    return true;
}
bool detect_stunt_specialist() {
    for (u8 world = 0; world < 10; world++) {
        bool world_stunt_badges_full = true;
        for (u8 stage = 0; stage < 10; stage++) {
            if (!savedata::true_in_slot(savedata::STUNT_BADGE_START + 10 * world + stage)) {
                world_stunt_badges_full = false;
                continue;
            }
        }
        if (world_stunt_badges_full) return true;
    }
    return false;
}
bool detect_stunt_ace() {
    for (u8 i = 0; i < 100; i++) {
        if (!savedata::true_in_slot(savedata::STUNT_BADGE_START + i)) return false;
    }
    return true;
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
                // Requires back to back finishes
                if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 2) {
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
            // FLIP WIZARD | 9-3 Flip Switches  -  Clear the stage without flipping the switches once (ID: 9)
            case 61: {
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    if (mkb::stagedef->coli_header_list[i].anim_group_id >= 1 &&
                        mkb::stagedef->coli_header_list[i].anim_group_id <= 4 &&
                        mkb::itemgroups[i].playback_state == 0) {
                        flipped_yet = true;
                    }
                }
                if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 2) {
                    flipped_yet = false;
                }
                break;
            }
        }
        // 38) AAAAA | Clear a stage after traveling over 1,000 mph
        if (mkb::math_sqrt(VEC_LEN_SQ(ball.vel) * (134.2198 * 134.2198)) > 999.0) went_very_fast = true;
        if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 2) went_very_fast = false;

        // 33) YOU-DA-BACON | Clear a stage 10x in a row
        if(mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 2) {
            if(last_attempt_won) last_attempt_won = false;
            else completions_in_a_row = 0;
        }
    }// if currently valid

    // Reset last stellar goal when the mode starts
    if (mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::g_current_stage_id == 221 && mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT) {
        last_stellar_goal = 0;
    }
}

void on_goal() {
    // Stage challenge achievements
    if (validate::is_currently_valid()) {
        switch (mkb::g_current_stage_id) {
            // DOUBLE TAKE | 1-8 Double Time  -  Clear the stunt goal at both spinning speeds on back to back attempts (ID: 1)
            case 4: {
                if (mkb::mode_info.entered_goal_type == mkb::Red) {
                    if (mkb::itemgroups[8].playback_state == 0) {// IG #3 is one of the animated ones, playback state 0 is 1x speed
                        if (DT_last_completion_speed == 2) {
                            claim_achievement(1);
                        }
                        else {
                            DT_last_completion_speed = 1;
                            DT_back_to_back = true;
                        }
                    }
                    if (mkb::itemgroups[8].playback_state == 3) {// Playback state 3 is 2x speed
                        if (DT_last_completion_speed == 1) {
                            claim_achievement(1);
                        }
                        else {
                            DT_last_completion_speed = 2;
                            DT_back_to_back = true;
                        }
                    }
                }
                break;
            }
            // UP, UP, AND AWAY | 2-6 Liftoff  -  Soar higher than the highest cloud onstage (ID: 2)
            // ^^^ is included in the tick function, since goal is not required
            // DEFUSED | 3-10 Detonation  -  Clear the blue goal without activating the bomb switch (ID: 3)
            case 17: {
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                     mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    mkb::mode_info.entered_goal_type == mkb::Blue &&
                    mkb::itemgroups[5].anim_frame == 0) {// IG #5 is one of the animated ones
                    claim_achievement(3);
                }
                break;
            }
            // I WANNA BE THE BACK GOAL | 4-9 Avoidance  -  Enter the blue goal from the back side (ID: 4)
            case 26: {
                if (mkb::mode_info.entered_goal_type == mkb::Blue &&
                    ball.vel.z > 0) {
                    claim_achievement(4);
                }
                break;
            }
            // BEHIND LOCKED DOORS | 5-6 Door Dash  -  Clear the blue goal without opening any doors (ID: 5)
            case 33: {
                // Door IG #s: 15, 17, 19
                if (mkb::mode_info.entered_goal_type == mkb::Blue &&
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
                if (all_broken && mkb::mode_info.stage_time_frames_remaining >= 150 * 60) {
                    claim_achievement(7);
                }
                break;
            }
            // POTASSIUM ALLERGY | 8-4 Frequencies  -  Clear the stage without collecting any bananas (ID: 8)
            case 52: {
                if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT || mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                    mkb::mode_info.bananas_remaining == 45) {
                    claim_achievement(8);
                }
                break;
            }
            // FLIP WIZARD | 9-3 Flip Switches  -  Clear the stage without flipping the switches once (ID: 9)
            case 61: {
                if (!flipped_yet) {
                    claim_achievement(9);
                }
                break;
            }
            // STARSTRUCK | 10-10 Impact  -  Finish in the stunt goal after it shoots into the sky (ID: 10)
            case 350: {
                if (mkb::mode_info.entered_goal_type == mkb::Red &&
                    mkb::mode_info.stage_time_frames_remaining <= 15180) {// Star is in the sky
                    claim_achievement(10);
                }
                break;
            }
            // Secret achievements
            // 34) RULES LAWYER | Clear the stunt goal on Spleef without pressing any blue buttons             (on-goal + for loop)
            case 31: {
                bool none_pressed = true;
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    if (mkb::stagedef->coli_header_list[i].anim_group_id >= 1001 &&
                        mkb::stagedef->coli_header_list[i].anim_group_id <= 1008 &&
                        mkb::itemgroups[i].anim_frame == 30) {// Animation finished playing
                        none_pressed = false;
                    }
                }
                if (none_pressed) claim_achievement(34);
                break;
            }
            // 35) RULES LAWYER 2 | Clear the stunt goal on Currents without clicking the stunt goal button    (on-goal + for loop)
            case 44: {
                bool pressed = false;
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    if (mkb::stagedef->coli_header_list[i].anim_group_id == 1 &&
                        mkb::itemgroups[i].anim_frame == 420) {// Animation finished playing
                        pressed = true;
                    }
                }
                if (mkb::mode_info.entered_goal_type == mkb::Red && !pressed) claim_achievement(35);
                break;
            }
        }// Switch stage ID
        // 31) HEY GOOBZ PLAY DEBUG | Complete or skip through any of the debug sub-categories
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE && (mkb::g_current_stage_id == 245 ||// Credit Card
                                                           mkb::g_current_stage_id == 73 || // Candy Clog
                                                           mkb::g_current_stage_id == 71)) {// Precession
            claim_achievement(31);
        }
        // 32) A COMPLEX JOKE | Clear a stage with exactly 54.13 on the timer
        if (mkb::mode_info.stage_time_frames_remaining == 3248) claim_achievement(32);
        // 33) YOU-DA-BACON | Clear a stage 10x in a row
        if (last_completed_stage_id == mkb::g_current_stage_id) {
            completions_in_a_row++;
            last_attempt_won = true;
            if (completions_in_a_row == 10) claim_achievement(33);
        }
        else completions_in_a_row = 1;
        last_completed_stage_id = mkb::g_current_stage_id;
        // 36) ACUTALLY PLAYABLE | Clear a stage from ‘The Unplayable Zone’ in debug
        if (mkb::curr_difficulty == mkb::DIFF_ADVANCED && mkb::mode_info.cm_course_stage_num >= 90 && mkb::mode_info.cm_course_stage_num <= 110) {
            claim_achievement(36);
        }
        // 37 and all other interstellar achievements are in interstellar.cpp
        // 38) AAAAA | Clear a stage after traveling over 1,000 mph
        if (went_very_fast) claim_achievement(37);
    }// If valid

    // Badge-count achievements
    if (detect_beat_the_game()) claim_achievement(11);
    if (detect_stunt_trainee()) claim_achievement(12);
    if (detect_stunt_pilot()) claim_achievement(13);
    if (detect_stunt_specialist()) claim_achievement(14);
    if (detect_stunt_ace()) claim_achievement(15);


    // Banana-count achievements
    if (mkb::main_game_mode == mkb::STORY_MODE) {
        u16 banana_count = mkb::get_storymode_banana_count();
        // EATER OF SOULS | Obtain 5,000 bananas in Story Mode (ID: 17)
        if (banana_count >= 5000) {
            claim_achievement(16);
        }
        // EATER OF WORLDS | Obtain 9,999 bananas in Story Mode (ID: 18)
        if (banana_count >= 9999) {
            claim_achievement(17);
        }
    }

    // 26) FINISH HIM | Get all 10 interstellar goals in 1 run
    if (mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
        u8 stage = mkb::g_current_stage_id - 220;
        switch (stage) {
            case 1: {
                last_stellar_goal = 1;
                break;
            }
            case 2 ... 9: {
                if (last_stellar_goal == stage - 1) last_stellar_goal = stage;
                break;
            }
            case 10: {
                if (last_stellar_goal == stage - 1) claim_achievement(26);
                break;
            }
        }
    }

    // 27) EXTREME POTASSIUM OVERLOAD | (Practice Mode) Get all 100 bunches and finish on any stage
    if (badge::detect_sweep() && mkb::main_game_mode == mkb::PRACTICE_MODE && mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
        claim_achievement(27);
    }
}

void init() {
}

}// namespace achievement