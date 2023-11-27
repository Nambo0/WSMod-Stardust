#include "achievement_display.h"

#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"

namespace achievement_display {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-achievement-display",
        .description = "Achievement Display",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick, ))

u8 display_queue[] = {0, 0, 0, 0, 0, 0, 0, 0};
u8 display_timer = 0;
u8 test_id = 1;

static void set_sprite_achievement_name(mkb::Sprite* sprite) {
    switch (display_queue[0]) {
        // Stage Challenges
        case 1:
            mkb::strcpy(sprite->text, "Double Take");
            break;
        case 2:
            mkb::strcpy(sprite->text, "Up, Up, and Away");
            break;
        case 3:
            mkb::strcpy(sprite->text, "Defused");
            break;
        case 4:
            mkb::strcpy(sprite->text, "I Wanna Be the Back Goal");
            break;
        case 5:
            mkb::strcpy(sprite->text, "Behind Locked Doors");
            break;
        case 6:
            mkb::strcpy(sprite->text, "Monochromatic");
            break;
        case 7:
            mkb::strcpy(sprite->text, "Target Master");
            break;
        case 8:
            mkb::strcpy(sprite->text, "Potassium Allergy");
            break;
        case 9:
            mkb::strcpy(sprite->text, "Flip Wizard");
            break;
        case 10:
            mkb::strcpy(sprite->text, "Starstruck");
            break;
        // Story Mode
        case 11:
            mkb::strcpy(sprite->text, "Beat the Game");
            break;
        case 12:
            mkb::strcpy(sprite->text, "Stunt Trainee");
            break;
        case 13:
            mkb::strcpy(sprite->text, "Stunt Pilot");
            break;
        case 14:
            mkb::strcpy(sprite->text, "Stunt Specialist");
            break;
        case 15:
            mkb::strcpy(sprite->text, "Stunt Ace");
            break;
        case 16:
            mkb::strcpy(sprite->text, "Eater of Souls");
            break;
        case 17:
            mkb::strcpy(sprite->text, "Eater of Worlds");
            break;
        // Interstellar
        case 21:
            mkb::strcpy(sprite->text, "Bronze Rank");
            break;
        case 22:
            mkb::strcpy(sprite->text, "Silver Rank");
            break;
        case 23:
            mkb::strcpy(sprite->text, "Gold Rank");
            break;
        case 24:
            mkb::strcpy(sprite->text, "Platinum Rank");
            break;
        case 25:
            mkb::strcpy(sprite->text, "Star Rank");
            break;
        case 26:
            mkb::strcpy(sprite->text, "Finish Him!");
            break;
        // Secret/Shadow
        case 31:
            mkb::strcpy(sprite->text, "Hey Goobz Play Debug");
            break;
        case 32:
            mkb::strcpy(sprite->text, "A Complex Joke");
            break;
        case 33:
            mkb::strcpy(sprite->text, "You-Da-Bacon");
            break;
        case 34:
            mkb::strcpy(sprite->text, "Spleef Rules Lawyer");
            break;
        case 35:
            mkb::strcpy(sprite->text, "Currents Rules Lawyer");
            break;
        case 36:
            mkb::strcpy(sprite->text, "Acutally Playable");
            break;
        case 37:
            mkb::strcpy(sprite->text, "Uhhh... GG!");
            break;
        case 38:
            mkb::strcpy(sprite->text, "AAAAAAAAA");
            break;
        // Error case
        default:
            mkb::strcpy(sprite->text, "???");
            break;
    }
}

void achievement_sprite_tick(u8* status, mkb::Sprite* sprite) {
    // Queue is empty
    if (display_queue[0] == 0) {
        sprite->alpha = 0;
        return;
    }
    // New achievement being displayed
    if (display_timer == 180) {
        set_sprite_achievement_name(sprite);
    }
    // Handle sprite alpha
    switch (display_timer) {
        case 0: {
            sprite->alpha = 0;
            break;
        }
        case 1 ... 20: {
            sprite->alpha = display_timer * 0.05;
            break;
        }
        case 21 ... 160: {
            sprite->alpha = 1;
            break;
        }
        case 161 ... 180: {
            sprite->alpha = (180 - display_timer) * 0.05;
            break;
        }
    }
}

