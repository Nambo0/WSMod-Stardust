#include "galactic_log.h"

#include "internal/heap.h"
#include "internal/log.h"
#include "internal/patch.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_button.h"
#include "internal/ui/widget_menu.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "widget_sprite.h"

namespace galactic_log {

TICKABLE_DEFINITION((
        .name = "galactic-log",
        .description = "Galactic log",
        .enabled = true,
        .init_main_loop = init_main_loop,
        .init_main_game = init_main_game,
        .tick = tick, ))

static patch::Tramp<decltype(&mkb::g_create_how_to_sprite)> s_g_create_how_to_sprite_tramp;
static char badge_stage_name_buffer[10][64];
char* format_str = "%d-%d %s";

void create_galactic_log_menu() {
    constexpr Vec2d center = Vec2d{640 / 2, 480 / 2};
    constexpr Vec2d box_size = Vec2d{450, 220};
    constexpr Vec2d box_origin = Vec2d{center.x - (box_size.x / 2), center.y - (box_size.y) / 2};

    // The menu for the Galactic Log
    ui::Menu& galactic_log_menu = ui::get_widget_manager().add(new ui::Menu(box_origin, box_size));
    galactic_log_menu.set_label("galmenu");
    galactic_log_menu.set_alignment(mkb::ALIGN_UPPER_CENTER);

    // The header text
    ui::Text& galactic_log_title = galactic_log_menu.add(new ui::Text("Galactic Log"));
    galactic_log_title.set_font_style(mkb::STYLE_TEGAKI);

    // Handler for the 'Credits & Special Thanks' button
    auto open_credits_handler = [&]() {
        // TODO: preserve selected menu state, so we can return back to it
        ui::get_widget_manager().remove("galmenu");
        create_credits_screen();
    };

    auto open_badge_handler = []() {
        ui::get_widget_manager().remove("galmenu");
        create_badge_screen();
    };

    // Placeholder handle... does nothing
    auto placeholder_handler = []() {};

    // Handle for 'Close' button
    auto close_handler = []() {
        ui::get_widget_manager().remove("galmenu");
        LOG("After closing free heap: %dkb", heap::get_free_space() / 1024);
        // TODO: go back to the pause menu?
    };

    galactic_log_menu.add(new ui::Button("Story Mode", open_badge_handler));
    galactic_log_menu.add(new ui::Button("Interstellar", placeholder_handler));
    galactic_log_menu.add(new ui::Button("Achievements", placeholder_handler));
    galactic_log_menu.add(new ui::Button("About", placeholder_handler));
    galactic_log_menu.add(new ui::Button("Credit & Special Thanks", open_credits_handler));
    galactic_log_menu.add(new ui::Button("Close", close_handler));
}

void create_credits_screen() {
    LOG("Creating credits screen...");
    mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

    // Parent widget, this is the pink screen
    auto& credits_menu_screen = ui::get_widget_manager().add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
    credits_menu_screen.set_label("galcred");
    credits_menu_screen.set_scale(Vec2d{300, 200});
    credits_menu_screen.set_alpha(0.6666f);
    credits_menu_screen.set_mult_color({0xff, 0x00, 0xff});// magenta
    credits_menu_screen.set_depth(0.02);

    // Header container
    auto& credits_menu_header_container = credits_menu_screen.add(new ui::Container(Vec2d{0, 0}, Vec2d{640, 128}));
    credits_menu_header_container.set_margin(0);
    credits_menu_header_container.set_layout_spacing(64);
    credits_menu_header_container.set_layout(ui::ContainerLayout::HORIZONTAL);

    // Back arrow
    credits_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));

    // Title box
    auto& title_box = credits_menu_header_container.add(new ui::Window(Vec2d{0, 0}, Vec2d{384, 64}));
    title_box.set_alignment(mkb::ALIGN_CENTER);

    auto& title_text = title_box.add(new ui::Text("Credits & Special Thanks"));
    title_text.set_alignment(mkb::ALIGN_CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    // Next arrow
    auto& next_arrow = credits_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
    next_arrow.set_mirror(true);

    // Credits Page 1
    auto& credits_container = credits_menu_screen.add(new ui::Container(Vec2d{5, 65}, Vec2d{640 - 5, 480 - 65 - 5}));

    // Todo: pages. maybe a button with a callback that changes the active text, use sprintf to set the text perhaps?
    auto& credits_text = credits_container.add(new ui::Text(
        "/bcFFFFFF/NOTE: Anything marked in /bc008CFF/blue/bcFFFFFF/ means you can find\n"
        "a relevant link in the Credits file which came downloaded with the iso\n"
        "/bcC800FF/DIRECT CONTRIBUTIONS:/bcFFFFFF/\n"
        "/bc7E00A1/||   Original Soundtrack   ||/bcFFFFFF/\n"
        "(>^^)> Walkr_ (/bc008CFF/Hear their music/bcFFFFFF/)\n"
        "(>^^)> Relayer (/bc008CFF/Hear their music/bcFFFFFF/)\n"
        "Song Credits:\n"
        "Title/Debug - quasar 3, by Walkr\n"
        "Menu - quasar 2, by Walkr\n"
        "World 1 - Bubblegum Dream, by Relayer\n"
        "World 2 - Cloudscape, by Walkr\n"
        "World 3 - Solar Orbit, by Relayer\n"
        "World 4 - Deep Space, by Walkr\n"
        //"World 5 - Dark Generator, by Relayer\n"
        ""));

    credits_container.set_alignment(mkb::ALIGN_UPPER_CENTER);
    credits_text.set_drop_shadow(false);
    credits_text.set_color({0x00, 0x00, 0x00});

    auto close_credits = [&]() {
        ui::get_widget_manager().remove("galcred");
        create_galactic_log_menu();
    };

    auto& close_handler = credits_menu_screen.add(new ui::Button("", Vec2d{0, 0}, close_credits));// TODO: generic input handler widget
    close_handler.set_active(true);
    close_handler.set_input(mkb::PAD_BUTTON_B);
}

