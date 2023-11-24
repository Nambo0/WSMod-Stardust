#include "galactic_log.h"

#include "../stardust/savedata.h"
#include "../stardust/unlock.h"
#include "internal/heap.h"
#include "internal/log.h"
#include "internal/patch.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_button.h"
#include "internal/ui/widget_menu.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "widget_input.h"
#include "widget_sprite.h"

namespace galactic_log {

TICKABLE_DEFINITION((
        .name = "galactic-log",
        .description = "Galactic log",
        .enabled = true,
        .init_main_loop = init_main_loop,
        .init_main_game = init_main_game,
        .tick = tick, ))

static patch::Tramp<decltype(&mkb::create_how_to_sprite)> s_g_create_how_to_sprite_tramp;
static char s_badge_stage_name_buffer[10][64];
static char s_text_page_buffer[1024] = {0};
static uint8_t s_log_page_number = 0;// Current index of page in log screen
static uint8_t s_log_page_count = 0; // Number of pages in a log screen

// All relevant pages of text here
namespace {
constexpr char* s_log_pages_about[8] = {
    // About Page 1
    "/bc00fffb/BADGES/bcFFFFFF/\n"
    "\n"
    "Each Story Mode stage has 3 objectives tracked in the Story Mode\n"
    "section of the Galactic Log, each one represented by a circular badge.\n"
    "\n"
    "/bc009DFF/Clear Badge:/bcFFFFFF/ Complete that stage's normal goal\n"
    "/bcC800FF/Stunt Badge:/bcFFFFFF/ Complete that stage's stunt goal\n"
    "/bcFBFF00/Sweep Badge:/bcFFFFFF/ Complete that stage with every bunch collected in one run\n"
    "\n"
    "Whenever you collect a new badge, its icon will briefly appear\n"
    "in the bottom left corner above the stage name\n",

    // About Page 2
    "/bc00fffb/COSMIC BUNCHES/bcFFFFFF/\n"
    "\n"
    "Various types of /bcFBFF00/banana bunches/bcFFFFFF/ can be found in Story Mode.\n"
    "Cosmic bunches are larger, with small colored comets that\n"
    "orbit around them\n"
    "\n"
    "/bcFBFF00/Normal Bunches/bcFFFFFF/ award /bcFBFF00/10/bcFFFFFF/ bananas\n"
    "/bc00FF2F/Green Cosmic Bunches/bcFFFFFF/ award /bc00FF2F/20/bcFFFFFF/ bananas\n"
    "/bc009DFF/Blue Cosmic Bunches/bcFFFFFF/ award /bc009DFF/30/bcFFFFFF/ bananas\n"
    "/bcC800FF/Purple Cosmic Bunches/bcFFFFFF/ award /bcC800FF/50/bcFFFFFF/ bananas\n"
    "\n"
    "Collecting every single bunch on a stage will award\n"
    "that stage's /bcFBFF00/Sweep Badge/bcFFFFFF/.\n"
    "\n"
    "Collecting a bunch will reveal a /bcFBFF00/counter/bcFFFFFF/ under the stage\n"
    "name to help keep track of how many are left on the stage\n",

    // About Page 3 (ONLY SHOW IF BONUS LOCKED)
    "/bc00fffb/UNLOCKING BONUS MODES/bcFFFFFF/\n"
    "\n"
    "Unlock all bonus modes by claiming the following achievements:     \n"
    "\n"
    "/bcFF9900/BEAT THE GAME:/bcFFFFFF/ Complete Story Mode\n"
    "/bcFF9900/STUNT PILOT:/bcFFFFFF/ Complete 1 /bcC800FF/Stunt Goal/bcFFFFFF/ in each world\n"
    "\n"
    "The list of all /bcFF9900/achievements/bcFFFFFF/ can be found in that section\n"
    "of the Galactic log.\n",

    // About Page 4 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/INTERSTELLAR/bcFFFFFF/\n"
    "\n"
    "Interstellar is a bonus mode with 10 massive bonus stages\n"
    "played back-to-back. You have 300 seconds to collect as\n"
    "many /bcFBFF00/bananas/bcFFFFFF/ as possible on each stage.\n"
    "High score runs can be viewed in the Interstellar section\n"
    "of the Galactic log.\n"
    "\n"
    "/bcC800FF/Penalties & Bonuses:/bcFFFFFF/\n"
    "\n"
    "Falling out does NOT reset the timer or bunches collected.\n"
    "Instead, there is a /bcFF9900/15 second penalty/bcFFFFFF/ each time you fall out.\n"
    "\n"
    "Each stage has a goal which will award /bcFBFF00/50 bananas/bcFFFFFF/ upon\n"
    "completion. The goal will bring you to the next stage, so\n"
    "try to spend as much time as possible collecting bunches!\n",

    // About Page 5 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/INTERSTELLAR/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/Ranks:/bcFFFFFF/\n"
    "\n"
    "There are 5 ranks based on the following milestones:\n"
    "/bcB68E00/BRONZE RANK:/bcFFFFFF/ 1000+ bananas\n"
    "/bcCCCCCC/SILVER RANK:/bcFFFFFF/ 2000+ bananas\n"
    "/bcFFDD00/GOLD RANK:/bcFFFFFF/ 3000+ bananas\n"
    "/bc6EFFFD/PLATIMUM RANK:/bcFFFFFF/ 4000+ bananas\n"
    "/bcC800FF/STAR RANK:/bcFFFFFF/ 5000+ bananas\n"
    "\n"
    "Each rank will display a unique /bcC800FF/medallion/bcFFFFFF/ at the end of\n"
    "the run, and your best run's medallion will be displayed\n"
    "in the center of /bc009DFF/Monuments/bcFFFFFF/.\n",

    // About Page 6 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/DEBUG/bcFFFFFF/\n"
    "\n"
    "Debug is a bonus mode made up of drafts, prototypes,\n"
    "and joke stages. It's split up into 4 sub-sections:\n"
    "\n"
    "/bc00FF2F/Joke Stages:/bcFFFFFF/ Joke/troll stages that I made for fun\n"
    "/bc009DFF/Stage Drafts:/bcFFFFFF/ Old revisions of stages that got re-made,\n"
    "                  and concepts that didn't become stages\n"
    "/bcC800FF/The Unplayable Zone:/bcFFFFFF/ Obnoxious/impossible joke stages\n"
    "\n"
    "PLEASE NOTE: This is meant to be a silly showcase of\n"
    "the inner workings of this pack's development. I'm fully\n"
    "aware these stages are bad. /bc00FF2F/That's why they are here.\n"
    "\n"
    "/bcA1A1A1/* Stages can be skipped by holding B for 3 seconds./bcFFFFFF/\n",

    // About Page 7 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/MONUMENTS/bcFFFFFF/\n"
    "\n"
    "Monuments is a /bcC800FF/trophy room/bcFFFFFF/ for this pack's bonus goals!\n"
    "\n"
    "The surrounding statues each represent one world, and\n"
    "above you can see which /bcC800FF/badges/bcFFFFFF/ you have completed.\n"
    "The world's Stage Challenge Achievement is marked as\n"
    "an /bcFF9900/Achievement Icon/bcFFFFFF/ above those 3 rows.\n"
    "\n"
    "If you get all 10 /bcC800FF/stunt/bcFFFFFF/ or /bcFBFF00/sweep/bcFFFFFF/ badges in any world,\n"
    "its statue will be decorated with a /bc009DFF/special effect/bcFFFFFF/!\n"
    "\n"
    "If you get all 100 /bcC800FF/stunt/bcFFFFFF/ or /bcFBFF00/sweep/bcFFFFFF/ badges, something\n"
    "special will appear on the /bc009DFF/giant empty pedestals/bcFFFFFF/\n"
    "outside the circle!\n",

    // About Page 8 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/MONUMENTS/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/Continued:/bcFFFFFF/\n"
    "\n"
    "The center of the stage has a spinning /bc009DFF/medallion/bcFFFFFF/,\n"
    "representing your best Interstellar rank!\n"};
constexpr char* s_log_pages_credits[2] = {
    "/bcFFFFFF/NOTE: Credits.pdf (included with the ISO) has more     \n"
    "annotated credits, including clickable /bc008CFF/links/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/DIRECT CONTRIBUTIONS:/bcFFFFFF/\n"
    "\n"
    "/bc00fffb/Original Soundtrack/bcFFFFFF/\n"
    "      (>^^)> Walkr (/bc008CFF/linktr.ee/walkrmusic/bcFFFFFF/)\n"
    "      (>^^)> Relayer (/bc008CFF/wxokeys.bandcamp.com/bcFFFFFF/)\n"
    "/bc00fffb/Custom Code/bcFFFFFF/\n"
    "      (>^^)> Rehtrop, Bombsquad, Eucalyptus\n"
    "/bc00fffb/Art/bcFFFFFF/\n"
    "      (>^^)> Shadow (/bc008CFF/charredshadow.tumblr.com/bcFFFFFF/)\n"
    "/bc00fffb/Playtesters/bcFFFFFF/\n"
    "      (>^^)> Eddy, 42guy42, Rehtrop, Null\n"
    "      (>^^)> Walkr, Goobz, Dyrude, Eucalyptus\n",
    "/bcFFFFFF/NOTE: Credits.pdf (included with the ISO) has more     \n"
    "annotated credits, including clickable /bc008CFF/links/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/SPECIAL THANKS:/bcFFFFFF/\n"
    "\n"
    "/bc00fffb/Specific Mentions/bcFFFFFF/\n"
    "      (>^^)> Zona, Ghost Ham, Sudachi\n"
    "      (>^^)> PetresInc, Dwaitley, scrap651\n"
    "      (>^^)> Petra, Yhouse, Jesse, Ariana, Shadow\n"
    "/bc00fffb/Broad Shout-Outs/bcFFFFFF/\n"
    "      (>^^)> Everyone listed on page 1\n"
    "      (>^^)> Friends from Monkey Ball speedrunning\n"
    "      (>^^)> Friends from Random Randos\n"
    "      (>^^)> My family & irl friends\n"
    "      (>^^)> YOU!\n"};
constexpr char* s_stellar_ranks[6] = {
    "NONE",
    "/bcB68E00/BRONZE/bcFFFFFF/",
    "/bcCCCCCC/SILVER/bcFFFFFF/",
    "/bcFFDD00/GOLD/bcFFFFFF/",
    "/bc6EFFFD/PLATIMUM/bcFFFFFF/",
    "/bcC800FF/STAR/bcFFFFFF/"};
constexpr char* s_log_pages_interstellar =
    "/bc00fffb/BEST RUN/bcFFFFFF/\n"
    "\n"
    "/bcFFFFFF/Grand Total: /bcFBFF00/%d/bcFFFFFF/\n"
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
    "World 10: /bcFBFF00/%d/bcFFFFFF/\n";
}// namespace

void create_galactic_log_menu() {
    constexpr Vec2d center = Vec2d{640 / 2, 480 / 2};
    constexpr Vec2d box_size = Vec2d{450, 220};
    constexpr Vec2d box_origin = Vec2d{center.x - (box_size.x / 2), center.y - (box_size.y) / 2};

    // The menu for the Galactic Log
    ui::Menu& galactic_log_menu = ui::get_widget_manager().add(new ui::Menu(box_origin, box_size));
    galactic_log_menu.set_label("galmenu");
    galactic_log_menu.set_alignment(mkb::ALIGN_UPPER_CENTER);
    galactic_log_menu.set_depth(0.002);

    // The header text
    // Hack for making these children widgets appear above the pause menu screen overlay
    galactic_log_menu.set_depth(0.0055);
    ui::Text& galactic_log_title = galactic_log_menu.add(new ui::Text("Galactic Log"));
    galactic_log_title.set_font_style(mkb::STYLE_TEGAKI);
    galactic_log_menu.set_depth(0.002);

    // Handler for the 'About' button
    auto open_about_handler = [&]() {
        // TODO: preserve selected menu state, so we can return back to it
        ui::get_widget_manager().remove("galmenu");
        create_about_screen();
    };

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

    auto open_interstellar_handler = []() {
        ui::get_widget_manager().remove("galmenu");
        create_interstellar_screen();
    };

    // Placeholder handle... does nothing
    auto placeholder_handler = []() {};

    // Handle for 'Close' button
    auto close_handler = []() {
        ui::get_widget_manager().remove("galmenu");
        LOG("After closing free heap: %dkb", heap::get_free_space() / 1024);
        // TODO: go back to the pause menu?
    };

    // Hack for making these children widgets appear above the pause menu screen overlay
    galactic_log_menu.set_depth(0.0055);
    galactic_log_menu.add(new ui::Button("Story Mode", open_badge_handler));
    galactic_log_menu.add(new ui::Button("Interstellar", open_interstellar_handler));
    galactic_log_menu.add(new ui::Button("Achievements", placeholder_handler));
    galactic_log_menu.add(new ui::Button("About", open_about_handler));
    galactic_log_menu.add(new ui::Button("Credit & Special Thanks", open_credits_handler));
    galactic_log_menu.add(new ui::Button("Close", close_handler));
    galactic_log_menu.set_depth(0.002);
}


void create_about_screen() {
    LOG("Creating about screen...");
    mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

    // Initialize the correct page count/page index
    s_log_page_number = 0;
    s_log_page_count = 8;// TODO: hook into bonus locked/unlocked status

    // Parent widget, this is the darkened screen
    auto& about_menu_screen = ui::get_widget_manager().add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
    about_menu_screen.set_label("galcred");
    about_menu_screen.set_scale(Vec2d{300, 200});
    about_menu_screen.set_alpha(0.6666f);
    about_menu_screen.set_mult_color({0x00, 0x00, 0x00});// black
    about_menu_screen.set_depth(0.02);

    // Header container
    auto& about_menu_header_container = about_menu_screen.add(new ui::Container(Vec2d{0, 0}, Vec2d{640, 128}));
    about_menu_header_container.set_margin(0);
    about_menu_header_container.set_layout_spacing(64);
    about_menu_header_container.set_layout(ui::ContainerLayout::HORIZONTAL);

    // Back arrow
    about_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));

    // Title box
    auto& title_box = about_menu_header_container.add(new ui::Window(Vec2d{0, 0}, Vec2d{384, 64}));
    title_box.set_alignment(mkb::ALIGN_CENTER);

    auto& title_text = title_box.add(new ui::Text("About"));
    title_text.set_alignment(ui::CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    // Next arrow
    auto& next_arrow = about_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
    next_arrow.set_mirror(true);

    auto& about_container = about_menu_screen.add(new ui::Container(Vec2d{0, 65}, Vec2d{640, 480 - 65 - 5}));
    mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[s_log_page_number]);
    auto& about_text = about_container.add(new ui::Text(s_text_page_buffer));
    about_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    about_text.set_alignment(ui::LEFT);
    about_text.set_drop_shadow(false);
    about_text.set_color({0x00, 0x00, 0x00});

    auto close_about = [&]() {
        ui::get_widget_manager().remove("galcred");
        create_galactic_log_menu();
    };

    auto decrement_page_about = []() {
        // UNLOCKED: Skip page 3
        // LOCKED: End on page 3
        if (unlock::unlock_condition_met()) {
            if (s_log_page_number == 0) {
                s_log_page_number = s_log_page_count - 1;
            }
            else {
                --s_log_page_number;
            }
            if (s_log_page_number == 2) s_log_page_number = 1;// Skip page 3
        }
        else {
            if (s_log_page_number == 0) {
                s_log_page_number = 2;// Wraparound to page 3
            }
            else {
                --s_log_page_number;
            }
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[s_log_page_number]);
    };

    auto increment_page_about = []() {
        // UNLOCKED: Skip page 3
        // LOCKED: End on page 3
        if (unlock::unlock_condition_met()) {
            if (s_log_page_number + 1 >= s_log_page_count) {
                s_log_page_number = 0;
            }
            else {
                ++s_log_page_number;
            }
            if (s_log_page_number == 2) s_log_page_number = 3;// Skip page 3
        }
        else {
            if (s_log_page_number + 1 >= 3) {// End on page 3
                s_log_page_number = 0;
            }
            else {
                ++s_log_page_number;
            }
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[s_log_page_number]);
    };

    // Close handler
    about_menu_screen.add(new ui::Input(mkb::PAD_BUTTON_B, close_about));

    auto& previous_page_handler = about_menu_screen.add(new ui::Input(pad::DIR_LEFT, decrement_page_about));
    previous_page_handler.set_sound_effect_id(0x6f);

    auto& next_page_handler = about_menu_screen.add(new ui::Input(pad::DIR_RIGHT, increment_page_about));
    next_page_handler.set_sound_effect_id(0x6f);
}