void create_achievement_sprite() {
    mkb::Sprite* sprite = mkb::create_sprite();
    if (sprite != (mkb::Sprite*) 0x0) {
        sprite->pos.x = 60.0 + 20;
        sprite->pos.y = 420.0 - 40;
        sprite->font = mkb::FONT_ASC_24x24;
        sprite->alignment = mkb::ALIGN_CENTER_RIGHT;
        sprite->mult_color.red = 0xff;
        sprite->mult_color.green = 0x99;
        sprite->mult_color.blue = 0x00;
        sprite->font = mkb::FONT_ASC_24x24;
        sprite->fpara1 = 1.0;
        sprite->fpara2 = 0.0;
        sprite->alpha = 0.0;
        sprite->width = 0.6;
        sprite->height = 0.6;
        sprite->field21_0x20 = 1.0;
        sprite->field22_0x24 = 1.0;
        sprite->g_counter = 180;
        sprite->g_flags1 = sprite->g_flags1 | 0xa1000000;
        sprite->widescreen_translation_x = 0x60;
        sprite->tick_func = achievement_sprite_tick;
        set_sprite_achievement_name(sprite);
    }
    mkb::Sprite* sprite_shadow = mkb::create_sprite();
    if (sprite_shadow != nullptr) {
        sprite_shadow->pos.x = 60.0 + 20 + 2;
        sprite_shadow->pos.y = 420.0 - 40 + 2;
        sprite_shadow->font = mkb::FONT_ASC_24x24;
        sprite_shadow->alignment = mkb::ALIGN_CENTER_RIGHT;
        sprite_shadow->depth = 0.11;
        sprite_shadow->fpara1 = 0.45;
        sprite_shadow->fpara2 = 0.0;
        sprite_shadow->alpha = 0.0;
        (sprite_shadow->mult_color).red = 0x00;
        (sprite_shadow->mult_color).green = 0x00;
        (sprite_shadow->mult_color).blue = 0x00;
        sprite_shadow->width = 0.6;
        sprite_shadow->height = 0.6;
        sprite_shadow->field21_0x20 = 0.45;
        sprite_shadow->field22_0x24 = 0;
        sprite_shadow->g_counter = 180;
        sprite_shadow->g_flags1 = sprite->g_flags1 | 0xa1000000;
        sprite_shadow->widescreen_translation_x = 0x60;
        sprite_shadow->tick_func = achievement_sprite_tick;
        set_sprite_achievement_name(sprite);
    }
    sprite = mkb::create_sprite();
    if (sprite != (mkb::Sprite*) 0x0) {
        sprite->type = mkb::SPRT_BMP;
        sprite->pos.x = 60.0;
        sprite->pos.y = 420.0 - 40;
        sprite->alignment = mkb::ALIGN_CENTER;
        sprite->bmp = 0x5e;
        sprite->alpha = 0.0;
        sprite->g_counter = 180;
        sprite->g_flags1 = 0x1000000;
        sprite->width = 0.25;
        sprite->height = 0.25;
        sprite->widescreen_translation_x = 0x60;
        sprite->tick_func = achievement_sprite_tick;
        mkb::strcpy(sprite->text, "achievement icon");
    }
    return;
}

void add_achievement_to_display_queue(u8 id) {
    // Add to slot 1 (0 = active, 1 = "on deck")
    for (u8 queue_slot = 1; queue_slot < 8; queue_slot++) {
        // Interstellar ranks: Only display highest rank (replace lower ones in the queue)
        if (id >= 21 && id <= 25 && display_queue[queue_slot] < id) {
            display_queue[queue_slot] = id;
            return;
        }
        // Fill empty slot with id
        if (display_queue[queue_slot] == 0) {
            display_queue[queue_slot] = id;
            return;
        }
    }
}

void tick() {
    /* TEST BIND
    if(pad::button_pressed(mkb::PAD_BUTTON_DOWN)){
        add_achievement_to_display_queue(test_id);
        test_id++;
    } */

    // Update display
    if (display_timer > 0) display_timer -= 1;                     // Timer counting down
    else {                                                         // Timer = 0
        if (display_queue[1] == 0) display_timer = 0;              // Queue empty
        else {                                                     // Queue active
            for (u8 queue_slot = 0; queue_slot < 7; queue_slot++) {// Move queue forward
                display_queue[queue_slot] = display_queue[queue_slot + 1];
            }
            display_queue[7] = 0;
            if (display_queue[0] != 0) display_timer = 180;
        }
    }
}

static patch::Tramp<decltype(&mkb::create_hud_sprites)> s_create_hud_sprites_tramp;

void init() {
    patch::hook_function(s_create_hud_sprites_tramp, mkb::create_hud_sprites, []() {
        s_create_hud_sprites_tramp.dest();
        create_achievement_sprite();
    });
}

}// namespace achievement_display