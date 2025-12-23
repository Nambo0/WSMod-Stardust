#include "interstellar.h"
#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"
#include "../stardust/achievement.h"
#include "../stardust/badge.h"
#include "../stardust/savedata.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_menu.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "pausecooldown.h"
#include "utils/ppcutil.h"
#include "widget_input.h"
#include "widget_sprite.h"


namespace interstellar {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-interstellar",
        .description = "Interstellar",
        .enabled = true,
        .init_main_loop = init,
        .init_main_game = init_main_game,
        .init_sel_ngc = init_sel_ngc,
        .tick = tick,
        .on_goal = on_goal, ))

static u8 goal_bonus_effect = 0;
bool paused_now = *reinterpret_cast<u32*>(0x805BC474) & 8;
static u16 frames_left = 300 * 60;
static u32 bunches_gone[] = {0, 0, 0, 0};// Bitfield array for keeping track of which bunches on a stage were collected between attempts
static u32 anim_states = 0;              // Bitfield array for keeping track of which play-once anims were activated between attempts
static u8 this_rank = 0;                 // Rank of the current finished run
// NOTE: Only tracks anim IDs 1-32, so I gotta remember to have all fallout-sustained switches be anim ID 1-32
char endtext_buffer[1024];

bool timer_wrap_count = 0;

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

// void create_fallout_or_bonus_finish_sprite(s32 param_1);
static patch::Tramp<decltype(&mkb::create_fallout_or_bonus_finish_sprite)> s_create_fallout_or_bonus_finish_sprite_tramp;
// void g_reset_ball(struct Ball *in_ball);
static patch::Tramp<decltype(&mkb::g_reset_ball)> s_g_reset_ball_tramp;
// void load_stagedef(u32 stage_id);
static patch::Tramp<decltype(&mkb::load_stagedef)> s_load_stagedef_tramp;
static patch::Tramp<decltype(&mkb::smd_game_ringout_tick)> s_smd_game_ringout_tick_tramp;
static patch::Tramp<decltype(&mkb::smd_game_timeover_tick)> s_smd_game_timeover_tick_tramp;
static patch::Tramp<decltype(&mkb::smd_game_roll_init)> s_smd_game_roll_init_tramp;
// void smd_game_nameentry_tick(void);
static patch::Tramp<decltype(&mkb::smd_game_nameentry_tick)> s_smd_game_nameentry_tick_tramp;

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

