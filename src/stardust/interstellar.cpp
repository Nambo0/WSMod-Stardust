#include "interstellar.h"

#include "../internal/patch.h"
#include "../internal/pad.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"
#include "../stardust/badge.h"
#include "../stardust/savedata.h"


namespace interstellar {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-interstellar",
        .description = "Interstellar",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick,
        .on_goal = on_goal, ))

static u8 goal_bonus_effect = 0;
bool paused_now = *reinterpret_cast<u32*>(0x805BC474) & 8;
static u16 frames_left = 300 * 60;
static u32 bunches_gone[] = {0, 0, 0, 0};// Bitfield array for keeping track of which bunches on a stage were collected between attempts
static u32 anim_states = 0;              // Bitfield array for keeping track of which play-once anims were activated between attempts
// NOTE: Only tracks anim IDs 1-32, so I gotta remember to have all fallout-sustained switches be anim ID 1-32

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

// void create_fallout_or_bonus_finish_sprite(s32 param_1);
static patch::Tramp<decltype(&mkb::create_fallout_or_bonus_finish_sprite)> s_create_fallout_or_bonus_finish_sprite_tramp;
// void g_reset_ball(struct Ball *in_ball);
static patch::Tramp<decltype(&mkb::g_reset_ball)> s_g_reset_ball_tramp;
// void load_stagedef(u32 stage_id);
static patch::Tramp<decltype(&mkb::load_stagedef)> s_load_stagedef_tramp;

static bool stage_id_is_stellar(u32 stage_id) {
    switch (stage_id) {
        case 221 ... 230: {
            return true;
        }
        default: {
            return false;
        }
    }
}

static u8 bunches_collected_on_stage[10];

static u16 bunches_collected_total(){
    u16 return_value = 0;
    for(int i = 0; i < 10; i++){
        return_value += bunches_collected_on_stage[i];
    }
    return return_value;
}

static void save_finished_run_total(){
    bunches_collected_on_stage[9] = (mkb::balls[mkb::curr_player_idx].banana_count / 10) - bunches_collected_total();
    savedata::update_stellar_bunch_counts(bunches_collected_on_stage);
    savedata::save();
}

// REMOVE THIS NAME, AND MOVE THIS TO WHEREVER THE ENDSCREEN CODE ENDS UP
static void stuff_to_run_at_end_of_run(){
    if(bunches_collected_total() > savedata::stellar_best_run_total()){
        save_finished_run_total();
        // Display "New Best!"
    }
}

static void spawn_banana_effect(){
    mkb::Effect effect;
    mkb::Ball ball = mkb::balls[mkb::curr_player_idx];
    mkb::Item& dummy_item = mkb::items[1]; 
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue; // skip if its inactive
        mkb::Item& item = mkb::items[i];                       // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue;                     // skip if its not a bunch
        dummy_item = item;
        break;
    }
    mkb::memset(&effect, 0, 0xb0);
    effect.type = mkb::EFFECT_HOLDING_BANANA;
    effect.g_ball_idx = (short) (char) mkb::current_ball->idx;
    effect.g_pos = ball.pos;
    effect.g_some_vec = ball.vel;
    effect.g_some_rot = dummy_item.rotation;
    effect.g_pointer_to_some_struct = (s32) mkb::g_something_with_coins(
        (int**) dummy_item.g_something_with_gma_model);
    effect.g_scale.x =
        (1.0 / *(float*) (effect.g_pointer_to_some_struct + 0x14)) *
        1.5;
    effect.g_scale.y = effect.g_scale.x;
    effect.g_scale.z = effect.g_scale.x;
    spawn_effect(&effect);
    // Sounds
    mkb::call_SoundReqID_arg_0(0x39);
    if (((mkb::mode_info.ball_mode & mkb::BALLMODE_IN_TUTORIAL_SEQUENCE) !=
            mkb::BALLMODE_NONE) ||
        ((mkb::mode_info.ball_mode & mkb::BALLMODE_IN_REPLAY) ==
            mkb::BALLMODE_NONE)) {
        mkb::call_SoundReqID_arg_0(0x2820);
    }
}

void goal_bonus_sprite_tick(u8* status, mkb::Sprite *sprite) {
    if (sprite->g_counter > 0){
        sprite->alpha += 0.05;
        if (sprite->alpha > 1){
        sprite->alpha = 1;
    }
    }
    sprite->g_counter -= 1;
    if (sprite->g_counter < 0){
        sprite->g_counter = 0;
        sprite->alpha -= 0.05;
        if (sprite->alpha < 0){
        sprite->alpha = 0;
    }
    }
}

