#include "hardcode.h"

#include "internal/pad.h"
#include "internal/patch.h"
#include "internal/tickable.h"
#include "mkb/mkb.h"
#include "stardust/achievement.h"
#include "stardust/validate.h"

namespace hardcode {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-hardcode",
        .description = "Hardcoded features",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick, ))

static bool entered_wormhole = false;

static patch::Tramp<decltype(&mkb::teleport_through_wormhole)> s_teleport_through_wormhole_tramp;

void flip_hardcoded_wormholes(int idx) {
    switch (mkb::g_current_stage_id) {
        // Flip vertical wormhole exit direction
        // for 2-9 Pitfall, 8-2 Leaning Tower, and 10-8 Butterfly
        case 348:
        case 50: {
            mkb::balls[mkb::curr_player_idx].vel.x *= -1;
            mkb::balls[mkb::curr_player_idx].pos.x *= -1;
            mkb::cameras[mkb::curr_player_idx].rot.x *= -1;
            mkb::cameras[mkb::curr_player_idx].rot.y *= -1;
            break;
        }
        case 15: {
            auto ball_velocity = &mkb::balls[mkb::curr_player_idx].vel;
            auto camera_rotation = &mkb::cameras[mkb::curr_player_idx].rot;
            if (idx != 5 && idx != 13 && idx != 16 && idx != 17) {
                ball_velocity->x *= -1;
                camera_rotation->x -= camera_rotation->x * 2;
                camera_rotation->y -= camera_rotation->y * 2;
            }
            else {
                ball_velocity->z *= -1;
                // Rotate by 180deg to mirror across correct axis
                camera_rotation->x += 32768;
                camera_rotation->y += 32768;
                camera_rotation->x -= camera_rotation->x * 2;
                camera_rotation->y -= camera_rotation->y * 2;
            }
            break;
        }
        case 38: {// For 6-1 Recolor achievement
            entered_wormhole = true;
            break;
        }
    }
}

void init() {
    patch::hook_function(s_teleport_through_wormhole_tramp, mkb::teleport_through_wormhole, [](int ball_idx, int wormhole_idx) {
        mkb::undefined8 result = s_teleport_through_wormhole_tramp.dest(ball_idx, wormhole_idx);
        flip_hardcoded_wormholes(wormhole_idx + 1);
        return result;
    });
}

void tick() {
    if (pad::button_pressed(mkb::PAD_BUTTON_LEFT)) mkb::mode_info.stage_time_frames_remaining -= 600;
    if (pad::button_pressed(mkb::PAD_BUTTON_RIGHT)) mkb::mode_info.stage_time_frames_remaining += 600;

    if (pad::button_pressed(mkb::PAD_BUTTON_LEFT)) mkb::mode_info.stage_time_limit -= 600;
    if (pad::button_pressed(mkb::PAD_BUTTON_RIGHT)) mkb::mode_info.stage_time_limit += 600;


    switch (mkb::g_current_stage_id) {
        // 5-4 Spleef
        // Make the stunt goal activate when all 45 buttons are pressed
        case 31: {
            // Count number of activated green buttons
            u32 inactive_count = 0;
            for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
                if ((anim_id >= 1 && anim_id <= 45) &&
                    mkb::itemgroups[i].anim_frame < 1) {
                    inactive_count++;
                }
            }

            // If all 45 green buttons are activated, open stunt goal (id 69)
            if (inactive_count < 1) {
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    if (mkb::stagedef->coli_header_list[i].anim_group_id != 69) {
                        continue;
                    }
                    mkb::itemgroups[i].playback_state = 0;
                    break;
                }
            }
            break;
        }
        // 9-7 Drawbridges
        // Make the buttons only un-press when the animation reaches frame 0
        case 65: {
            // Make sure we're not messing with stuff before or after the stage starts/ends
            if (mkb::sub_mode != mkb::SMD_GAME_PLAY_INIT ||
                mkb::sub_mode != mkb::SMD_GAME_PLAY_MAIN) {
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    // Jam reversing buttons, repeat frames 3 -> 2
                    if (mkb::stagedef->coli_header_list[i].anim_group_id == 1 &&
                        mkb::itemgroups[i].playback_state == 2 &&
                        mkb::itemgroups[i].anim_frame == 2) {
                        mkb::itemgroups[i].anim_frame = 3;
                    }
                    // Jam playing buttons, repeat frames 58 -> 59
                    if (mkb::stagedef->coli_header_list[i].anim_group_id == 1 &&
                        mkb::itemgroups[i].playback_state == 0 &&
                        mkb::itemgroups[i].anim_frame == 59) {
                        mkb::itemgroups[i].anim_frame = 58;
                    }
                }
            }
            break;
        }
        // MONOCHROMATIC | 6-1 Recolor  -  Clear any goal without entering a color-changing portal (ID: 6)
        case 38: {
            if (mkb::mode_info.stage_time_frames_remaining == mkb::mode_info.stage_time_limit - 1) {
                entered_wormhole = false;
            }
            if ((mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT ||
                 mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN) &&
                validate::is_currently_valid() &&
                !entered_wormhole) {
                achievement::claim_achievement(6);
            }
            break;
        }
        // Interstellar 4 Deep Space
        // Hardcoded giant vertical wormhole
        case 224: {
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
                if(mkb::balls[mkb::curr_player_idx].pos.x > -500 &&
                   mkb::balls[mkb::curr_player_idx].pos.x < 500 &&
                   mkb::balls[mkb::curr_player_idx].pos.z > -500 &&
                   mkb::balls[mkb::curr_player_idx].pos.z < 500){
                    if(mkb::balls[mkb::curr_player_idx].pos.y < 0){
                        mkb::balls[mkb::curr_player_idx].pos.y += 650;
                    }
                    if(mkb::balls[mkb::curr_player_idx].pos.y > 650){
                        mkb::balls[mkb::curr_player_idx].pos.y -= 650;
                    }
                }
            }
            break;
        }
    }
}
}// namespace hardcode