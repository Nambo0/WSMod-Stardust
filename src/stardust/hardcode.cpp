#include "hardcode.h"

#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"
#include "../stardust/achievement.h"
#include "../stardust/badge.h"
#include "../stardust/savedata.h"
#include "../stardust/validate.h"
#include "../utils/vecutil.h"
#include "log.h"

namespace hardcode {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-hardcode",
        .description = "Hardcoded features",
        .enabled = true,
        .init_main_loop = init,
        .init_sel_ngc = init_sel_ngc,
        .tick = tick, ))

static bool entered_wormhole = false;
static u16 frame_to_remove_indicators = 0;

static patch::Tramp<decltype(&mkb::teleport_through_wormhole)> s_teleport_through_wormhole_tramp;
static patch::Tramp<decltype(&mkb::draw_sprite)> s_draw_sprite_tramp;    // Monuments: Hide HUD
static patch::Tramp<decltype(&mkb::g_draw_minimap)> s_draw_minimap_tramp;// Monuments: Hide HUD

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

void fix_stellar_bunch_indicators() {
    for (u32 ig = 0; ig < mkb::stagedef->coli_header_count; ig++) {
        if (mkb::stagedef->coli_header_list[ig].anim_group_id == 12001) {
            bool bunch_near = false;
            for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
                mkb::Item& item = mkb::items[i];                                                                     // shorthand: current item in the list = "item"
                if (mkb::item_pool_info.status_list[i] == 0) continue;                                               // skip if its inactive
                if (item.coin_type != 1) continue;                                                                   // skip if its not a bunch
                if (item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd) continue;// skip if banana is gone
                Vec diff = {mkb::stagedef->coli_header_list[ig].origin.x - item.g_position_copy_2.x,                 //
                            mkb::stagedef->coli_header_list[ig].origin.y - item.g_position_copy_2.y,
                            mkb::stagedef->coli_header_list[ig].origin.z - item.g_position_copy_2.z};
                if (diff.x * diff.x < 1      // Check if an indicator is in the same place
                    && diff.y * diff.y < 1   // as a bunch
                    && diff.z * diff.z < 1) {// (Squared to use absolute value)
                    bunch_near = true;
                }
            }
            if (!bunch_near) {
                mkb::itemgroups[ig].playback_state = 0;
                mkb::itemgroups[ig].anim_frame = 2;
            }
        }
    }
}

void init() {
    patch::hook_function(s_teleport_through_wormhole_tramp, mkb::teleport_through_wormhole, [](int ball_idx, int wormhole_idx) {
        mkb::undefined8 result = s_teleport_through_wormhole_tramp.dest(ball_idx, wormhole_idx);
        flip_hardcoded_wormholes(wormhole_idx + 1);
        return result;
    });
    patch::hook_function(s_draw_sprite_tramp, mkb::draw_sprite, [](mkb::Sprite* sprite) {
        // Hide every sprite except the pause menu
        bool correct_mode = mkb::main_mode == mkb::MD_GAME;
        bool is_pausemenu_sprite = sprite->disp_func == mkb::sprite_pausemenu_disp;
        if (!((mkb::g_current_stage_id == 77) && correct_mode && !is_pausemenu_sprite)) {
            s_draw_sprite_tramp.dest(sprite);
        }
    });
    patch::hook_function(s_draw_minimap_tramp, mkb::g_draw_minimap, []() {
        if (mkb::g_current_stage_id != 77) {
            s_draw_minimap_tramp.dest();
        }
    });
}