void create_goal_bonus_sprite() {
  mkb::Sprite* sprite = mkb::create_sprite();
  if (sprite != (mkb::Sprite *)0x0) {
    sprite->pos.x = 320.0;
    sprite->pos.y = 380.0;
    sprite->font = mkb::FONT_JAP_24x24_2;
    sprite->alignment = mkb::ALIGN_CENTER;
    sprite->mult_color.red = 0xff;
    sprite->mult_color.green = 0xff;
    sprite->mult_color.blue = 0x00;
    sprite->alpha = 0.0;
    sprite->g_counter = 300;
    sprite->g_flags1 = 0x1000000;
    sprite->widescreen_translation_x = 0x140;
    sprite->tick_func = goal_bonus_sprite_tick;
    mkb::strcpy(sprite->text, "BONUS  +50");
  }
  sprite = mkb::create_sprite();
   if (sprite != (mkb::Sprite *)0x0) {
    sprite->type = mkb::SPRT_BMP;
    sprite->pos.x = 340.0;
    sprite->pos.y = 380.0;
    sprite->alignment = mkb::ALIGN_CENTER;
    sprite->bmp = 0xc;
    sprite->alpha = 0.0;
    sprite->g_counter = 300;
    sprite->g_flags1 = 0x1000000;
    sprite->width = 0.3;
    sprite->height = 0.3;
    sprite->widescreen_translation_x = 0x140;
    sprite->tick_func = goal_bonus_sprite_tick;
    mkb::strcpy(sprite->text, "bonus banana");
  }
  return;
}

void remove_banana(mkb::Item& item) {
    item.g_some_flag = 0;
    item.g_some_bitfield = item.g_some_bitfield | 1;
    item.g_some_bitfield = item.g_some_bitfield & 0xfffffffd;
}

void create_penalty_sprite() {

  mkb::Sprite* sprite = mkb::create_sprite();
  if (sprite != (mkb::Sprite *)0x0) {
    sprite->pos.x = 320.0;
    sprite->pos.y = 380.0;
    sprite->font = mkb::FONT_JAP_24x24_2;
    sprite->alignment = mkb::ALIGN_CENTER;
    sprite->mult_color.red = 0xff;
    sprite->mult_color.green = 0x80;
    sprite->mult_color.blue = 0x00;
    sprite->alpha = 0.0;
    sprite->g_counter = 300;
    sprite->g_flags1 = 0x1000000;
    sprite->widescreen_translation_x = 0x140;
    sprite->tick_func = goal_bonus_sprite_tick;
    mkb::strcpy(sprite->text, "PENALTY  -15");
  }
  sprite = mkb::create_sprite();
   if (sprite != (mkb::Sprite *)0x0) {
    sprite->type = mkb::SPRT_BMP;
    sprite->pos.x = 367.0;
    sprite->pos.y = 380.0;
    sprite->alignment = mkb::ALIGN_CENTER;
    sprite->bmp = 0x510;
    sprite->alpha = 0.0;
    sprite->g_counter = 300;
    sprite->g_flags1 = 0x1000000;
    sprite->width = 0.75;
    sprite->height = 0.75;
    sprite->widescreen_translation_x = 0x140;
    sprite->tick_func = goal_bonus_sprite_tick;
    mkb::strcpy(sprite->text, "penalty time");
  }
  return;
}

void on_stage_load(u32 stage_id) {
    if (stage_id_is_stellar(stage_id)) {
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE) {
            // Set proper timer (300s)
            frames_left = 300 * 60;

            // Set all bunches to un-collected
            for (u32 i = 0; i < 4; i++) {
                bunches_gone[i] = 0;
            }

            // Reset logged stage bunch counts
            if(stage_id == 220){
                for(int i = 0; i < 10; i++){
                    bunches_collected_on_stage[i] = 0;
                }
            }
            // Count bunches on every load-in past #2
            else {
                bunches_collected_on_stage[stage_id - 222] = (mkb::balls[mkb::curr_player_idx].banana_count / 10) - bunches_collected_total();
            }
        }
    }

    // Handle frozen & increasing timers
    if(mkb::main_game_mode == mkb::PRACTICE_MODE && stage_id_is_stellar(stage_id)){
        // time over at -60 frames (so timer is able to stop at 0.00)
        *reinterpret_cast<u32*>(0x80297548) = 0x2c00ffa0;
        // Add 1 to the timer each frame (increasing)
        patch::write_word(reinterpret_cast<u32*>(0x80297534), 0x38030001);
    }
    else if(stage_id == 267 || stage_id == 77){
        // time over at -60 frames (so timer is able to stop at 0.00)
        *reinterpret_cast<u32*>(0x80297548) = 0x2c00ffa0;
        // add 0 to the timer each frame (frozen)
        patch::write_word(reinterpret_cast<u32*>(0x80297534), 0x38030000);
    }
    else {
        // time over at 0 frames
        *reinterpret_cast<u32*>(0x80297548) = 0x2c000000;
        // add -1 to timer each frame (counting down, normal)
        patch::write_word(reinterpret_cast<u32*>(0x80297534), 0x3803ffff);
    }
}

