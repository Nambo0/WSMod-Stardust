#include "badge.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "mkb/mkb.h"
#include "../stardust/validate.h"
#include "../stardust/savedata.h"

namespace badge {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-badge",
        .description = "Badges",
        .enabled = true,
        .tick = tick,
        .on_goal = on_goal, ))

// Wait 1 frame before display to make sure all cpp files are done calling for badge claims
// This also prevents the function from running more than once in a row!
static u8 badge_display_delay = 100;

void set_display_badges_next_frame_true(){
    badge_display_delay = 1;
}

int stage_id_to_stage_number(int stage_id) {
    switch (stage_id) {
        case 201 ... 204:
            return stage_id - 200;
            break;
        case 1 ... 16:
            return stage_id + 4;
            break;
        case 231 ... 239:
            return stage_id - 210;
            break;
        case 17 ... 47:
            return stage_id + 13;
            break;
        case 281 ... 289:
            return stage_id - 220;
            break;
        case 48 ... 68:
            return stage_id + 22;
            break;
        case 341 ... 350:
            return stage_id - 250;
            break;
        default:
            return 0; // Any non-story mode stage
    }
}

bool detect_sweep() {
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue;                                       // skip if its inactive
        mkb::Item& item = mkb::items[i];                                                             // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue;                                                           // skip if its not a bunch
        if (item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd) {// True if banana is gone
            continue;
        }
        else return false;// Returns false if any bunches are un-collected
    }
    return true;
}

void claim_blue_goal(u16 stage_number){
    if(stage_number == 0) return; // Don't try to claim anything if we're not on a story mode stage
    u32 claimed_slot = savedata::CLEAR_BADGE_START + stage_number - 1; // Blue goals start at slot 0
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
        set_display_badges_next_frame_true();
    }
}

void claim_stunt_goal(u16 stage_number){
    if(stage_number == 0) return; // Don't try to claim anything if we're not on a story mode stage
    u32 claimed_slot = savedata::STUNT_BADGE_START + stage_number - 1; // Stunt goals start at slot 100
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
        set_display_badges_next_frame_true();
    }
}

void claim_sweep(u16 stage_number){
    if(stage_number == 0) return; // Don't try to claim anything if we're not on a story mode stage
    u32 claimed_slot = savedata::SWEEP_BADGE_START + stage_number - 1; // Sweep badges start at slot 200
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
        set_display_badges_next_frame_true();
    }
}

void badge_sprite_tick(u8* status, mkb::Sprite *sprite) {
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

static void create_badge_sprite(u8 type, u8 slot){
    mkb::Sprite* sprite = mkb::create_sprite();
    if (sprite != (mkb::Sprite *)0x0) {
        sprite->type = mkb::SPRT_BMP;
        sprite->pos.x = 125.0 + slot*40;
        sprite->pos.y = 420.0;
        sprite->alignment = mkb::ALIGN_CENTER;
        switch(type){
            case 0: sprite->bmp = 0x32; break; // Clear
            case 1: sprite->bmp = 0x1e; break; // Stunt
            case 2: sprite->bmp = 0x42; break; // Sweep
            case 3: sprite->bmp = 0x5e; break; // Ach
            case 4: sprite->bmp = 0x49; break; // Empty
        }
        sprite->alpha = 0.0;
        sprite->g_counter = 300;
        sprite->g_flags1 = 0x1000000;
        sprite->width = 0.5;
        sprite->height = 0.5;
        sprite->widescreen_translation_x = 0x140;
        sprite->tick_func = badge_sprite_tick;
        switch(type){
            case 0: mkb::strcpy(sprite->text, "clear badge"); break; // Clear
            case 1: mkb::strcpy(sprite->text, "stunt badge"); break; // Stunt
            case 2: mkb::strcpy(sprite->text, "sweep badge"); break; // Sweep
            case 3: mkb::strcpy(sprite->text, "achmt badge"); break; // Ach
            case 4: mkb::strcpy(sprite->text, "empty badge"); break; // Empty
        }
    }
}

static void display_badges(u16 stage_number){
    // Clear Badge
    if(savedata::true_in_slot(stage_number - 1)) create_badge_sprite(0, 0);
    else create_badge_sprite(4, 0);
    // Stunt Badge
    if(savedata::true_in_slot(100 + stage_number - 1)) create_badge_sprite(1, 1);
    else create_badge_sprite(4, 1);
    // Sweep Badge
    if(savedata::true_in_slot(200 + stage_number - 1)) create_badge_sprite(2, 2);
    else create_badge_sprite(4, 2);
    // Stage Challenge (ONLY SHOW ON ACHIEVEMENT STAGES)
    switch(stage_number) {
        case 8:  // 1-8 Double Time
        case 16: // 2-6 Liftoff
        case 30: // 3-10 Detonation
        case 39: // 4-9 Avoidance
        case 46: // 5-6 Door Dash
        case 51: // 6-1 Recolor
        case 70: // 7-10 Break the Targets
        case 74: // 8-4 Frequencies
        case 83: // 9-3 Flip Switches
        case 100: { // 10-10 Impact
            if(savedata::true_in_slot(300 + (stage_number - 1) / 10)) create_badge_sprite(3, 3);
            else create_badge_sprite(4, 3);
            break;
        }
    }
}

void tick(){
    if(badge_display_delay == 0){
        display_badges(stage_id_to_stage_number(mkb::g_current_stage_id));
        badge_display_delay = 100;
    }
    if(badge_display_delay == 1){
        badge_display_delay = 0;
    }
}

void on_goal() {
    if(validate::is_currently_valid()){
        if(mkb::mode_info.entered_goal_type == mkb::Blue){
            claim_blue_goal(stage_id_to_stage_number(mkb::g_current_stage_id));
        }
        else if(mkb::mode_info.entered_goal_type == mkb::Red){
            claim_stunt_goal(stage_id_to_stage_number(mkb::g_current_stage_id));
        }
        if(detect_sweep()){
            claim_sweep(stage_id_to_stage_number(mkb::g_current_stage_id));
        }
    }
    else{
        // TODO: Error message
    }
}

}// namespace badge