void tick() {
    /* DEV TIMER BINDS
    if (pad::button_pressed(mkb::PAD_BUTTON_LEFT) && mkb::mode_info.stage_time_frames_remaining > 600) mkb::mode_info.stage_time_frames_remaining -= 600;
    if (pad::button_pressed(mkb::PAD_BUTTON_RIGHT)) mkb::mode_info.stage_time_frames_remaining += 600;

    if (pad::button_pressed(mkb::PAD_BUTTON_LEFT) && mkb::mode_info.stage_time_limit > 600) mkb::mode_info.stage_time_limit -= 600;
    if (pad::button_pressed(mkb::PAD_BUTTON_RIGHT)) mkb::mode_info.stage_time_limit += 600;
    */

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
        // Interstellar 3 Solar Orbit
        // Animations retain from attempt to attempt
        /*
        case 223: {
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT && mkb::main_game_mode == mkb::CHALLENGE_MODE) {
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
                    if (anim_id == 11101) {
                        mkb::itemgroups[i].anim_frame = (300*60 - mkb::mode_info.stage_time_frames_remaining) % 100*60;
                        continue;
                    }
                    if (anim_id == 11003) {
                        mkb::itemgroups[i].anim_frame = (300*60 - mkb::mode_info.stage_time_frames_remaining) % 50*60;
                        continue;
                    }
                }
            }
            break;
        }
        */
        // Interstellar 4 Deep Space
        // Giant vertical wormhole
        case 224: {
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
                if (mkb::balls[mkb::curr_player_idx].pos.x > -500 &&
                    mkb::balls[mkb::curr_player_idx].pos.x < 500 &&
                    mkb::balls[mkb::curr_player_idx].pos.z > -500 &&
                    mkb::balls[mkb::curr_player_idx].pos.z < 500) {
                    if (mkb::balls[mkb::curr_player_idx].pos.y < 0) {
                        mkb::balls[mkb::curr_player_idx].pos.y += 650;
                        mkb::call_SoundReqID_arg_0(0x317);// Wormhole sound
                    }
                    if (mkb::balls[mkb::curr_player_idx].pos.y > 650) {
                        mkb::balls[mkb::curr_player_idx].pos.y -= 650;
                        mkb::call_SoundReqID_arg_0(0x317);// Wormhole sound
                    }
                }
            }
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT) {
                fix_stellar_bunch_indicators();
            }
            break;
        }
        // Interstellar 5 Singularity
        // Typewriter buttons
        case 225: {
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
                // Count number of activated purple/green buttons
                u8 purple_inactive_count = 0;
                u8 green_inactive_count = 0;
                u8 indicator_inactive_count = 0;
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
                    if ((anim_id >= 11 && anim_id <= 13) &&
                        mkb::itemgroups[i].anim_frame < 1) {
                        purple_inactive_count++;
                    }
                    if ((anim_id >= 14 && anim_id <= 16) &&
                        mkb::itemgroups[i].anim_frame < 1) {
                        green_inactive_count++;
                    }
                    if ((anim_id == 12001) &&
                        mkb::itemgroups[i].anim_frame < 1) {
                        indicator_inactive_count++;
                    }
                }

                // If all 3 purple buttons are activated, activate 'bunch' animation (id 11005)
                if (purple_inactive_count < 1 && indicator_inactive_count < 1) {
                    for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                        switch (mkb::stagedef->coli_header_list[i].anim_group_id) {
                            case 11003:
                                mkb::itemgroups[i].playback_state = 0;
                                break;
                            case 12001:
                                if (mkb::itemgroups[i].anim_frame > 1) mkb::itemgroups[i].playback_state = 2;// Reverse bunch indicator anims
                                break;
                        }
                    }
                }
                // If all 3 green buttons are activated, activate 'bounce' animation (id 20)
                if (green_inactive_count < 1) {
                    for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                        if (mkb::stagedef->coli_header_list[i].anim_group_id != 20) {
                            continue;
                        }
                        mkb::itemgroups[i].playback_state = 0;
                    }
                }

                // Fix bunch indicators on challenge retry
                // Do this on the 30th frame after retrying so their animations can align them!
                if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT) frame_to_remove_indicators = mkb::mode_info.stage_time_frames_remaining - 32;
                if (mkb::mode_info.stage_time_frames_remaining < frame_to_remove_indicators) {
                    fix_stellar_bunch_indicators();
                }
            }
            break;
        }
        // Interstellar 6 Glitch Anomaly
        // Giant flipper wormhole
        case 226: {
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
                if (((mkb::balls[mkb::curr_player_idx].pos.x > -270 - 600 &&
                      mkb::balls[mkb::curr_player_idx].pos.x < 270 - 600) ||
                     (mkb::balls[mkb::curr_player_idx].pos.x > -270 + 600 &&
                      mkb::balls[mkb::curr_player_idx].pos.x < 270 + 600)) &&
                    mkb::balls[mkb::curr_player_idx].pos.z > -270 &&
                    mkb::balls[mkb::curr_player_idx].pos.z < 270) {
                    if (mkb::balls[mkb::curr_player_idx].pos.y < 0) {
                        // Flip ball
                        mkb::balls[mkb::curr_player_idx].pos.y = 0;
                        mkb::balls[mkb::curr_player_idx].pos.x *= -1;
                        mkb::balls[mkb::curr_player_idx].vel.y *= -1;
                        mkb::balls[mkb::curr_player_idx].vel.y += 0.1;// Add some velocity to prevent getting stuck
                        mkb::balls[mkb::curr_player_idx].vel.x *= -1;
                        // Flip camera
                        auto camera_rotation = &mkb::cameras[mkb::curr_player_idx].rot;
                        camera_rotation->x -= camera_rotation->x * 2;
                        camera_rotation->y -= camera_rotation->y * 2;
                        camera_rotation->z += 32768;// 180deg rotation

                        mkb::call_SoundReqID_arg_0(0x317);// Wormhole sound
                    }
                }
            }
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT) {
                fix_stellar_bunch_indicators();
            }
            break;
            // Fix bunch indicators on challenge retry
            if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT) {
                for (u32 ig = 0; ig < mkb::stagedef->coli_header_count; ig++) {
                    if (mkb::stagedef->coli_header_list[ig].anim_group_id == 12001) {
                        bool bunch_near = false;
                        for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
                            mkb::Item& item = mkb::items[i];                                                                     // shorthand: current item in the list = "item"
                            if (mkb::item_pool_info.status_list[i] == 0) continue;                                               // skip if its inactive
                            if (item.coin_type != 1) continue;                                                                   // skip if its not a bunch
                            if (item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd) continue;// skip if banana is gone
                            Vec diff = {mkb::stagedef->coli_header_list[ig].origin.x - item.g_position_copy_2.x,                 //
                                        mkb::stagedef->coli_header_list[ig].origin.y - item.g_position_copy_2.y,
                                        mkb::stagedef->coli_header_list[ig].origin.z - item.g_position_copy_2.z};
                            if (diff.x * diff.x < 1      // Check if an indicator is in the same place
                                && diff.y * diff.y < 1   // as a bunch
                                && diff.z * diff.z < 1) {// (Squared to use absolute value)
                                bunch_near = true;
                            }
                        }
                        if (!bunch_near) {
                            mkb::itemgroups[ig].playback_state = 0;
                            mkb::itemgroups[ig].anim_frame = 2;
                        }
                    }
                }
            }
        }
        // Monuments
        // Show Galactic Log progress
        case 77: {
            if (mkb::sub_mode == mkb::SMD_GAME_READY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
                for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                    u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
                    switch (anim_id) {
                        // Claimed badges
                        case 0 ... 309: {
                            if (savedata::true_in_slot(anim_id)) {
                                mkb::itemgroups[i].playback_state = 0;
                                mkb::itemgroups[i].anim_frame = 2;
                            }
                            break;
                        }

                        // Interstellar Rank Medallion
                        // These positions are variable so they all need separate cases
                        case 400:
                            if (savedata::best_stellar_rank() == 1) mkb::itemgroups[i].playback_state = 0;
                            break;
                        case 401:
                            if (savedata::best_stellar_rank() == 2) mkb::itemgroups[i].playback_state = 0;
                            break;
                        case 402:
                            if (savedata::best_stellar_rank() == 3) mkb::itemgroups[i].playback_state = 0;
                            break;
                        case 403:
                            if (savedata::best_stellar_rank() == 4) mkb::itemgroups[i].playback_state = 0;
                            break;
                        case 404:
                            if (savedata::best_stellar_rank() == 5) mkb::itemgroups[i].playback_state = 0;
                            break;

                        // World sweep statue decorations
                        case 500 ... 519: {
                            if (savedata::consecutive_true_from_slot((anim_id - 500) * 10 + 100, 10)) {
                                mkb::itemgroups[i].playback_state = 0;
                                mkb::itemgroups[i].anim_frame = 2;
                            }
                            break;
                        }

                        // Story sweep statues
                        case 600:
                        case 601: {
                            if (savedata::consecutive_true_from_slot((anim_id - 600) * 100 + 100, 100)) {
                                mkb::itemgroups[i].playback_state = 0;
                                mkb::itemgroups[i].anim_frame = 2;
                            }
                            break;
                        }
                    }
                    if (anim_id > 399 && anim_id < 405 && mkb::itemgroups[i].anim_frame > 1198) {
                        mkb::itemgroups[i].anim_frame = 2;
                    }
                }
            }
            break;
        }
        case 205: {
            if(pad::button_down(mkb::PAD_TRIGGER_R) && pad::button_down(mkb::PAD_BUTTON_DOWN) && pad::button_pressed(mkb::PAD_BUTTON_B)) {
                savedata::debug_display_mode();
            }
            break;
        }
    }

    // Special hardcode for Interstellar 6 size-3 wormholes
    if (mkb::current_stage_id == 226) {
        patch::write_word(reinterpret_cast<void*>(0x803de6f8), 0x41400000);// 12.0
        patch::write_word(reinterpret_cast<void*>(0x803e65b0), 0x40c00000);// 6.0
    }
    // Special hardcode for Interstellar 9 size-6 big wormholes
    else if (mkb::current_stage_id == 229 || mkb::current_stage_id == 230) {
        patch::write_word(reinterpret_cast<void*>(0x803de6f8), 0x41c00000);// 24.0
        patch::write_word(reinterpret_cast<void*>(0x803e65b0), 0x41400000);// 12.0
    }
    else {
        patch::write_word(reinterpret_cast<void*>(0x803de6f8), 0x40800000);// 4.0 (default)
        patch::write_word(reinterpret_cast<void*>(0x803e65b0), 0x40000000);// 2.0 (default)
    }
    // Special hardcode for W4 & W6 snow particle rates
    patch::write_nop(reinterpret_cast<void*>(0x802f4ef4));
    if (mkb::world_theme == 0x00120000) patch::write_word(reinterpret_cast<void*>(0x802f4eac), 0xc07f001c);
    else patch::write_word(reinterpret_cast<void*>(0x802f4eac), 0xc07f0038);
    // Hurry up removal for frozen/count-up timers
    if (mkb::main_mode == mkb::MD_GAME &&
        ((mkb::main_game_mode == mkb::PRACTICE_MODE && (mkb::curr_difficulty == mkb::DIFF_BEGINNER)) || (mkb::current_stage_id == 267) || (mkb::current_stage_id == 77))) {
        patch::write_nop(reinterpret_cast<void*>(0x80339da0));
        patch::write_nop(reinterpret_cast<void*>(0x80339f14));
        patch::write_word(reinterpret_cast<void*>(0x808f5108), 0x2c00ff01);
        patch::write_word(reinterpret_cast<void*>(0x808f50b4), 0x2c00ff01);
        patch::write_word(reinterpret_cast<void*>(0x808f4ff8), 0x2c00ff01);
        patch::write_nop(reinterpret_cast<void*>(0x808f5044));
        patch::write_nop(reinterpret_cast<void*>(0x808f508c));
        patch::write_nop(reinterpret_cast<void*>(0x808f50f8));
        patch::write_nop(reinterpret_cast<void*>(0x808f514c));
        patch::write_nop(reinterpret_cast<void*>(0x808f5004));
        // patch::write_nop(reinterpret_cast<void*>(0x80339d7c)); // 0.00
    }
    else {
        patch::write_word(reinterpret_cast<void*>(0x80339da0), 0x901d004c);
        patch::write_word(reinterpret_cast<void*>(0x80339f14), 0x2c000258);
        // patch::write_word(reinterpret_cast<void*>(0x80339d7c), 0x901d004c); // 0.00
    }
    // Fast retry and fixed camera for Monuments
    if (mkb::current_stage_id == 77) {
        patch::write_word(reinterpret_cast<void*>(0x802ba280), 0x38000000);
        patch::write_word(reinterpret_cast<void*>(0x802ba288), 0x38800000);
        patch::write_word(reinterpret_cast<void*>(0x802b8cdc), 0x38800020);
        if (mkb::in_practice_mode == false) {
            patch::write_word(reinterpret_cast<void*>(0x802973d8), 0x2c000000);
        }
        else {
            patch::write_word(reinterpret_cast<void*>(0x802973d8), 0x2c000002);
        }
        patch::write_word(reinterpret_cast<void*>(0x802ba304), 0x3880004c);
        patch::write_word(reinterpret_cast<void*>(0x8044b1f4), 0x0000004d);
        patch::write_word(reinterpret_cast<void*>(0x8044b200), 0x00000000);
        patch::write_word(reinterpret_cast<void*>(0x8044b204), 0x43e10000);
    }
    else {
        patch::write_word(reinterpret_cast<void*>(0x802ba280), 0x3800001e);
        patch::write_word(reinterpret_cast<void*>(0x802ba288), 0x3880001e);
        patch::write_word(reinterpret_cast<void*>(0x802b8cdc), 0x38800004);
        patch::write_word(reinterpret_cast<void*>(0x802973d8), 0x2c000001);
        patch::write_word(reinterpret_cast<void*>(0x802ba304), 0x38800000);
        patch::write_word(reinterpret_cast<void*>(0x8044b1f4), 0x00000117);
        patch::write_word(reinterpret_cast<void*>(0x8044b200), 0xc2480000);
        patch::write_word(reinterpret_cast<void*>(0x8044b204), 0x42c80000);
    }
    if ((mkb::sub_mode == mkb::SMD_SEL_NGC_MAIN) && (mkb::g_currently_visible_menu_screen == 4)) {
        mkb::locked_menu_items = 0xe;
    }
    if (mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
        mkb::strcpy(mkb::LOADIN_TEXT_STAGE, "WORLD %d");
        mkb::strcpy(mkb::LOADIN_TEXT_FINAL_STAGE, "FINAL WORLD");
        patch::write_word(reinterpret_cast<void*>(0x8032cf28), 0x380000ff);
        patch::write_word(reinterpret_cast<void*>(0x8032cf30), 0x380000ff);
    }
    else if (mkb::curr_difficulty == mkb::DIFF_ADVANCED) {
        mkb::strcpy(mkb::LOADIN_TEXT_STAGE, "DEBUG %d");
        mkb::strcpy(mkb::LOADIN_TEXT_FINAL_STAGE, "END OF ZONE");
        if (mkb::current_stage_id == 245) {
            patch::write_word(reinterpret_cast<void*>(0x8032cf50), 0x38000000);
            patch::write_word(reinterpret_cast<void*>(0x8032cf58), 0x380000ff);
            patch::write_word(reinterpret_cast<void*>(0x8032cf60), 0x38000000);
        }
        else if (mkb::current_stage_id == 73) {
            patch::write_word(reinterpret_cast<void*>(0x8032cf50), 0x38000000);
            patch::write_word(reinterpret_cast<void*>(0x8032cf58), 0x38000000);
            patch::write_word(reinterpret_cast<void*>(0x8032cf60), 0x380000ff);
        }
        else {
            patch::write_word(reinterpret_cast<void*>(0x8032cf50), 0x380000ff);
            patch::write_word(reinterpret_cast<void*>(0x8032cf58), 0x38000000);
            patch::write_word(reinterpret_cast<void*>(0x8032cf60), 0x380000ff);
        }
    }
    else {
        mkb::strcpy(mkb::LOADIN_TEXT_STAGE, "STAGE %d");
        mkb::strcpy(mkb::LOADIN_TEXT_FINAL_STAGE, "");
    }
    if (mkb::main_game_mode != mkb::CHALLENGE_MODE) {
        mkb::strcpy(mkb::LOADIN_TEXT_FINAL_STAGE, "");
    }
    mkb::strcpy(mkb::LOADIN_TEXT_MASTER, "WORLD X]%d");
    patch::write_nop(reinterpret_cast<void*>(0x8032bba0));
    patch::write_nop(reinterpret_cast<void*>(0x8032bba8));
    patch::write_nop(reinterpret_cast<void*>(0x8032bbb0));

}// void tick
void init_sel_ngc() {
    mkb::strcpy(mkb::NUM_OF_PLAYERS_DESCRIPTION_NO_PLAYPOINTS, "You can play with 1 player.");
    mkb::strcpy(mkb::NUM_OF_PLAYERS_DESCRIPTION_PLAYPOINTS, "You can play with 1 player.");
    mkb::strcpy(mkb::NUM_OF_PLAYERS_DESCRIPTION, "You can play with 1 player.");
}
}// namespace hardcode