void create_credits_screen() {
    LOG("Creating credits screen...");
    mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

    // Initialize the correct page count/page index
    s_log_page_number = 0;
    s_log_page_count = 2;

    // Parent widget, this is the darkened screen
    auto& credits_menu_screen = ui::get_widget_manager().add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
    credits_menu_screen.set_label("galcred");
    credits_menu_screen.set_scale(Vec2d{300, 200});
    credits_menu_screen.set_alpha(0.6666f);
    credits_menu_screen.set_mult_color({0x00, 0x00, 0x00});// black
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
    title_text.set_alignment(ui::CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    // Next arrow
    auto& next_arrow = credits_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
    next_arrow.set_mirror(true);

    // Credits Page 1
    auto& credits_container = credits_menu_screen.add(new ui::Container(Vec2d{5, 65}, Vec2d{640 - 5, 480 - 65 - 5}));
    mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_credits[s_log_page_number]);
    auto& credits_text = credits_container.add(new ui::Text(s_text_page_buffer));
    credits_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    credits_text.set_alignment(mkb::ALIGN_LOWER_RIGHT);
    credits_text.set_drop_shadow(false);
    credits_text.set_color({0x00, 0x00, 0x00});

    auto close_credits = [&]() {
        ui::get_widget_manager().remove("galcred");
        create_galactic_log_menu();
    };

    auto& close_handler = credits_menu_screen.add(new ui::Button("", Vec2d{0, 0}, close_credits));// TODO: generic input handler widget
    close_handler.set_active(true);
    close_handler.set_input(mkb::PAD_BUTTON_B);

    auto decrement_page_credits = []() {
        if (s_log_page_number == 0) {
            s_log_page_number = s_log_page_count - 1;
        }
        else {
            --s_log_page_number;
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_credits[s_log_page_number]);
    };

    auto increment_page_credits = []() {
        if (s_log_page_number + 1 >= s_log_page_count) {
            s_log_page_number = 0;
        }
        else {
            ++s_log_page_number;
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_credits[s_log_page_number]);
    };
    auto& previous_page_handler = credits_menu_screen.add(new ui::Input(pad::DIR_LEFT, decrement_page_credits));
    previous_page_handler.set_sound_effect_id(0x6f);

    auto& next_page_handler = credits_menu_screen.add(new ui::Input(pad::DIR_RIGHT, increment_page_credits));
    next_page_handler.set_sound_effect_id(0x6f);
}

