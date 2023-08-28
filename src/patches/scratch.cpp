#include "scratch.h"
#include "internal/heap.h"
#include "internal/log.h"
#include "internal/pad.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_container.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "widget_button.h"
#include "widget_sprite.h"

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
    if (pad::button_pressed(mkb::PAD_TRIGGER_Z) && !sent) {
        mkb::call_SoundReqID_arg_2(10);
        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);

        constexpr Vec2d center = Vec2d{640 / 2, 480 / 2};
        constexpr Vec2d box_size = Vec2d{450, 220};
        constexpr Vec2d computed_center = Vec2d{center.x - (box_size.x / 2), center.y - (box_size.y) / 2};

        auto& ui = ui::get_widget_manager();
        ui::Window& galactic_log_menu = ui.add(new ui::Window(computed_center, box_size));
        ui::Container& galactic_log_container = galactic_log_menu.add(new ui::Container(computed_center, Vec2d{box_size.x - 5, box_size.y}));

        auto hello_world = []() {};
        auto close = []() {
            for (auto& w : ui::get_widget_manager().get_widgets()) {
                w->set_visible(false);
        } };

        ui::Text& galatic_log_title = galactic_log_container.add(new ui::Text("Galactic Log", galactic_log_container.get_dimensions()));
        galatic_log_title.set_font_style(mkb::STYLE_TEGAKI);

        // Credits screen test
        mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

        auto& credits_menu_screen = ui.add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
        credits_menu_screen.set_scale(Vec2d{300, 200});
        credits_menu_screen.set_alpha(0.6666f);
        credits_menu_screen.set_mult_color({0xff, 0x00, 0xff});// magenta
        credits_menu_screen.set_depth(0.02);

        auto& title_box = credits_menu_screen.add(new ui::Window(Vec2d{128, 8}, Vec2d{384, 64}));
        auto& title_text_container = title_box.add(new ui::Container(title_box.get_pos(), title_box.get_dimensions()));// TODO: make windows containers so we don't need to do this
        auto& title_text = title_text_container.add(new ui::Text("Credits & Special Thanks", title_box.get_dimensions()));
        title_text.set_font_style(mkb::STYLE_TEGAKI);

        // Back arrow
        credits_menu_screen.add(new ui::Sprite(0xc27, Vec2d{48, 36}, Vec2d{64, 64}));

        // Next arrow
        auto& next_arrow = credits_menu_screen.add(new ui::Sprite(0xc27, Vec2d{640 - 48, 36}, Vec2d{64, 64}));
        next_arrow.set_mirror(true);

        auto& credits_container = credits_menu_screen.add(new ui::Container(Vec2d{16 - (304), 60}, Vec2d{640 - 32, 480 - 80 - 32}));
        auto& credits_text = credits_container.add(new ui::Text(
            "It's the credits & special thanks page! Here are the credits and the special thanks.\n"
            "...no credits list yet.\n"
            "But here's some indented stuff.\n"
            "    - Indented stuff here\n"
            "Did you know that text containers automatically scale text horizontally/vertically\n"
            "if the text is larger than the container's dimensions?\n",
            credits_container.get_dimensions()));

        credits_text.set_alignment(mkb::ALIGN_UPPER_RIGHT);
        credits_text.set_drop_shadow(false);
        credits_text.set_color({0x00, 0x00, 0x00});

        auto close_credits = [&credits_menu_screen]() {
            auto& w = ui::get_widget_manager().get_widgets();
            w.back()->set_visible(false);// TODO: hack. need to fix this
            w.front()->set_visible(true);
        };

        auto& close_handler = credits_menu_screen.add(new ui::Button("", Vec2d{0, 0}, close_credits));// TODO: generic input handler widget
        close_handler.set_active(true);
        close_handler.set_input(mkb::PAD_BUTTON_B);

        credits_menu_screen.set_visible(false);// not visible by default, needs to be opened

        // Back to menu test
        auto open_credits = [&]() {
            auto& w = ui::get_widget_manager().get_widgets();
            w.back()->set_visible(true);// TODO: hack. need to fix this
            w.front()->set_visible(false);
        };

        galactic_log_container.add(new ui::Button("Story Mode", galactic_log_container.get_dimensions(), hello_world));
        galactic_log_container.add(new ui::Button("Interstellar", galactic_log_container.get_dimensions(), hello_world));
        galactic_log_container.add(new ui::Button("Achievements", galactic_log_container.get_dimensions(), hello_world));
        galactic_log_container.add(new ui::Button("About", galactic_log_container.get_dimensions(), hello_world));
        galactic_log_container.add(new ui::Button("Credit & Special Thanks", galactic_log_container.get_dimensions(), open_credits));
        galactic_log_container.add(new ui::Button("Close", galactic_log_container.get_dimensions(), close));

        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);
        sent = true;
    }

    if (pad::button_pressed(mkb::PAD_BUTTON_Y) && !sent2) {
        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);
        ui::get_widget_manager().clear();
        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);
        sent = false;
    }

    /*
    if (base_ptr) {
        base_ptr->tick();
    }*/
}

void disp() {}

}// namespace scratch
