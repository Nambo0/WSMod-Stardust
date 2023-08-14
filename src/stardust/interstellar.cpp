#include "interstellar.h"

#include "mkb/mkb.h"
#include "mkb/mkb2_ghidra.h"
#include "internal/patch.h"
#include "stardust/badge.h"
#include "internal/tickable.h"

namespace interstellar {

// Patch is enabled by default
TICKABLE_DEFINITION((
                        .name = "stardust-interstellar",
                        .description = "Interstellar",
                        .enabled = true,
                        .init_main_loop = init,
                        .tick = tick,
                        .on_goal = on_goal,
                    ))

static bool blitz_mode = false;
static u16 frames_left = 300*60;
static u32 bunches_gone[] = {0,0,0, 0}; // Bitfield array for keeping track of which bunches on a stage were collected between attempts
static u32 anim_states = 0; // Bitfield array for keeping track of which play-once anims were activated between attempts
// NOTE: Only tracks anim IDs 1-32, so I gotta remember to have all fallout-sustained switches be anim ID 1-32

//Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

// void create_fallout_or_bonus_finish_sprite(s32 param_1);
static patch::Tramp<decltype(&mkb::create_fallout_or_bonus_finish_sprite)> s_create_fallout_or_bonus_finish_sprite_tramp;
// void g_reset_ball(struct Ball *in_ball);
static patch::Tramp<decltype(&mkb::g_reset_ball)> s_g_reset_ball_tramp;
// void load_stagedef(u32 stage_id);
static patch::Tramp<decltype(&mkb::load_stagedef)> s_load_stagedef_tramp;

static bool stage_id_is_stellar(u32 stage_id){
    switch (stage_id) {
        case 221 ... 230: {
            return true; 
        }
        default: {
            return false;
        } 
    }
}

void on_stage_load(u32 stage_id){
    if(stage_id_is_stellar(stage_id)){
        if(mkb::main_game_mode == mkb::CHALLENGE_MODE){
            // Set proper timer (300s for standard, 60s for blitz)
            if(blitz_mode) frames_left = 60*60;
            else frames_left = 300*60;

            // Set all bunches to un-collected
            for(u32 i=0; i < 4; i++){
                bunches_gone[i] = 0;
            }
        }
    }
}

void tick(){
    if(stage_id_is_stellar(mkb::g_current_stage_id)){
        if(mkb::main_game_mode == mkb::PRACTICE_MODE && 
        (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN)){
            if(mkb::mode_info.stage_time_frames_remaining == frames_left - 1){
                mkb::mode_info.stage_time_frames_remaining += 2;
                frames_left = mkb::mode_info.stage_time_frames_remaining;
            }
        frames_left = mkb::mode_info.stage_time_frames_remaining;
        }
        if(mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::mode_info.stage_time_frames_remaining <= 20*60){
            mkb::mode_info.ball_mode |= 1 << 6;
        }
    }
}

void on_goal(){
    if(stage_id_is_stellar(mkb::g_current_stage_id)){
        // Goal Bonus
        ball.banana_count += 50;
        // TODO: Display "+50" below the banana counter!

        // Sweep Bonus
        if(badge::detect_sweep()){
            u32 seconds_remaining = mkb::mode_info.stage_time_frames_remaining / 60;
            u32 bonus_total = seconds_remaining * 5;
            ball.banana_count += bonus_total;
            // TODO: Display "Sweep Bonus: # seconds = +###[banana icon]"
        }
    }
}

void remove_banana(mkb::Item &item){
    item.g_some_flag = 0;
    item.g_some_bitfield = item.g_some_bitfield | 1;
    item.g_some_bitfield = item.g_some_bitfield & 0xfffffffd;
}

void on_fallout(){
    if(stage_id_is_stellar(mkb::g_current_stage_id)){
        if(mkb::main_game_mode == mkb::CHALLENGE_MODE){
            // Fallout Timer Penalty
            if(mkb::mode_info.stage_time_frames_remaining > 20*60){
                frames_left = mkb::mode_info.stage_time_frames_remaining - 20*60;
            }
            else{
                mkb::mode_info.ball_mode |= 1 << 6;
            }
            // TO-DO: Display "-20" below the timer!

            // Save banana state
            for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
                if (mkb::item_pool_info.status_list[i] == 0) continue; // skip if its inactive
                mkb::Item &item = mkb::items[i]; // shorthand: current item in the list = "item"
                if (item.coin_type != 1) continue; // skip if its not a bunch
                if(item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd){ // True if banana is gone
                    u32 array_index = i / 32; // Determine the index of the array element containing the i'th bit
                    u32 bit_offset = i % 32;    // Determine the bit offset within the array element
                    bunches_gone[array_index] |= 1 << bit_offset;
                }
            }

            // Save play-once animation states 
            anim_states = 0;
            for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                if(mkb::stagedef->coli_header_list[i].anim_group_id >= 1
                && mkb::stagedef->coli_header_list[i].anim_group_id <= 32
                && mkb::itemgroups[i].anim_frame > 1){
                    anim_states |= 1 << mkb::stagedef->coli_header_list[i].anim_group_id;
                }
            }
        }
        if(mkb::main_game_mode == mkb::PRACTICE_MODE){
            frames_left = 2;
            mkb::mode_info.stage_time_frames_remaining = 2;
        }
    }
}

