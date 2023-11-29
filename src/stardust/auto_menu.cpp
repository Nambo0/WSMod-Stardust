#include "auto_menu.h"

#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"

namespace auto_menu {

u8 auto_mode = 0;// 0 = none, 1 = all
bool trigger_retry = true;
u8 fade_frame = 0;

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-auto_menu",
        .description = "Auto Menuing",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick,
        .on_goal = on_goal, ))

void begin_stage_select_fade() {
    if (fade_frame == 0) fade_frame = 1;
    mkb::fade_screen_to_color(0x101, 0xffffff, 15);
}

void force_stage_select() {
    mkb::mode_info.ball_mode = mkb::mode_info.ball_mode & ~mkb::BALLMODE_IN_REPLAY;
    mkb::main_mode_request = mkb::MD_GAME;
    mkb::sub_mode_request = mkb::SMD_GAME_SCENARIO_RETURN;
    if (mkb::stage_complete) {
        int goal_score;
        goal_score = mkb::get_goal_score((uint32_t*) 0x0, (int*) 0x0);
        mkb::set_storymode_score(mkb::balls[mkb::curr_player_idx].score + goal_score);
        mkb::set_storymode_bananas(mkb::balls[mkb::curr_player_idx].banana_count);
    }
}

void tick() {
    if (mkb::g_auto_reload_setting == 0 && mkb::unlock_info.g_movies_watched == 0x0fff) {
        auto_mode = 1;
    }
    else {
        auto_mode = 0;
    }

    if (mkb::main_game_mode == mkb::STORY_MODE) {
        if (auto_mode == 0) patch::write_word(reinterpret_cast<void*>(0x808f4a3c), 0x2c000001);
        if (auto_mode > 0) patch::write_word(reinterpret_cast<void*>(0x808f4a3c), 0x2c00ff01);
        // Auto Stage Select
        if (fade_frame > 0) {
            if (fade_frame > 15) {
                force_stage_select();
                fade_frame = 0;
            }
            else fade_frame += 1;
        }
    }

    if (mkb::main_game_mode == mkb::STORY_MODE) {
        // Bufferable A press on story select screen
        if (pad::button_down(mkb::PAD_BUTTON_A) && mkb::g_storymode_stageselect_state == mkb::STAGE_SELECT_IDLE) {
            mkb::g_storymode_stageselect_state = 5;// 5 is unlabeled "STAGE_SELECTED_INIT"
            mkb::call_SoundReqID_arg_2(0x6e);      // Plays the menu sound
        }
    }
}

void init() {
}

void on_goal() {
    if (mkb::main_game_mode == mkb::STORY_MODE) {
        // Auto Stage Select
        trigger_retry = true;
        switch (auto_mode) {
            case 0:
                return;
            case 1:
                if(!pad::button_down(mkb::PAD_BUTTON_A)) begin_stage_select_fade();
                return;
        }
    }
}

}// namespace auto_menu