void create_badge_screen() {
    LOG("Creating badge screen...");
    mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

    // Parent widget, this is the pink screen
    auto& badge_menu_screen = ui::get_widget_manager().add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
    badge_menu_screen.set_label("galbadg");
    badge_menu_screen.set_scale(Vec2d{300, 200});
    badge_menu_screen.set_alpha(0.6666f);
    badge_menu_screen.set_mult_color({0xff, 0x00, 0xff});// magenta
    badge_menu_screen.set_depth(0.02);

    // Header container
    auto& badge_menu_header_container = badge_menu_screen.add(new ui::Container(Vec2d{0, 0}, Vec2d{640, 128}));
    badge_menu_header_container.set_margin(0);
    badge_menu_header_container.set_layout_spacing(64);
    badge_menu_header_container.set_layout(ui::ContainerLayout::HORIZONTAL);

    // Back arrow
    badge_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));

    // Title box
    auto& title_box = badge_menu_header_container.add(new ui::Window(Vec2d{0, 0}, Vec2d{384, 64}));
    title_box.set_alignment(mkb::ALIGN_CENTER);

    auto& title_text = title_box.add(new ui::Text("Story Mode"));
    title_text.set_alignment(mkb::ALIGN_CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    // Next arrow
    auto& next_arrow = badge_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
    next_arrow.set_mirror(true);

    // Credits Page 1
    auto& badge_container = badge_menu_screen.add(new ui::Container(Vec2d{5, 65}, Vec2d{640 - 5, 480 - 65 - 5}));

    // Todo: pages. maybe a button with a callback that changes the active text, use sprintf to set the text perhaps?
    LOG("Creating stage name list...");
    uint32_t active_world_idx = 0;
    for (uint32_t stage_idx = 0; stage_idx < 10; stage_idx++) {
        auto& layout_row = badge_container.add(new ui::Container(Vec2d{0, 0}, Vec2d{630, 32}));
        layout_row.set_layout(ui::ContainerLayout::HORIZONTAL);
        auto& text_container = layout_row.add(new ui::Container(Vec2d{0, 0}, Vec2d{420, 32}));
        auto& sprite_container = layout_row.add(new ui::Container(Vec2d{0, 0}, Vec2d{210, 32}));
        sprite_container.set_layout(ui::ContainerLayout::HORIZONTAL);

        uint32_t stage_id = mkb::get_story_mode_stage_id(active_world_idx, stage_idx);
        LOG("Got id %d", stage_id);
        char stage_name_buffer[64] = {0};
        mkb::read_stage_name_from_dvd(stage_id, stage_name_buffer, 64);
        LOG("Got name %s", stage_name_buffer)
        mkb::sprintf(badge_stage_name_buffer[stage_idx], format_str, active_world_idx + 1, stage_idx + 1, stage_name_buffer);
        LOG("Did sprintf to yield: %s", badge_stage_name_buffer[stage_idx])
        auto& text = text_container.add(new ui::Text(badge_stage_name_buffer[stage_idx]));

        // 0xc3b = blue, 0xc3a = purple, 0xc39 = sweep, 0xc3c = achievement, 0xc3d = empty
        // TODO: Hook into badge system!
        uint32_t id_1 = (mkb::rand() % 2) ? 0xc3b : 0xc3d;
        uint32_t id_2 = (mkb::rand() % 2) ? 0xc3a : 0xc3d;
        uint32_t id_3 = (mkb::rand() % 2) ? 0xc39 : 0xc3d;
        auto& blue = sprite_container.add(new ui::Sprite(id_1, Vec2d{32, 32}));
        auto& purple = sprite_container.add(new ui::Sprite(id_2, Vec2d{32, 32}));
        auto& sweep = sprite_container.add(new ui::Sprite(id_3, Vec2d{32, 32}));

        blue.set_scale(Vec2d{0.5, 0.5});
        purple.set_scale(Vec2d{0.5, 0.5});
        sweep.set_scale(Vec2d{0.5, 0.5});

        text.set_alignment(mkb::ALIGN_LOWER_RIGHT);
    }

    badge_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    /*
    badge_text.set_drop_shadow(true);
    badge_text.set_color({0x00, 0x00, 0x00});
     */

    auto close_badge = [&]() {
        ui::get_widget_manager().remove("galbadg");
        create_galactic_log_menu();
    };

    auto& close_handler = badge_menu_screen.add(new ui::Button("", Vec2d{0, 0}, close_badge));// TODO: generic input handler widget
    close_handler.set_active(true);
    close_handler.set_input(mkb::PAD_BUTTON_B);
}

void init_main_loop() {
    patch::hook_function(s_g_create_how_to_sprite_tramp, mkb::g_create_how_to_sprite, [](void) {
        mkb::call_SoundReqID_arg_2(10);
        LOG("Heap free before: %dkb", heap::get_free_space() / 1024);
        create_galactic_log_menu();
        LOG("Heap free after: %dkb", heap::get_free_space() / 1024);
        return;
    });
}
void init_main_game() {
}

void tick() {
}

}// namespace galactic_log
