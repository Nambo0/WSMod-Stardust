#include "scratch.h"
#include "pad.h"
#include "ui_box.h"

namespace scratch
{

char* msg = "Achievement unlocked!";
char* msg1 = "Banana mania";
char* msg2_f = "Collect at least 15 bananas. (%d/15)";
char msg2[48];
u32 flash_color;
u8 fc;
mkb::Sprite* sprite_banana = nullptr;
bool sent = false;
bool sent2 = false;
u32 mystery = 0;
u32 mystery_2 = 0;
/*
void sprite_disp(mkb::Sprite* sprite) {

    mkb::sprintf(msg2, msg2_f, mkb::balls[0].banana_count);

    fc = static_cast<u8>((mkb::math_sin(sprite->para1)+1.0)*127.0);
    flash_color = fc + (fc<<8) + (fc<<16);

    sprite->para1 += 546*2;
    if (sprite->para2 < 16384) {
        sprite->pos.x = -100.0 + mkb::math_sin(sprite->para2)*225;
    }

    sprite->para2 += 546;

    if (sprite->para2 > 65536*2) {
        sprite->pos.x -= mkb::math_sin(sprite->para2)*100;
    }

    if (sprite->para2 > (65536*2)+16384) {
        mkb::destroy_sprite_with_unique_id(0x420);
        mkb::destroy_sprite_with_unique_id(0x421);
        return;
    }

    mkb::init_ui_box_sprite_with_defaults();
    mkb::set_ui_box_sprite_pos(sprite->pos.x+10, 100.0);
    mkb::set_ui_box_sprite_scale(0.6, 0.5);
    mkb::draw_ui_box(0x5);

    mkb::init_global_font_sprite_vars_with_defaults();
    mkb::set_global_font_sprite_type(sprite->font);
    mkb::set_global_font_sprite_alignment(sprite->alignment);
    mkb::set_global_font_sprite_flags(0x80000000);
    mkb::set_global_font_sprite_dimensions(0.60, 0.75);
    mkb::g_set_global_font_style(mkb::STYLE_BOLD_ITALIC);
    mkb::g_set_global_font_sprite_drop_shadow_flag();
    mkb::g_set_global_font_sprite_mult_color(0x00ffff00);
    mkb::set_global_font_sprite_add_color(flash_color);
    mkb::set_global_font_sprite_pos(sprite->pos.x+10, 75.0);
    mkb::draw_text_sprite_string((mkb::byte*)msg);

    mkb::init_global_font_sprite_vars_with_defaults();
    mkb::set_global_font_sprite_type(sprite->font);
    mkb::set_global_font_sprite_alignment(sprite->alignment);
    mkb::set_global_font_sprite_flags(0x80000000);
    mkb::set_global_font_sprite_dimensions(0.60, 0.75);
    mkb::g_set_global_font_sprite_drop_shadow_flag();
    mkb::g_set_global_font_sprite_mult_color(0x00ffff00);
    mkb::set_global_font_sprite_pos(sprite->pos.x+10, 100.0);
    mkb::draw_text_sprite_string((mkb::byte*)msg1);

    mkb::init_global_font_sprite_vars_with_defaults();
    mkb::set_global_font_sprite_type(sprite->font);
    mkb::set_global_font_sprite_alignment(sprite->alignment);
    mkb::set_global_font_sprite_flags(0x80000000);
    mkb::set_global_font_sprite_dimensions(0.45, 0.60);
    mkb::g_set_global_font_sprite_drop_shadow_flag();
    mkb::g_set_global_font_sprite_mult_color(0x00ffff00);
    mkb::set_global_font_sprite_pos(sprite->pos.x+10, 125.0);
    mkb::draw_text_sprite_string((mkb::byte*)msg2);

    if (sprite_banana != nullptr) {
        sprite_banana->pos.x = sprite->pos.x-60;
    }
}

*/
void init() {
}

void tick() {
    if (pad::button_down(mkb::PAD_BUTTON_X)) {
        mkb::OSReport("playing sound %d\n", mystery);
        mkb::call_SoundReqID_arg_1(mystery);
        mystery += 1;
    }

    if (pad::button_down(mkb::PAD_TRIGGER_L)) {
        mkb::OSReport("x: %f\n", mystery);
    }

    if (pad::button_down(mkb::PAD_TRIGGER_Z) && !sent) {
        //mkb::call_SoundReqID_arg_2(10);
        mkb::OSReport("free: %x\n", heap::get_free_space());

        sent = true;
        sent2 = false;
    }

    if (pad::button_down(mkb::PAD_BUTTON_Y) && !sent2) {
        sent = false;
        sent2 = true;
    }
    if (sent) mystery_2++;
}

void disp() {}

}
