#include "bunch_count.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"

namespace bunch_count {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-bunch-count",
        .description = "Bunch Count",
        .enabled = true,
        .init_main_loop = init, 
        .tick = tick, ))

static u8 bunches_collected = 0;
static u8 bunches_total = 0;

static void count_bunches() {
    bunches_collected = 0;
    bunches_total = 0;
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue;                                       // skip if its inactive
        mkb::Item& item = mkb::items[i];                                                             // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue;                                                           // skip if its not a bunch
        bunches_total++;
        if (item.g_some_flag == 0 /*&& item.g_some_bitfield & (1 << 0)*/ /*&& item.g_some_bitfield & 0xfffffffd*/) {// True if banana is gone
            bunches_collected++;
        }
    }
}

static void sprite_bunch_count_tick(u8 *status,mkb::Sprite *sprite) {
    if(bunches_collected != 0){
        mkb::Sprite *other_sprite;
    
        other_sprite = mkb::get_sprite_with_unique_id(mkb::SPRITE_HUD_KIWAKU_WORLD);
        if (((other_sprite != (mkb::Sprite *)0x0) && (other_sprite->g_counter == 0)) && (sprite->fpara2 < 1.0)) {
            sprite->fpara2 = sprite->fpara2 + 0.05;
        }
        sprite->alpha = sprite->fpara1 * sprite->fpara2;
        if (sprite->font == mkb::FONT_ASC_24x24) {
            mkb::sprintf(sprite->text, "%u/%u", bunches_collected,bunches_total);
        }
    }
}

static void display_counter(){
    if (mkb::main_game_mode == mkb::STORY_MODE || (mkb::main_game_mode == mkb::PRACTICE_MODE && mkb::stageselect_is_storymode == true)) {
    mkb::Sprite* sprite = mkb::create_sprite();
    if (sprite != nullptr) {
        sprite->pos.x = 54;
        sprite->pos.y = 462;
        sprite->type = mkb::SPRT_BMP;
        sprite->fpara1 = 1.0;
        sprite->fpara2 = 0.0;
        sprite->alpha = 0.0;
        (sprite->mult_color).red = 0xff;
        (sprite->mult_color).green = 0xff;
        (sprite->mult_color).blue = 0x00;
        sprite->bmp = 0x59;
        sprite->width = 0.14;
        sprite->height = 0.14;
        sprite->field21_0x20 = 1.0;
        sprite->field22_0x24 = 1.0;
        sprite->g_flags1 = sprite->g_flags1 | 0xa1000000;
        sprite->widescreen_translation_x = 0x60;
        sprite->tick_func = sprite_bunch_count_tick;
        mkb::strcpy(sprite->text, "bunch");
    }
    mkb::Sprite* sprite_1 = mkb::create_sprite();
    if (sprite_1 != nullptr) {
        sprite_1->pos.x = 57;
        sprite_1->pos.y = 465;
        sprite_1->type = mkb::SPRT_BMP;
        sprite_1->depth = 0.11;
        sprite_1->fpara1 = 0.45;
        sprite_1->fpara2 = 0.0;
        sprite_1->alpha = 0.0;
        (sprite_1->mult_color).red = 0x00;
        (sprite_1->mult_color).green = 0x00;
        (sprite_1->mult_color).blue = 0x00;
        sprite_1->bmp = 0x59;
        sprite_1->width = 0.14;
        sprite_1->height = 0.14;
        sprite_1->field21_0x20 = 0.45;
        sprite_1->field22_0x24 = 0;
        sprite_1->g_flags1 = sprite_1->g_flags1 | 0xa1000000;
        sprite_1->widescreen_translation_x = 0x60;
        sprite_1->tick_func = sprite_bunch_count_tick;
    }
    mkb::Sprite* sprite_2 = mkb::create_sprite();
    if (sprite_2 != nullptr) {
        sprite_2->pos.x = 75;
        sprite_2->pos.y = 462;
        sprite_2->font = mkb::FONT_ASC_24x24;
        sprite_2->fpara1 = 1.0;
        sprite_2->fpara2 = 0.0;
        sprite_2->alpha = 0.0;
        (sprite_2->mult_color).red = 0xff;
        (sprite_2->mult_color).green = 0xff;
        (sprite_2->mult_color).blue = 0x00;
        sprite_2->width = 0.6;
        sprite_2->height = 0.6;
        sprite_2->field21_0x20 = 1.0;
        sprite_2->field22_0x24 = 1.0;
        sprite_2->g_flags1 = sprite_2->g_flags1 | 0xa1000000;
        sprite_2->widescreen_translation_x = 0x60;
        sprite_2->tick_func = sprite_bunch_count_tick;
    }
    mkb::Sprite* sprite_shadow = mkb::create_sprite();
    if (sprite_shadow != nullptr) {
        sprite_shadow->pos.x = 78;
        sprite_shadow->pos.y = 465;
        sprite_shadow->font = mkb::FONT_ASC_24x24;
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
        sprite_shadow->g_flags1 = sprite->g_flags1 | 0xa1000000;
        sprite_shadow->widescreen_translation_x = 0x60;
        sprite_shadow->tick_func = sprite_bunch_count_tick;
    }
    }
}

static patch::Tramp<decltype(&mkb::item_coin_disp)> s_item_coin_disp_tramp;
static patch::Tramp<decltype(&mkb::create_hud_sprites)> s_create_hud_sprites_tramp;

void init() {
    patch::hook_function(s_create_hud_sprites_tramp, mkb::create_hud_sprites, []() {
        s_create_hud_sprites_tramp.dest();
        display_counter();
    });
}

void tick(){
    count_bunches();
}

}// namespace bunch_count