#include "auto_menu.h"

#include "internal/patch.h"
#include "internal/tickable.h"
#include "internal/pad.h"
#include "mkb/mkb.h"

namespace auto_menu {

u8 auto_mode = 1; // 0 = none, 1 = all, 2 = stunt only
bool trigger_retry = true;

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-auto_menu",
        .description = "Auto Menuing",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick,
        .on_goal = on_goal, ))

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

void force_stage_select(){
    mkb::mode_info.ball_mode = mkb::mode_info.ball_mode & ~mkb::BALLMODE_IN_REPLAY;
    mkb::main_mode_request = mkb::MD_GAME;
    mkb::sub_mode_request = mkb::SMD_GAME_SCENARIO_RETURN;
    if (mkb::stage_complete) {
        int goal_score;
        goal_score = mkb::get_goal_score((uint32_t *)0x0,(int *)0x0);
        mkb::set_storymode_score(mkb::balls[mkb::curr_player_idx].score + goal_score);
        mkb::set_storymode_bananas(mkb::balls[mkb::curr_player_idx].banana_count);
    }
}

void tick() {
    // Auto Retry
    if((mkb::sub_mode == mkb::SMD_GAME_READY_MAIN) && (auto_mode > 0) && trigger_retry){
        mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
        trigger_retry = false;
    }

    // Dpad Down to switch modes
    if (pad::button_pressed(mkb::PAD_BUTTON_DOWN)){
        auto_mode += 1;
        if(auto_mode > 2) auto_mode = 0;

        ball.banana_count = auto_mode;
    }
}

void init() {

}

void on_goal(){
    // Auto Stage Select
    trigger_retry = true;
    switch (auto_mode) {
        case 0:
            return;
        case 1:
            force_stage_select();
            return;
        case 2:
            if(mkb::mode_info.entered_goal_type == 2) force_stage_select();
        return;
    }
}

}// namespace auto_menu