static u8 bunches_collected_on_stage[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u16 bunches_collected_total() {
    u16 total = 0;
    for (int i = 0; i < 10; i++) {
        total += bunches_collected_on_stage[i];
    }
    return total;
}

static void save_finished_run_total() {
    savedata::update_stellar_bunch_counts(bunches_collected_on_stage);
    savedata::save();
}

static void finished_run_calculations() {
    bunches_collected_on_stage[9] = (mkb::balls[mkb::curr_player_idx].banana_count / 10) - bunches_collected_total();
    this_rank = mkb::balls[mkb::curr_player_idx].banana_count / 1000;
    if (this_rank > 5) this_rank = 5;
    if (bunches_collected_total() > savedata::stellar_best_run_total()) {
        save_finished_run_total();
        // Collect rank achievement(s) (ID: 21-25)
        for (u16 rank = 1; rank <= 5; rank++) {
            if (mkb::balls[mkb::curr_player_idx].banana_count >= rank * 1000) {
                achievement::claim_achievement(20 + rank);
            }
        }
        // Ach ID: 37) UHHH GG | Complete Interstellar with 0 bananas (DISCONTINUED)
        // if (mkb::balls[mkb::curr_player_idx].banana_count == 0) achievement::claim_achievement(37);
    }
}

static void spawn_banana_effect() {
    mkb::Effect effect;
    mkb::Ball ball = mkb::balls[mkb::curr_player_idx];
    mkb::Item& dummy_item = mkb::items[1];
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue;// skip if its inactive
        mkb::Item& item = mkb::items[i];                      // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue;                    // skip if its not a bunch
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

void goal_bonus_sprite_tick(u8* status, mkb::Sprite* sprite) {
    if (sprite->g_counter > 0) {
        sprite->alpha += 0.05;
        if (sprite->alpha > 1) {
            sprite->alpha = 1;
        }
    }
    sprite->g_counter -= 1;
    if (sprite->g_counter < 0) {
        sprite->g_counter = 0;
        sprite->alpha -= 0.05;
        if (sprite->alpha < 0) {
            sprite->alpha = 0;
        }
    }
}

void create_goal_bonus_sprite() {
    mkb::Sprite* sprite = mkb::create_sprite();
    if (sprite != (mkb::Sprite*) 0x0) {
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
    if (sprite != (mkb::Sprite*) 0x0) {
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

void create_penalty_sprite(s16 counter) {

    mkb::Sprite* sprite = mkb::create_sprite();
    if (sprite != (mkb::Sprite*) 0x0) {
        sprite->pos.x = 320.0;
        sprite->pos.y = 380.0;
        sprite->font = mkb::FONT_JAP_24x24_2;
        sprite->alignment = mkb::ALIGN_CENTER;
        sprite->mult_color.red = 0xff;
        sprite->mult_color.green = 0x80;
        sprite->mult_color.blue = 0x00;
        sprite->alpha = 0.0;
        sprite->g_counter = counter;
        sprite->g_flags1 = 0x1000000;
        sprite->widescreen_translation_x = 0x140;
        sprite->tick_func = goal_bonus_sprite_tick;
        mkb::strcpy(sprite->text, "PENALTY  -15");
    }
    sprite = mkb::create_sprite();
    if (sprite != (mkb::Sprite*) 0x0) {
        sprite->type = mkb::SPRT_BMP;
        sprite->pos.x = 367.0;
        sprite->pos.y = 380.0;
        sprite->alignment = mkb::ALIGN_CENTER;
        sprite->bmp = 0x510;
        sprite->alpha = 0.0;
        sprite->g_counter = counter;
        sprite->g_flags1 = 0x1000000;
        sprite->width = 0.75;
        sprite->height = 0.75;
        sprite->widescreen_translation_x = 0x140;
        sprite->tick_func = goal_bonus_sprite_tick;
        mkb::strcpy(sprite->text, "penalty time");
    }
    return;
}

int stage_id_replay_frames(u32 stage_id) {
    switch(stage_id) {
        case 91: return 60 * 300;  // Record Scratch
        case 92: return 60 * 60;   // Dodge Demon
        case 93 ... 100: return 60 * 300; // The rest of Silent Supernova
        default: return 2; // 2 instead of 0 prevents replay crashes
    }
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
        }
    }

    // Handle frozen & increasing timers
    if ((mkb::main_game_mode == mkb::PRACTICE_MODE && stage_id_is_stellar(stage_id)) // Stellar practice
        || stage_id == 267 // Stellar W2 Draft
        || (stage_id >= 91 && stage_id <= 100)) { // Silent Supernova
        // time over at -60 frames (so timer is able to stop at 0.00)
        *reinterpret_cast<u32*>(0x80297548) = 0x2c00ffa0;
        // Add 1 to the timer each frame (increasing)
        patch::write_word(reinterpret_cast<u32*>(0x80297534), 0x38030001);
    }
    else if (stage_id == 77) {
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

bool lock_name_entry;
bool can_view = true;

void end_screen() {
    constexpr char* s_stellar_ranks[6] = {
        "NONE",
        "/bcB68E00/BRONZE/bcFFFFFF/",
        "/bcCCCCCC/SILVER/bcFFFFFF/",
        "/bcFFDD00/GOLD/bcFFFFFF/",
        "/bc6EFFFD/PLATINUM/bcFFFFFF/",
        "/bcC800FF/STAR/bcFFFFFF/"};
    u8 new_best_state = 0;// 1 = display new best text
    if (bunches_collected_total() == savedata::stellar_best_run_total()) new_best_state = 1;
    constexpr char* s_new_best_text[2] = {
        "",
        "(New best!)"};
    constexpr char* s_text_format =
        "/bc00fffb/THIS RUN/bcFFFFFF/\n"
        "\n"
        "/bcFFFFFF/Grand Total: /bcFBFF00/%d/bcFFFFFF/ %s\n"
        "\n"
        "Rank: %s\n"
        "\n"
        "World 1: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 2: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 3: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 4: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 5: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 6: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 7: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 8: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 9: /bcFBFF00/%d/bcFFFFFF/\n"
        "World 10: /bcFBFF00/%d/bcFFFFFF/\n"
        "\n"
        "Press p/BUTTON_B/ to continue.";
    auto& end_box = ui::get_widget_manager().add(new ui::Window(Vec2d{80, 40}, Vec2d{480, 405}));
    end_box.set_alignment(mkb::ALIGN_CENTER);
    end_box.set_label("endbox");
    mkb::sprintf(endtext_buffer,
                 s_text_format,
                 bunches_collected_total() * 10,
                 s_new_best_text[new_best_state],
                 s_stellar_ranks[this_rank],
                 bunches_collected_on_stage[0] * 10,
                 bunches_collected_on_stage[1] * 10,
                 bunches_collected_on_stage[2] * 10,
                 bunches_collected_on_stage[3] * 10,
                 bunches_collected_on_stage[4] * 10,
                 bunches_collected_on_stage[5] * 10,
                 bunches_collected_on_stage[6] * 10,
                 bunches_collected_on_stage[7] * 10,
                 bunches_collected_on_stage[8] * 10,
                 bunches_collected_on_stage[9] * 10);
    auto& end_text = end_box.add(new ui::Text(endtext_buffer));
    end_text.set_alignment(ui::CENTER);
    can_view = false;
    lock_name_entry = true;
}

void tick() {
    // End screen display
    if (mkb::scen_info.next_world != 11 && mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
        if (mkb::sub_mode == mkb::SMD_GAME_NAMEENTRY_MAIN) {
            if (mkb::g_nameentry_did_get_top_5) {
                if (mkb::g_nameentry_state == 3) {
                    if (can_view) {
                        end_screen();
                    }
                }
            }
            else {
                if (can_view) {
                    end_screen();
                }
            }
            if (pad::button_pressed(mkb::PAD_BUTTON_B)) {
                ui::get_widget_manager().remove("endbox");
                lock_name_entry = false;
            }
        }
        else {
            ui::get_widget_manager().remove("endbox");
        }
        if (mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
            can_view = true;
        }
    }

    // Regular per-frame stuff
    if ((stage_id_is_stellar(mkb::current_stage_id) && mkb::main_game_mode == mkb::PRACTICE_MODE) // Interstellar practice
        || mkb::current_stage_id == 267 // Stellar W2 Draft
        || (mkb::current_stage_id >= 91 && mkb::current_stage_id <= 100)) { // Silent Supernova
        if (mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT || mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) {
            if (mkb::mode_info.stage_time_frames_remaining >= 500 * 60) {
                // Loop timer to 0 at 500
                mkb::mode_info.stage_time_frames_remaining = 0;
                timer_wrap_count += 1;
            }
        }
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::mode_info.stage_time_frames_remaining <= 15 * 60 && mkb::g_current_stage_id != 267) {
            mkb::mode_info.ball_mode |= 1 << 6;
        }
    }

    if (goal_bonus_effect > 0 && !paused_now) {
        goal_bonus_effect++;
        if (goal_bonus_effect % 6 == 0) {
            spawn_banana_effect();
        }
        if (goal_bonus_effect > 30) {
            goal_bonus_effect = 0;
        }
    }

    /* // BINDS FOR TESTING INTERSTELLAR SCORE CALCULATIONS
    if (pad::button_pressed(mkb::PAD_BUTTON_DOWN)) {
        mkb::balls[mkb::curr_player_idx].banana_count += 100;
    } */

    // Only run at the start of each stage
    bool paused_now = *reinterpret_cast<u32*>(0x805BC474) & 8;
    if (stage_id_is_stellar(mkb::g_current_stage_id) && mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::mode_info.stage_time_frames_remaining == 300 * 60 - 1 && !paused_now) {
        // (i1 only) Reset logged stage bunch counts
        if (mkb::g_current_stage_id == 221) {
            for (int i = 0; i < 10; i++) {
                bunches_collected_on_stage[i] = 0;
            }
        }
        // (i2-i10) Count bunches on the previous stage
        else {
            bunches_collected_on_stage[mkb::g_current_stage_id - 222] = (mkb::balls[mkb::curr_player_idx].banana_count / 10) - bunches_collected_total();
        }
    }
    if (mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::g_current_stage_id == 230 && mkb::sub_mode == mkb::SMD_GAME_TIMEOVER_INIT && !paused_now) finished_run_calculations();// Run ends via 0.00 timeover
}

void on_goal() {
    if (stage_id_is_stellar(mkb::g_current_stage_id)) {
        // Goal Bonus
        ball.banana_count += 50;
        ball.score += 5 * 1000;
        goal_bonus_effect = 1;
        create_goal_bonus_sprite();
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE && mkb::g_current_stage_id == 230) finished_run_calculations();// Run ends via goal

        // Practice mode time bonus
        if (mkb::main_game_mode == mkb::PRACTICE_MODE && timer_wrap_count < 2 && ball.banana_count == 1050) {
            ball.score += (100000 - (timer_wrap_count * 50000) - (mkb::mode_info.stage_time_frames_remaining * 100 / 60));
        }
    }
}

void on_fallout() {
    if (stage_id_is_stellar(mkb::g_current_stage_id)) {
        if (mkb::main_game_mode == mkb::CHALLENGE_MODE) {
            // Fallout Timer Penalty
            if (mkb::mode_info.stage_time_frames_remaining > 15 * 60) {
                if (mkb::mode_info.stage_time_frames_remaining < 300 * 60) {
                    frames_left = mkb::mode_info.stage_time_frames_remaining - 15 * 60;
                }
            }
            else {
                // Cause bonus finish if time is over
                mkb::mode_info.ball_mode |= 1 << 6;
                if (mkb::g_current_stage_id == 230) finished_run_calculations();// Run ends via <15s fallout
            }
            create_penalty_sprite(300);

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
    if (mkb::g_current_stage_id == 267 || (mkb::current_stage_id >= 91 && mkb::current_stage_id <= 100)) {
        mkb::mode_info.stage_time_limit = stage_id_replay_frames(mkb::current_stage_id); // Prevents replay-memory crashes
        mkb::mode_info.stage_time_frames_remaining = 0;
    }

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
            if (pausecooldown::is_retry == true && mkb::mode_info.stage_time_frames_remaining < 300 * 60) {
                create_penalty_sprite(100);
            }
        }
        if (mkb::main_game_mode == mkb::PRACTICE_MODE) {
            frames_left = 0;
            mkb::mode_info.stage_time_frames_remaining = 0;
            timer_wrap_count = 0;
        }
    }
}

void on_bonus_finish() {
    if ((mkb::current_stage_id == 230) && (mkb::mode_info.stage_time_frames_remaining < 15 * 60) && (mkb::in_practice_mode == false)) {
        if (mkb::sub_mode_frame_counter == 0x3c) {
            mkb::fade_screen_to_color(0x101, 0, 0x3d);
            mkb::g_fade_track_volume(0x3c, '\x02');
        }
        if (mkb::sub_mode_frame_counter < 1) {
            mkb::g_smth_with_ending_course();
            mkb::g_smth_with_ending_course_2();
            mkb::g_change_music_volume(0xc, 0x3c, '\0');
            mkb::main_mode_request = mkb::MD_AUTHOR;
            mkb::sub_mode_request = mkb::SMD_AUTHOR_PLAY_ENDING_INIT;
            mkb::mode_flags = mkb::mode_flags | 0x100040;
        }
    }
    if ((mkb::current_stage_id == 71) && (mkb::mode_info.stage_time_frames_remaining < 1) && (mkb::in_practice_mode == false) && (mkb::mode_info.ball_mode & (1 << 6))) {
        if (mkb::sub_mode_frame_counter == 0x3c) {
            mkb::fade_screen_to_color(0x101, 0, 0x3d);
            mkb::g_fade_track_volume(0x3c, '\x02');
        }
        if (mkb::sub_mode_frame_counter < 1) {
            mkb::g_smth_with_ending_course();
            mkb::g_smth_with_ending_course_2();
            mkb::g_change_music_volume(0xc, 0x3c, '\0');
            mkb::main_mode_request = mkb::MD_AUTHOR;
            mkb::sub_mode_request = mkb::SMD_AUTHOR_PLAY_ENDING_INIT;
            mkb::mode_flags = mkb::mode_flags | 0x100040;
        }
    }
}

void medallions() {
    if (mkb::scen_info.next_world <= 9 && mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
        patch::write_word(reinterpret_cast<void*>(0x8090a004), PPC_INSTR_LI(PPC_R3, 170 + this_rank));
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

void init_main_game() {
    if (mkb::curr_difficulty == mkb::DIFF_BEGINNER) {
        if ((mkb::main_game_mode == mkb::CHALLENGE_MODE) || (mkb::main_game_mode == mkb::PRACTICE_MODE && mkb::stageselect_is_storymode == false)) {
            patch::write_nop(reinterpret_cast<void*>(0x808f54f4));
            patch::write_nop(reinterpret_cast<void*>(0x808f5b90));
        }
    }
    patch::hook_function(s_smd_game_ringout_tick_tramp, mkb::smd_game_ringout_tick, []() {
        s_smd_game_ringout_tick_tramp.dest();
        on_bonus_finish();
    });
    patch::hook_function(s_smd_game_timeover_tick_tramp, mkb::smd_game_timeover_tick, []() {
        s_smd_game_timeover_tick_tramp.dest();
        on_bonus_finish();
    });
    patch::hook_function(s_smd_game_roll_init_tramp, mkb::smd_game_roll_init, []() {
        medallions();
        s_smd_game_roll_init_tramp.dest();
    });
    patch::hook_function(s_smd_game_nameentry_tick_tramp, mkb::smd_game_nameentry_tick, []() {
        if (!lock_name_entry) s_smd_game_nameentry_tick_tramp.dest();
    });
}

void init_sel_ngc() {
    mkb::scen_info.next_world = 0;
}

}// namespace interstellar

// TODO: End screen (Show total banana count, breakdown of banana count on each stage, and some kinda "Rank" based on how high the score is)