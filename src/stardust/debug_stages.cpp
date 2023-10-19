#include "debug_stages.h"

#include "../internal/patch.h"
#include "../internal/pad.h"
#include "internal/tickable.h"
#include "../mkb/mkb.h"
#include "../stardust/validate.h"

namespace debug_stages {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-debug-stages",
        .description = "Debug Category Features",
        .enabled = true,
        .tick = tick,
        .on_goal = on_goal, ))

u8 button_b_frames = 0;


static void skip_countdown_sprite_tick(u8* status, mkb::Sprite *sprite) {
    if (sprite->g_counter > 0){
        sprite->alpha = 1;
    }
    sprite->g_counter -= 1;
    if (sprite->g_counter < 0){
        sprite->g_counter = 0;
        sprite->alpha = 0;
    }
    switch(button_b_frames){
        case 0: sprite->g_counter = 0; break;
        case 1 ... 60: mkb::strcpy(sprite->text, "SKIP STAGE? 3"); break;
        case 61 ... 120: mkb::strcpy(sprite->text, "SKIP STAGE? 2"); break;
        case 121 ... 180: mkb::strcpy(sprite->text, "SKIP STAGE? 1"); break;
    }
}

static void create_skip_countdown_sprite() {
  mkb::Sprite* sprite = mkb::create_sprite();
  if (sprite != (mkb::Sprite *)0x0) {
    sprite->pos.x = 320.0;
    sprite->pos.y = 400.0;
    sprite->font = mkb::FONT_JAP_24x24_2;
    sprite->alignment = mkb::ALIGN_CENTER;
    sprite->mult_color.red = 0xff;
    sprite->mult_color.green = 0x80;
    sprite->mult_color.blue = 0x00;
    sprite->alpha = 0.0;
    sprite->g_counter = 180;
    sprite->g_flags1 = 0x1000000;
    sprite->widescreen_translation_x = 0x140;
    sprite->tick_func = skip_countdown_sprite_tick;
  }
  return;
}

static void skip_stage(){
    // Cause a bonus finish
    mkb::mode_info.ball_mode |= 1 << 6;
    mkb::mode_info.stage_time_frames_remaining = 1;
    // Special case for Stellar W2 draft's frozen timer
    if(mkb::g_current_stage_id == 267) mkb::mode_info.stage_time_frames_remaining = -100;
}

void tick(){
    if(mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::curr_difficulty == mkb::DIFF_ADVANCED &&
    (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN)){
        if(pad::button_down(mkb::PAD_BUTTON_B)){
            if(button_b_frames < 180){
                button_b_frames++;
                if(button_b_frames == 1){
                    create_skip_countdown_sprite();
                }
                if(button_b_frames == 180){
                    skip_stage();
                }
            }
        }
        else button_b_frames = 0;

    } 
}

void on_goal() {
}

}// namespace debug_stages