void tick() {
    if (stage_id_is_stellar(mkb::g_current_stage_id)) {
        if (mkb::main_game_mode == mkb::PRACTICE_MODE &&
            (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN)) {
            if(mkb::mode_info.stage_time_frames_remaining >= 540*60){
                // Loop timer to 0 at 540
                mkb::mode_info.stage_time_frames_remaining = 0;
            }
        }
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::mode_info.stage_time_frames_remaining <= 15 * 60) {
            mkb::mode_info.ball_mode |= 1 << 6;
        }
    }

    if(goal_bonus_effect > 0 && !paused_now){
        goal_bonus_effect++;
        if(goal_bonus_effect % 6 == 0){
            spawn_banana_effect();
        }
        if(goal_bonus_effect > 30){
            goal_bonus_effect = 0;
        }
    }
}

void on_goal() {
    if (stage_id_is_stellar(mkb::g_current_stage_id)) {
        // Goal Bonus
        ball.banana_count += 50;
        goal_bonus_effect = 1;
        create_goal_bonus_sprite();
    }
}

void on_fallout() {
    if (stage_id_is_stellar(mkb::g_current_stage_id)) {
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE) {
            // Fallout Timer Penalty
            if (mkb::mode_info.stage_time_frames_remaining > 15 * 60) {
                frames_left = mkb::mode_info.stage_time_frames_remaining - 15 * 60;
            }
            else {
                // Cause bonus finish if time is over
                mkb::mode_info.ball_mode |= 1 << 6;
            }
            create_penalty_sprite();

            // Save banana state
            for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
                if (mkb::item_pool_info.status_list[i] == 0) continue;                                       // skip if its inactive
                mkb::Item& item = mkb::items[i];                                                             // shorthand: current item in the list = "item"
                if (item.coin_type != 1) continue;                                                           // skip if its not a bunch
                if (item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd) {// True if banana is gone
                    u32 array_index = i / 32;                                                                // Determine the index of the array element containing the i'th bit
                    u32 bit_offset = i % 32;                                                                 // Determine the bit offset within the array element
                    bunches_gone[array_index] |= 1 << bit_offset;
                }
            }

            // Save play-once animation states
            anim_states = 0;
            for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                if (mkb::stagedef->coli_header_list[i].anim_group_id >= 1 && mkb::stagedef->coli_header_list[i].anim_group_id <= 32 && mkb::itemgroups[i].anim_frame > 1) {
                    anim_states |= 1 << mkb::stagedef->coli_header_list[i].anim_group_id;
                }
            }
        }
    }
}

void on_spin_in() {
    if (stage_id_is_stellar(mkb::g_current_stage_id)) {
        mkb::mode_info.stage_time_limit = 2;// Prevents replay-memory crashes
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE) {
            // Stage timer keeps ticking between attempts
            mkb::mode_info.stage_time_frames_remaining = frames_left;

            // Renew banana state from previous attempt
            for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
                if (mkb::item_pool_info.status_list[i] == 0) continue;// skip if its inactive
                mkb::Item& item = mkb::items[i];                      // shorthand: current item in the list = "item"
                if (item.coin_type != 1) continue;                    // skip if its not a bunch
                u32 array_index = i / 32;                             // Determine the index of the array element containing the i'th bit
                u32 bit_offset = i % 32;                              // Determine the bit offset within the array element
                if (bunches_gone[array_index] & (1 << bit_offset)) remove_banana(item);
            }

            // Renew play-once animation states from previous attempt
            for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                if (anim_states & (1 << mkb::stagedef->coli_header_list[i].anim_group_id)) {
                    mkb::itemgroups[i].playback_state = 0;
                }
            }
        }
        if (mkb::main_game_mode == mkb::PRACTICE_MODE) {
            frames_left = 0;
            mkb::mode_info.stage_time_frames_remaining = 0;
        }
    }
}

void init() {
    patch::hook_function(s_create_fallout_or_bonus_finish_sprite_tramp, mkb::create_fallout_or_bonus_finish_sprite, [](s32 param_1) {
        on_fallout();
        s_create_fallout_or_bonus_finish_sprite_tramp.dest(param_1);
    });
    patch::hook_function(s_g_reset_ball_tramp, mkb::g_reset_ball, [](mkb::Ball* in_ball) {
        s_g_reset_ball_tramp.dest(in_ball);
        on_spin_in();
    });
    patch::hook_function(s_load_stagedef_tramp, mkb::load_stagedef, [](u32 stage_id) {
        s_load_stagedef_tramp.dest(stage_id);
        on_stage_load(stage_id);
    });
}

}// namespace interstellar

// TODO: End screen (Show total banana count, breakdown of banana count on each stage, and some kinda "Rank" based on how high the score is)