void create_badge_list() {
    auto& badge_menu_screen = ui::get_widget_manager().find("galbadg");

    auto& badge_container = badge_menu_screen.add(new ui::Container(Vec2d{0, 65}, Vec2d{640, 480 - 65}));
    badge_container.set_label("galbdgc");
    badge_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    for (uint32_t stage_idx = 0; stage_idx < 10; stage_idx++) {
        auto& layout_row = badge_container.add(new ui::Container(Vec2d{0, 0}, Vec2d{630, 32}));
        layout_row.set_margin(0);
        layout_row.set_layout_spacing(0);
        layout_row.set_layout(ui::ContainerLayout::HORIZONTAL);
        auto& text_container = layout_row.add(new ui::Container(Vec2d{0, 0}, Vec2d{470, 32}));
        auto& sprite_container = layout_row.add(new ui::Container(Vec2d{0, 0}, Vec2d{160, 32}));
        sprite_container.set_layout(ui::ContainerLayout::HORIZONTAL);

        uint32_t stage_id = mkb::get_story_mode_stage_id(s_log_page_number, stage_idx);
        LOG("Got id %d", stage_id);
        char stage_name_buffer[64] = {0};
        mkb::read_stage_name_from_dvd(stage_id, stage_name_buffer, 64);
        LOG("Got name %s", stage_name_buffer)
        mkb::sprintf(s_badge_stage_name_buffer[stage_idx], "%d-%d %s", s_log_page_number + 1, stage_idx + 1, stage_name_buffer);
        LOG("Did sprintf to yield: %s", s_badge_stage_name_buffer[stage_idx])
        auto& text = text_container.add(new ui::Text(s_badge_stage_name_buffer[stage_idx]));

        // 0xc3b = blue, 0xc3a = purple, 0xc39 = sweep, 0xc3c = achievement, 0xc3d = empty
        uint32_t id_1 = 0xc3d;
        uint32_t id_2 = 0xc3d;
        uint32_t id_3 = 0xc3d;
        if (savedata::true_in_slot(savedata::CLEAR_BADGE_START + s_log_page_number * 10 + stage_idx)) id_1 = 0xc3b;
        if (savedata::true_in_slot(savedata::STUNT_BADGE_START + s_log_page_number * 10 + stage_idx)) id_2 = 0xc3a;
        if (savedata::true_in_slot(savedata::SWEEP_BADGE_START + s_log_page_number * 10 + stage_idx)) id_3 = 0xc39;
        auto& blue = sprite_container.add(new ui::Sprite(id_1, Vec2d{32, 32}));
        auto& purple = sprite_container.add(new ui::Sprite(id_2, Vec2d{32, 32}));
        auto& sweep = sprite_container.add(new ui::Sprite(id_3, Vec2d{32, 32}));

        blue.set_scale(Vec2d{0.5, 0.5});
        purple.set_scale(Vec2d{0.5, 0.5});
        sweep.set_scale(Vec2d{0.5, 0.5});

        text.set_alignment(ui::LEFT);
        text.set_drop_shadow(false);// temporary
    }
}
void create_badge_screen() {
    LOG("Creating badge screen...");
    mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

    s_log_page_number = 0;
    s_log_page_count = 10;

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
    title_text.set_alignment(ui::CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    // Next arrow
    auto& next_arrow = badge_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
    next_arrow.set_mirror(true);


    LOG("Creating stage name list...");
    create_badge_list();

    auto close_badge = [&]() {
        ui::get_widget_manager().remove("galbadg");
        create_galactic_log_menu();
    };

    // Only display up to the highest unlocked world (using story cutscene unlock info)
    s_log_page_count = 1;
    for (u8 world = 1; world < 10; world++) {
        if (!savedata::consecutive_false_from_slot(savedata::CLEAR_BADGE_START + 10 * world, 10) || !savedata::consecutive_false_from_slot(savedata::STUNT_BADGE_START + 10 * world, 10)) {
            s_log_page_count = world + 1;
        }
    }

    auto decrement_page_badge = []() {
        auto& badge_menu_screen = ui::get_widget_manager().find("galbadg");
        badge_menu_screen.remove("galbdgc");
        if (s_log_page_number == 0) {
            s_log_page_number = s_log_page_count - 1;
        }
        else {
            --s_log_page_number;
        }
        create_badge_list();
    };

    auto increment_page_badge = []() {
        auto& badge_menu_screen = ui::get_widget_manager().find("galbadg");
        badge_menu_screen.remove("galbdgc");
        if (s_log_page_number + 1 >= s_log_page_count) {
            s_log_page_number = 0;
        }
        else {
            ++s_log_page_number;
        }
        create_badge_list();
    };

    auto& previous_page_handler = badge_menu_screen.add(new ui::Input(pad::DIR_LEFT, decrement_page_badge));
    previous_page_handler.set_sound_effect_id(0x6f);

    auto& next_page_handler = badge_menu_screen.add(new ui::Input(pad::DIR_RIGHT, increment_page_badge));
    next_page_handler.set_sound_effect_id(0x6f);

    // Close handler
    badge_menu_screen.add(new ui::Input(mkb::PAD_BUTTON_B, close_badge));
}

void create_interstellar_screen() {
    LOG("Creating interstellar screen...");
    mkb::load_bmp_by_id(0xc);// TODO: do not rely on this, this wastes memory

    // Parent widget, this is the darkened screen
    auto& interstellar_menu_screen = ui::get_widget_manager().add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
    interstellar_menu_screen.set_label("galints");
    interstellar_menu_screen.set_scale(Vec2d{300, 200});
    interstellar_menu_screen.set_alpha(0.6666f);
    interstellar_menu_screen.set_mult_color({0x00, 0x00, 0x00});// black
    interstellar_menu_screen.set_depth(0.02);

    // Header container
    auto& interstellar_menu_header_container = interstellar_menu_screen.add(new ui::Container(Vec2d{0, 0}, Vec2d{640, 128}));
    interstellar_menu_header_container.set_margin(0);
    interstellar_menu_header_container.set_layout_spacing(64);
    interstellar_menu_header_container.set_layout(ui::ContainerLayout::HORIZONTAL);

    // Back arrow
    interstellar_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));

    // Title box
    auto& title_box = interstellar_menu_header_container.add(new ui::Window(Vec2d{0, 0}, Vec2d{384, 64}));
    title_box.set_alignment(mkb::ALIGN_CENTER);

    auto& title_text = title_box.add(new ui::Text("Interstellar"));
    title_text.set_alignment(ui::CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    // Next arrow
    auto& next_arrow = interstellar_menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
    next_arrow.set_mirror(true);

    // Interstellar Page 1
    auto& interstellar_container = interstellar_menu_screen.add(new ui::Container(Vec2d{5, 65}, Vec2d{640 - 5, 480 - 65 - 5}));
    if (unlock::unlock_condition_met()) {// If unlocked: Display best run
        mkb::sprintf(s_text_page_buffer,
                     s_log_pages_interstellar,
                     savedata::stellar_best_run_total() * 10,
                     s_stellar_ranks[savedata::best_stellar_rank()],
                     savedata::get_stellar_level(1) * 10,
                     savedata::get_stellar_level(2) * 10,
                     savedata::get_stellar_level(3) * 10,
                     savedata::get_stellar_level(4) * 10,
                     savedata::get_stellar_level(5) * 10,
                     savedata::get_stellar_level(6) * 10,
                     savedata::get_stellar_level(7) * 10,
                     savedata::get_stellar_level(8) * 10,
                     savedata::get_stellar_level(9) * 10,
                     savedata::get_stellar_level(10) * 10);
    }
    else {// If locked: Display a copy of about page 3 (Unlock condition)
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[2]);
    }
    auto& interstellar_text = interstellar_container.add(new ui::Text(s_text_page_buffer));
    interstellar_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    interstellar_text.set_alignment(mkb::ALIGN_LOWER_RIGHT);
    interstellar_text.set_drop_shadow(false);
    interstellar_text.set_color({0x00, 0x00, 0x00});

    auto close_interstellar = []() {
        ui::get_widget_manager().remove("galints");
        create_galactic_log_menu();
    };

    // Close handler
    interstellar_menu_screen.add(new ui::Input(mkb::PAD_BUTTON_B, close_interstellar));
}

void init_main_loop() {
    patch::hook_function(s_g_create_how_to_sprite_tramp, mkb::create_how_to_sprite, [](void) {
        mkb::g_some_pausemenu_var = 4;
        mkb::Sprite* pause_sprite = mkb::get_sprite_with_unique_id(mkb::SPRITE_HOW_TO);
        if (pause_sprite != nullptr) pause_sprite->para1 = 6;
        mkb::call_SoundReqID_arg_2(10);
        LOG("Heap free before: %dkb", heap::get_free_space() / 1024);
        create_galactic_log_menu();
        mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
        LOG("Heap free after: %dkb", heap::get_free_space() / 1024);
        return;
    });
}
void init_main_game() {
}

void tick() {
}

}// namespace galactic_log