void on_spin_in(){
    if(stage_id_is_stellar(mkb::g_current_stage_id)){
        mkb::mode_info.stage_time_limit = 2; // Prevents replay-memory crashes
        if(mkb::main_game_mode == mkb::CHALLENGE_MODE){
            // Stage timer keeps ticking between attempts
            mkb::mode_info.stage_time_frames_remaining = frames_left;

            // Renew banana state from previous attempt
            for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
                if (mkb::item_pool_info.status_list[i] == 0) continue; // skip if its inactive
                mkb::Item &item = mkb::items[i]; // shorthand: current item in the list = "item"
                if (item.coin_type != 1) continue; // skip if its not a bunch
                u32 array_index = i / 32; // Determine the index of the array element containing the i'th bit
                u32 bit_offset = i % 32;    // Determine the bit offset within the array element
                if(bunches_gone[array_index] & (1 << bit_offset)) remove_banana(item);
            }

            // Renew play-once animation states from previous attempt
            for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
                if(anim_states & (1 << mkb::stagedef->coli_header_list[i].anim_group_id)){
                    mkb::itemgroups[i].playback_state = 0;
                }
            }

        }
        if(mkb::main_game_mode == mkb::PRACTICE_MODE){
            frames_left = 2;
            mkb::mode_info.stage_time_frames_remaining = 2;
        }
    }
}

void init(){
    patch::hook_function(s_create_fallout_or_bonus_finish_sprite_tramp, mkb::create_fallout_or_bonus_finish_sprite, [](s32 param_1) {
        on_fallout();
        s_create_fallout_or_bonus_finish_sprite_tramp.dest(param_1);
    });
    patch::hook_function(s_g_reset_ball_tramp, mkb::g_reset_ball, [](mkb::Ball *in_ball) {
        s_g_reset_ball_tramp.dest(in_ball);
        on_spin_in();
    });
    patch::hook_function(s_load_stagedef_tramp, mkb::load_stagedef, [](u32 stage_id) {
        s_load_stagedef_tramp.dest(stage_id);
        on_stage_load(stage_id);
    });
}

} // namespace interstellar

// no 1-ups
// Goal bonus (Goal = +50 bananas, show little indicator under banana counter like how warp goal score bonuses work)
// Sweep bonus (All bananas collected + goal = 5 bananas per second left on the timer, also show a little indicator when this happens)
// End screen (Show total banana count, breakdown of banana count on each stage, and some kinda "Rank" based on how high the score is)
// Custom timer (max 2 frames, 100 or 300 depending on blitz mode, 20s penalty on death)