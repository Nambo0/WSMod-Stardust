#include "auto_menu.h"

#include "internal/patch.h"
#include "internal/tickable.h"
#include "internal/pad.h"
#include "mkb/mkb.h"

namespace auto_menu {

u8 auto_mode = 1; // 0 = none, 1 = all, 2 = stunt only
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

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

void begin_stage_select_fade(){
    if(fade_frame == 0) fade_frame = 1;
    mkb::fade_screen_to_color(0x101,0xffffff,15);
}

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
    if (mkb::main_game_mode == mkb::STORY_MODE){
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

        // Auto Stage Select
        if(fade_frame > 0){
            if(fade_frame > 15){
                force_stage_select();
                fade_frame = 0;
            }
            else fade_frame += 1;
        }
    }
}

void init() {

}

void on_goal(){
    if (mkb::main_game_mode == mkb::STORY_MODE){
        // Auto Stage Select
        trigger_retry = true;
        switch (auto_mode) {
            case 0:
                return;
            case 1:
                begin_stage_select_fade();
                return;
            case 2:
                if(mkb::mode_info.entered_goal_type == 2) begin_stage_select_fade();
            return;
        }
    }
}

}// namespace auto_menu