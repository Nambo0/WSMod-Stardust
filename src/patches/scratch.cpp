#include "scratch.h"
#include "internal/heap.h"
#include "internal/pad.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_container.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "widget_button.h"

namespace scratch {

TICKABLE_DEFINITION((
        .name = "scratch",
        .description = "Scratch",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick))

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
    if (pad::button_down(mkb::PAD_TRIGGER_Z) && !sent) {
        mkb::call_SoundReqID_arg_2(10);
        mkb::OSReport("free: %d\n", heap::get_free_space());

        auto& mgr_ref = ui::get_widget_manager();

        constexpr Vec2d center = Vec2d{640/2, 480/2};
        constexpr Vec2d box_size = Vec2d{300, 220};
        constexpr Vec2d computed_center = Vec2d{center.x-(box_size.x/2), center.y-(box_size.y)/2};
        auto window = etl::unique_ptr<ui::Window>(new ui::Window(computed_center, box_size));
        auto container_txt = etl::unique_ptr<ui::Container>(new ui::Container(computed_center, box_size));
        auto container = etl::unique_ptr<ui::Container>(new ui::Container(computed_center, box_size));

        auto hello_world = [](){ mkb::call_SoundReqID_arg_2(0x6e); };
        auto close = [&window](){ mkb::call_SoundReqID_arg_2(0x6e);
            for (auto& w : ui::get_widget_manager().get_widgets()) {
                w->set_visible(false);
        } };

        auto text = etl::unique_ptr<ui::Text>(new ui::Text(Vec2d{center.x, center.y-82}, "Galactic Log"));
        text->set_m_font_style(0x5);
        text->set_scale(Vec2d{1.0, 1.0});
        auto btn1 = etl::unique_ptr<ui::Button>(new ui::Button(Vec2d{center.x, center.y-45}, "Story Mode", hello_world));
        auto btn2 = etl::unique_ptr<ui::Button>(new ui::Button(Vec2d{center.x, center.y-20}, "Interstellar", hello_world));
        auto btn3 = etl::unique_ptr<ui::Button>(new ui::Button(Vec2d{center.x, center.y+5}, "Achievements", hello_world));
        auto btn4 = etl::unique_ptr<ui::Button>(new ui::Button(Vec2d{center.x, center.y+30}, "About", hello_world));
        auto btn5 = etl::unique_ptr<ui::Button>(new ui::Button(Vec2d{center.x, center.y+55}, "Credit & Special Thanks", hello_world));
        auto btn6 = etl::unique_ptr<ui::Button>(new ui::Button(Vec2d{center.x, center.y+80}, "Close", close));

        container_txt->add_child(std::move(text));
        container->add_child(std::move(btn1));
        container->add_child(std::move(btn2));
        container->add_child(std::move(btn3));
        container->add_child(std::move(btn4));
        container->add_child(std::move(btn5));
        container->add_child(std::move(btn6));
        window->add_child(std::move(container));
        window->add_child(std::move(container_txt));
        mgr_ref.push(std::move(window));

        mkb::OSReport("free: %d\n", heap::get_free_space());
        sent = true;
    }

    if (pad::button_down(mkb::PAD_BUTTON_Y) && !sent2) {
        sent = false;
    }

    /*
    if (base_ptr) {
        base_ptr->tick();
    }*/
}

void disp() {}

}// namespace scratch
