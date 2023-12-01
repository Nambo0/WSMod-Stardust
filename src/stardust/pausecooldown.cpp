#include "pausecooldown.h"
#include "../internal/pad.h"
#include "debug_stages.h"
#include "internal/patch.h"
#include "internal/tickable.h"
#include "mkb/mkb.h"
#include "patches/extensions/death_counter.h"

namespace pausecooldown {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-pause-cooldown",
        .description = "Pause cooldown",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick))

static patch::Tramp<decltype(&mkb::sprite_pausemenu_disp)> s_sprite_pausemenu_disp_tramp;
static patch::Tramp<decltype(&mkb::pause_game)> s_pause_game_tramp;

bool is_retry;
int original_pausemenu_selection;

static void limit_pauses() {
    if (mkb::unlock_info.g_movies_watched != 0x0fff && mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {// Story mode NOT completed (Unlocks with Pracc Mod Unlock)
        mkb::g_repause_cooldown_counter = 60;
    }
}

void pausemenu_handler() {
    int ball_index;
    mkb::Ball* ball;
    if (mkb::main_game_mode == mkb::CHALLENGE_MODE) {
        if (mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
            if (mkb::sub_mode == mkb::SMD_GAME_TIMEOVER_INIT || mkb::sub_mode == mkb::SMD_GAME_TIMEOVER_MAIN ||
                mkb::sub_mode == mkb::SMD_GAME_GOAL_REPLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_GOAL_REPLAY_MAIN ||
                mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT || mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN ||
                mkb::sub_mode == mkb::SMD_GAME_RINGOUT_INIT || mkb::sub_mode == mkb::SMD_GAME_RINGOUT_MAIN) {
                mkb::pausemenu_entry_counts[1] = 3;
                mkb::g_current_pause_menu_entry_count = 3;
                mkb::pausemenu_type = 0;
            }
            else if (mkb::mode_info.stage_time_frames_remaining <= 15 * 60) {
                mkb::pausemenu_entry_counts[1] = 3;
                mkb::g_current_pause_menu_entry_count = 3;
                mkb::pausemenu_type = 0;
            }
            else if ((mkb::sub_mode == mkb::SMD_GAME_READY_MAIN || mkb::sub_mode == mkb::SMD_GAME_READY_INIT) && mkb::mode_info.attempt_count > 1) {
                mkb::pausemenu_entry_counts[1] = 3;
                mkb::g_current_pause_menu_entry_count = 3;
                mkb::pausemenu_type = 0;
            }
            else {
                if (mkb::g_current_focused_pause_menu_entry == 2 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 1;
                    original_pausemenu_selection = 2;
                }
                else if (mkb::g_current_focused_pause_menu_entry == 3 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 2;
                    original_pausemenu_selection = 3;
                }
                else if (mkb::g_current_focused_pause_menu_entry == 1 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 5;
                    mkb::create_fallout_or_bonus_finish_sprite(1);
                    death_counter::update_death_count();
                    is_retry = true;
                    mkb::mode_info.attempt_count = mkb::mode_info.attempt_count + 1;
                    ball = mkb::balls;
                    ball_index = mkb::curr_player_idx;
                    for (ball_index = 0; ball_index < 8; ball_index = ball_index + 1) {
                        if (ball->status == mkb::STAT_NORMAL) {
                            ball->phys_flags = ball->phys_flags;
                            ball->g_effect_flags = ball->g_effect_flags | 0x80;
                        }
                        ball = ball + 1;
                    }
                    mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
                    mkb::g_fade_track_volume(100, '\n');
                }
                if (mkb::g_some_pausemenu_var != 0xffffffff) {
                    mkb::g_current_focused_pause_menu_entry = original_pausemenu_selection;
                }
                patch::write_word(reinterpret_cast<void*>(0x8047f1dc), 0x8047ee80);
                patch::write_word(reinterpret_cast<void*>(0x8047f1f4), 0x8047eda4);
                patch::write_word(reinterpret_cast<void*>(0x8047f20c), 0x8047edf4);
                patch::write_word(reinterpret_cast<void*>(0x8047f4a8), 0x8047f1ac);
                patch::write_word(reinterpret_cast<void*>(0x80273cc8), 0x2c000005);
                mkb::pausemenu_entry_counts[1] = 5;
                mkb::g_current_pause_menu_entry_count = 5;
                mkb::pausemenu_type = 1;
            }
        }
        if (mkb::curr_difficulty == mkb::DIFF_ADVANCED) {
            if (mkb::sub_mode == mkb::SMD_GAME_GOAL_INIT || mkb::sub_mode == mkb::SMD_GAME_GOAL_MAIN ||
                mkb::sub_mode == mkb::SMD_GAME_GOAL_REPLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_GOAL_REPLAY_MAIN ||
                mkb::sub_mode == mkb::SMD_GAME_RINGOUT_INIT || mkb::sub_mode == mkb::SMD_GAME_RINGOUT_MAIN ||
                mkb::sub_mode == mkb::SMD_GAME_TIMEOVER_INIT || mkb::sub_mode == mkb::SMD_GAME_TIMEOVER_MAIN) {
                mkb::pausemenu_entry_counts[1] = 4;
                mkb::g_current_pause_menu_entry_count = 4;
                patch::write_word(reinterpret_cast<void*>(0x8047f4a8), 0x8047eed4);
                patch::write_word(reinterpret_cast<void*>(0x80273cc8), 0x2c000004);
            }
            else if (mkb::sub_mode == mkb::SMD_GAME_READY_INIT || mkb::sub_mode == mkb::SMD_GAME_READY_MAIN) {
                if (mkb::g_current_focused_pause_menu_entry == 2 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 1;
                    original_pausemenu_selection = 2;
                }
                else if (mkb::g_current_focused_pause_menu_entry == 3 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 2;
                    original_pausemenu_selection = 3;
                }
                else if (mkb::g_current_focused_pause_menu_entry == 1 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 5;
                    mkb::create_fallout_or_bonus_finish_sprite(1);
                    is_retry = true;
                    mkb::mode_info.attempt_count = mkb::mode_info.attempt_count + 1;
                    ball = mkb::balls;
                    ball_index = mkb::curr_player_idx;
                    for (ball_index = 0; ball_index < 8; ball_index = ball_index + 1) {
                        if (ball->status == mkb::STAT_NORMAL) {
                            ball->phys_flags = ball->phys_flags;
                            ball->g_effect_flags = ball->g_effect_flags | 0x80;
                        }
                        ball = ball + 1;
                    }
                    mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
                    mkb::g_fade_track_volume(100, '\n');
                }
                if (mkb::g_some_pausemenu_var != 0xffffffff) {
                    mkb::g_current_focused_pause_menu_entry = original_pausemenu_selection;
                }
                mkb::pausemenu_entry_counts[1] = 5;
                mkb::g_current_pause_menu_entry_count = 5;
                patch::write_word(reinterpret_cast<void*>(0x8047f1dc), 0x8047ee80);
                patch::write_word(reinterpret_cast<void*>(0x8047f1f4), 0x8047eda4);
                patch::write_word(reinterpret_cast<void*>(0x8047f20c), 0x8047edf4);
                patch::write_word(reinterpret_cast<void*>(0x8047f4a8), 0x8047f1ac);
                patch::write_word(reinterpret_cast<void*>(0x80273cc8), 0x2c000005);
            }
            else {
                if (mkb::g_current_focused_pause_menu_entry == 2 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 0;
                    original_pausemenu_selection = 2;
                    debug_stages::skip_stage();
                }
                else if (mkb::g_current_focused_pause_menu_entry == 3 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 1;
                    original_pausemenu_selection = 3;
                }
                else if (mkb::g_current_focused_pause_menu_entry == 4 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 2;
                    original_pausemenu_selection = 4;
                }
                else if (mkb::g_current_focused_pause_menu_entry == 1 && pad::button_pressed(mkb::PAD_BUTTON_A) && mkb::g_some_status_bitflag_maybe_pause_related != 00000000) {
                    mkb::g_current_focused_pause_menu_entry = 6;
                    mkb::create_fallout_or_bonus_finish_sprite(1);
                    is_retry = true;
                    mkb::mode_info.attempt_count = mkb::mode_info.attempt_count + 1;
                    death_counter::update_death_count();
                    ball = mkb::balls;
                    ball_index = mkb::curr_player_idx;
                    for (ball_index = 0; ball_index < 8; ball_index = ball_index + 1) {
                        if (ball->status == mkb::STAT_NORMAL) {
                            ball->phys_flags = ball->phys_flags;
                            ball->g_effect_flags = ball->g_effect_flags | 0x80;
                        }
                        ball = ball + 1;
                    }
                    mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
                    mkb::g_fade_track_volume(100, '\n');
                }
                if (mkb::g_some_pausemenu_var != 0xffffffff) {
                    mkb::g_current_focused_pause_menu_entry = original_pausemenu_selection;
                }
                patch::write_word(reinterpret_cast<void*>(0x8047f4a8), 0x8047f1ac);
                patch::write_word(reinterpret_cast<void*>(0x8047ed54), 0x536b6970);
                patch::write_word(reinterpret_cast<void*>(0x8047ed58), 0x20737461);
                patch::write_word(reinterpret_cast<void*>(0x8047ed5c), 0x67650000);
                patch::write_word(reinterpret_cast<void*>(0x8047f1dc), 0x8047ed54);
                patch::write_word(reinterpret_cast<void*>(0x8047f1f4), 0x8047ee80);
                patch::write_word(reinterpret_cast<void*>(0x8047f20c), 0x8047eda4);
                patch::write_word(reinterpret_cast<void*>(0x8047f224), 0x8047edf4);
                patch::write_word(reinterpret_cast<void*>(0x80273cc8), 0x2c000006);
                mkb::pausemenu_entry_counts[1] = 6;
                mkb::g_current_pause_menu_entry_count = 6;
            }
        }
        if (mkb::curr_difficulty == mkb::DIFF_EXPERT) {
            mkb::pausemenu_entry_counts[1] = 4;
            mkb::g_current_pause_menu_entry_count = 4;
            patch::write_word(reinterpret_cast<void*>(0x8047f4a8), 0x8047eed4);
            patch::write_word(reinterpret_cast<void*>(0x80273cc8), 0x2c000004);
        }
    }
}

void init() {
    patch::hook_function(s_sprite_pausemenu_disp_tramp, mkb::sprite_pausemenu_disp, [](mkb::Sprite* sprite) {
        s_sprite_pausemenu_disp_tramp.dest(sprite);
        limit_pauses();
    });
    patch::hook_function(s_pause_game_tramp, mkb::pause_game, []() {
        s_pause_game_tramp.dest();
        pausemenu_handler();
    });
}

void tick() {
    if ((mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN || mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT) && mkb::g_pause_status == 0) {
        is_retry = false;
    }
}

}// namespace pausecooldown