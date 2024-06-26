#include "galactic_log.h"

#include "../internal/cardio.h"
#include "../stardust/savedata.h"
#include "../stardust/unlock.h"
#include "internal/heap.h"
#include "internal/log.h"
// #include "internal/pad.h"
#include "internal/patch.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_menu.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "utils/ppcutil.h"
#include "widget_input.h"
#include "widget_sprite.h"

namespace galactic_log {

TICKABLE_DEFINITION((
        .name = "galactic-log",
        .description = "Galactic log",
        .enabled = true,
        .init_main_loop = init_main_loop,
        .init_main_game = init_main_game,
        .init_sel_ngc = init_sel_ngc,
        .tick = tick, ))

static patch::Tramp<decltype(&mkb::create_how_to_sprite)> s_g_create_how_to_sprite_tramp;
static patch::Tramp<decltype(&mkb::check_pause_menu_input)> s_check_pause_menu_input;
static patch::Tramp<decltype(&mkb::did_any_pad_press_input)> s_did_any_pad_press_input;
static char s_badge_stage_name_buffer[10][64];
static char s_badge_stage_number_buffer[10][4];
static char s_achievement_name_buffer[7][256];
static char s_text_page_buffer[1024] = {0};
static char s_page_number_buffer[32] = {0};
static char s_memory_card_indicator_buffer[32] = {0};
static uint8_t s_log_page_number = 0;        // Current index of page in log screen
static uint8_t s_log_page_count = 0;         // Number of pages in a log screen
static uint8_t s_log_page_count_visible = 0; // Number of pages a player can actually see
static uint8_t s_log_page_number_visible = 0;// Index of page relative to number that are actually visible
static etl::optional<size_t> s_galactic_log_index;
alignas(4) static bool s_main_menu_input_lock = false;// Main menu input not processed if 'true'
static uint32_t s_input_frame_lock_buffer = 0;        // Does not return control to the game for this many frames
constexpr uint32_t INPUT_FRAME_LOCK_DELAY = 5;        // Default number of frames to delay
mkb::Rgb24 bar_colors[10] = {{0xFF, 0x00, 0x00},
                             {0x00, 0x4C, 0xFF},
                             {0xFF, 0xE5, 0x00},
                             {0x00, 0xCC, 0x00},
                             {0x00, 0xCC, 0xCC},
                             {0xFF, 0x00, 0xCC},
                             {0x19, 0x33, 0x33},
                             {0xFF, 0xFF, 0xE5},
                             {0xB2, 0x00, 0xFF},
                             {0xB2, 0xFF, 0x00}};

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
    "in the bottom left corner above the stage name.\n",

    // About Page 2
    "/bc00fffb/COSMIC BUNCHES/bcFFFFFF/\n"
    "\n"
    "Various types of /bcFBFF00/banana bunches/bcFFFFFF/ can be found in Story Mode.\n"
    "Cosmic bunches are larger, with small colored comets that\n"
    "orbit around them.\n"
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
    "name to help track how many bunches remain on the stage.\n",

    // About Page 3 (ONLY SHOW IF BONUS LOCKED)
    "/bc00fffb/UNLOCKING BONUS MODES/bcFFFFFF/\n"
    "\n"
    "Unlock all bonus modes by claiming the following achievements:\n"
    "\n"
    "/bcFF9900/BEAT THE GAME:/bcFFFFFF/ Complete Story Mode\n"
    "/bcFF9900/STUNT PILOT:/bcFFFFFF/ Complete 1 /bcC800FF/Stunt Goal/bcFFFFFF/ in each world\n"
    "\n"
    "The list of all /bcFF9900/achievements/bcFFFFFF/ can be found in that section\n"
    "of the Galactic log.\n"
    "\n"
    "After unlocking, explanations for each mode can be found\n"
    "in the About section of the Galactic log.\n",

    // About Page 4 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/INTERSTELLAR/bcFFFFFF/\n"
    "\n"
    "Interstellar is a bonus mode with 10 massive bonus stages\n"
    "played back-to-back. You have 300 seconds to collect as\n"
    "many /bcFBFF00/bananas/bcFFFFFF/ as possible on each stage. Your best run \n"
    "is saved in the Interstellar section of the Galactic log.\n"
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
    "/bc6EFFFD/PLATINUM RANK:/bcFFFFFF/ 4000+ bananas\n"
    "/bcC800FF/STAR RANK:/bcFFFFFF/ 5000+ bananas\n"
    "\n"
    "Each rank will display a unique /bcC800FF/medallion/bcFFFFFF/ at the end of\n"
    "the run, and your best run's medallion will be displayed\n"
    "in the center of /bc009DFF/Monuments/bcFFFFFF/.\n",

    // About Page 6 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/DEBUG/bcFFFFFF/\n"
    "\n"
    "Debug is a bonus mode made up of drafts, prototypes,\n"
    "and joke stages. It's split up into 3 zones:\n"
    "\n"
    "/bc00FF2F/Joke Stages:/bcFFFFFF/ Joke/troll stages that I made for fun\n"
    "/bc009DFF/Stage Drafts:/bcFFFFFF/ Old revisions of stages that got remade,\n"
    "                  and concepts that didn't become stages\n"
    "/bcC800FF/The Unplayable Zone:/bcFFFFFF/ Obnoxious/impossible joke stages\n"
    "\n"
    "PLEASE NOTE: This is meant to be a silly showcase of\n"
    "the inner workings of this pack's development. I'm fully\n"
    "aware these stages are bad. /bc00FF2F/That's why they are here.\n"
    "\n"
    "/bcA1A1A1/* Stages can be skipped through the pause menu/bcFFFFFF/\n",

    // About Page 7 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/MONUMENTS/bcFFFFFF/\n"
    "\n"
    "Monuments is a /bcC800FF/trophy room/bcFFFFFF/ for all of the bonus\n"
    "objectives found in the Galactic Log!\n"
    "\n"
    "The stage contains ten statues that each represent one\n"
    "of Stardust's worlds. Above each statue, you can see\n"
    "see which /bc009DFF/clear/bcFFFFFF/, /bcC800FF/stunt/bcFFFFFF/, or /bcFBFF00/sweep/bcFFFFFF/ badges are completed\n"
    "\n"
    "Each world's /bcFF9900/Stage Challenge/bcFFFFFF/ achievement is marked by\n"
    "an icon above the three rows of badges.\n"
    "\n"
    "/bcC800FF/Continued on next page.../bcFFFFFF/\n",

    // About Page 8 (ONLY SHOW IF BONUS UNLOCKED)
    "/bc00fffb/MONUMENTS/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/Continued:/bcFFFFFF/\n"
    "\n"
    "If you get all 10 /bcC800FF/stunt/bcFFFFFF/ or /bcFBFF00/sweep/bcFFFFFF/ badges in any world,\n"
    "its statue will be decorated with a /bc009DFF/special effect/bcFFFFFF/!\n"
    "\n"
    "If you get all 100 /bcC800FF/stunt/bcFFFFFF/ or /bcFBFF00/sweep/bcFFFFFF/ badges, something\n"
    "special will appear on the /bc009DFF/giant empty pedestals/bcFFFFFF/\n"
    "outside the circle!\n"
    "\n"
    "The center of the stage has a spinning /bc009DFF/medallion/bcFFFFFF/,\n"
    "representing your best Interstellar rank!\n"};
constexpr char* s_log_pages_credits[2] = {
    "/bcFFFFFF/NOTE: Credits.pdf (included with the ISO) has more\n"
    "annotated credits, including clickable /bc008CFF/links/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/DIRECT CONTRIBUTIONS:/bcFFFFFF/\n"
    "\n"
    "/bc00fffb/Original Soundtrack/bcFFFFFF/\n"
    "      (>^^)> Walkr (/bc008CFF/linktr.ee/walkrmusic/bcFFFFFF/)\n"
    "      (>^^)> Relayer (/bc008CFF/wxokeys.bandcamp.com/bcFFFFFF/)\n"
    "/bc00fffb/Custom Code/bcFFFFFF/\n"
    "      (>^^)> rehtrop, The BombSquad, Eucalyptus\n"
    "/bc00fffb/Art/bcFFFFFF/\n"
    "      (>^^)> Shadow (/bc008CFF/charredshadow.tumblr.com/bcFFFFFF/)\n"
    "/bc00fffb/Playtesters/bcFFFFFF/\n"
    "      (>^^)> eddy, 42guy42, rehtrop, Null\n"
    "      (>^^)> Walkr, Goobz, Dyrude, Eucalyptus\n",
    "/bcFFFFFF/NOTE: Credits.pdf (included with the ISO) has more\n"
    "annotated credits, including clickable /bc008CFF/links/bcFFFFFF/\n"
    "\n"
    "/bcC800FF/SPECIAL THANKS:/bcFFFFFF/\n"
    "\n"
    "/bc00fffb/Specific Mentions/bcFFFFFF/\n"
    "      (>^^)> Zona, Ghost Ham, Sudachi\n"
    "      (>^^)> PetresInc, ganon, Dwaitley, scrap651\n"
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
    "/bc6EFFFD/PLATINUM/bcFFFFFF/",
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
constexpr char* s_log_pages_interstellar_pre_unlock =
    "/bcFFFFFF/Come back to this menu after unlocking Interstellar!\n"
    "\n"
    "Unlock all bonus modes by claiming the following achievements:\n"
    "\n"
    "/bcFF9900/BEAT THE GAME:/bcFFFFFF/ Complete Story Mode\n"
    "/bcFF9900/STUNT PILOT:/bcFFFFFF/ Complete 1 /bcC800FF/Stunt Goal/bcFFFFFF/ in each world\n";
constexpr char* s_achievement_names[39] = {
    "",
    "/bcFF9900/BOTTLE FLIP/bcFFFFFF/ | 1-9 Klein Bottle\n"
    "Clear the stage while the goals are upside down",
    "/bcFF9900/UP, UP, AND AWAY/bcFFFFFF/ | 2-6 Liftoff\n"
    "Soar higher than the highest cloud onstage",
    "/bcFF9900/DEFUSED/bcFFFFFF/ | 3-10 Detonation\n"
    "Clear the blue goal without activating the bomb switch",
    "/bcFF9900/I WANNA BE THE BACK GOAL/bcFFFFFF/ | 4-9 Avoidance\n"
    "Enter the blue goal from the back side",
    "/bcFF9900/BEHIND LOCKED DOORS/bcFFFFFF/ | 5-6 Door Dash\n"
    "Clear the blue goal without opening any doors",
    "/bcFF9900/MONOCHROMATIC/bcFFFFFF/ | 6-1 Recolor\n"
    "Clear the stage without entering any portals",
    "/bcFF9900/TARGET MASTER/bcFFFFFF/ | 7-10 Break the Targets\n"
    "Break all 8 targets and finish with time bonus (150s)",
    "/bcFF9900/POTASSIUM ALLERGY/bcFFFFFF/ | 8-4 Frequencies\n"
    "Clear the stage without collecting any bananas",
    "/bcFF9900/FLIP WIZARD/bcFFFFFF/ | 9-3 Flip Switches\n"
    "Clear the stage without flipping the switches once",
    "/bcFF9900/STARSTRUCK/bcFFFFFF/ | 10-10 Impact\n"
    "Clear the stunt goal after it shoots into the sky",
    "/bcFF9900/BEAT THE GAME/bcFFFFFF/\n"
    "Complete Story Mode",
    "/bcFF9900/STUNT TRAINEE/bcFFFFFF/\n"
    "Complete 1 /bcC800FF/Stunt Goal/bcFFFFFF/\n",
    "/bcFF9900/STUNT PILOT/bcFFFFFF/\n"
    "Complete 1 /bcC800FF/Stunt Goal/bcFFFFFF/ in each world\n",
    "/bcFF9900/STUNT SPECIALIST/bcFFFFFF/\n"
    "Complete all 10 /bcC800FF/Stunt Goals/bcFFFFFF/ in any world\n",
    "/bcFF9900/STUNT ACE/bcFFFFFF/\n"
    "Complete all 100 /bcC800FF/Stunt Goals/bcFFFFFF/ in Story Mode\n",
    "/bcFF9900/EATER OF SOULS/bcFFFFFF/\n"
    "Reach /bcFBFF00/5,000 bananas/bcFFFFFF/ in Story Mode\n",
    "/bcFF9900/EATER OF WORLDS/bcFFFFFF/\n"
    "Reach /bcFBFF00/9,999 bananas/bcFFFFFF/ in Story Mode\n",
    "???",
    "???",
    "/bc707070/(HIDDEN)/bcFFFFFF/",
    "/bcFF9900/BRONZE RANK/bcFFFFFF/\n"
    "Finish an Interstellar run with /bcFBFF00/1,000+ bananas/bcFFFFFF/\n",
    "/bcFF9900/SILVER RANK/bcFFFFFF/\n"
    "Finish an Interstellar run with /bcFBFF00/2,000+ bananas/bcFFFFFF/\n",
    "/bcFF9900/GOLD RANK/bcFFFFFF/\n"
    "Finish an Interstellar run with /bcFBFF00/3,000+ bananas/bcFFFFFF/\n",
    "/bcFF9900/PLATINUM RANK/bcFFFFFF/\n"
    "Finish an Interstellar run with /bcFBFF00/4,000+ bananas/bcFFFFFF/\n",
    "/bcFF9900/STAR RANK/bcFFFFFF/\n"
    "Finish an Interstellar run with /bcFBFF00/5,000+ bananas/bcFFFFFF/\n",
    "/bcFF9900/FINISH HIM!/bcFFFFFF/\n"
    "Get all 10 /bc009DFF/goals/bcFFFFFF/ in a single run\n",
    "/bcFF9900/EXTREME POTASSIUM OVERLOAD/bcFFFFFF/\n"
    "(Practice Mode) Get all /bcFBFF00/100 bunches/bcFFFFFF/ and finish on any stage",
    "???",
    "???",
    "???",
    "/bcFF9900/HEY GOOBZ PLAY DEBUG/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Complete or skip through any of the debug zones",
    "/bcFF9900/A COMPLEX JOKE/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Clear a stage with exactly 54.13 on the timer",
    "/bcFF9900/YOU-DA-BACON/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Clear a stage 10x in a row",
    "/bcFF9900/SPLEEF RULES LAWYER/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Clear the stunt goal on Spleef without pressing blue buttons",
    "/bcFF9900/CURRENTS RULES LAWYER/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Clear the stunt goal on Currents without clicking the button",
    "/bcFF9900/ACUTALLY PLAYABLE/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Clear a stage from The Unplayable Zone",
    "/bcFF9900/AAAAA/bcFFFFFF/ /bc707070/(shadow achievement)/bcFFFFFF/\n"
    "Clear a stage after travelling >1,000 mph"};
constexpr char* s_achievement_page_titles[5] = {
    "/bc00fffb/STAGE CHALLENGES (1/2)/bcFFFFFF/",
    "/bc00fffb/STAGE CHALLENGES (2/2)/bcFFFFFF/",
    "/bc00fffb/STORY MODE/bcFFFFFF/",
    "/bc00fffb/INTERSTELLAR/bcFFFFFF/",
    "/bc00fffb/SECRET/bcFFFFFF/ /bc707070/(Not intended as real achievements)/bcFFFFFF/"};
constexpr char* s_memory_card_indicator_names[3] = {
    "No Slot\nLoaded",
    "Slot A\nLoaded",
    "Slot B\nLoaded"};
}// namespace

// The menu for accessing the various pages
void create_galactic_log_menu() {
    if (mkb::main_mode == mkb::MD_SEL) {
        s_main_menu_input_lock = true;
    }
    else if (mkb::main_mode == mkb::MD_GAME) {
        patch::write_nop(reinterpret_cast<void*>(0x80274b5c));                                   // Prevent A button from returning to the pause menu when Galactic Log is open
        patch::write_nop(reinterpret_cast<void*>(0x80274b88));                                   // Prevent B button from returning to the pause screen... at least until everything's ready
        patch::write_word(reinterpret_cast<void*>(0x80274ba8), PPC_INSTR_LI(PPC_R0, 0x00000000));// Prevent Start button from returning to pause menu
    }
    else {
        MOD_ASSERT_MSG(false, "Is there a mode where you can access this that I haven't handled yet? Tell BombSquad!!");
    }

    constexpr Vec2d center = Vec2d{640 / 2, 480 / 2};
    constexpr Vec2d box_size = Vec2d{450, 220};
    constexpr Vec2d box_origin = Vec2d{center.x - (box_size.x / 2), center.y - (box_size.y) / 2};

    // The menu for the Galactic Log
    ui::Menu& galactic_log_menu = ui::get_widget_manager().add(new ui::Menu(box_origin, box_size));
    galactic_log_menu.set_label("galmenu");
    galactic_log_menu.set_alignment(mkb::ALIGN_UPPER_CENTER);
    galactic_log_menu.set_depth(0.002);

    if (s_galactic_log_index) {
        galactic_log_menu.set_active_index(*s_galactic_log_index);
    }

    // The header text
    // Hack for making these children widgets appear above the pause menu screen overlay
    galactic_log_menu.set_depth(0.0055);
    ui::Text& galactic_log_title = galactic_log_menu.add(new ui::Text("Galactic Log"));
    galactic_log_title.set_font_style(mkb::STYLE_TEGAKI);
    galactic_log_menu.set_depth(0.002);

    // Handler for the 'About' button
    auto open_about_handler = [](ui::Widget&, void*) {
        if (heap::get_free_space() > 5 * 1024) {// TEMP FIX: Make sure we're not about to crash! (Practice Mod savestates)
            auto& menu = static_cast<ui::Menu&>(ui::get_widget_manager().find("galmenu"));
            s_galactic_log_index = menu.get_active_index();
            ui::get_widget_manager().remove(menu);
            menu.free_inactive();
            create_about_screen();
        }
        else {
            mkb::call_SoundReqID_arg_1(35);// Announcer saying "ZERO!"
        }
    };

    // Handler for the 'Credits & Special Thanks' button
    auto open_credits_handler = [](ui::Widget&, void*) {
        if (heap::get_free_space() > 5 * 1024) {// TEMP FIX: Make sure we're not about to crash! (Practice Mod savestates)
            auto& menu = static_cast<ui::Menu&>(ui::get_widget_manager().find("galmenu"));
            s_galactic_log_index = menu.get_active_index();
            ui::get_widget_manager().remove(menu);
            menu.free_inactive();
            create_credits_screen();
        }
        else {
            mkb::call_SoundReqID_arg_1(35);// Announcer saying "ZERO!"
        }
    };

    auto open_badge_handler = [](ui::Widget&, void*) {
        if (heap::get_free_space() > 40 * 1024) {// TEMP FIX: Make sure we're not about to crash! (Practice Mod savestates)
            auto& menu = static_cast<ui::Menu&>(ui::get_widget_manager().find("galmenu"));
            s_galactic_log_index = menu.get_active_index();
            ui::get_widget_manager().remove(menu);
            menu.free_inactive();
            create_badge_screen();
        }
        else {
            mkb::call_SoundReqID_arg_1(35);// Announcer saying "ZERO!"
        }
    };

    auto open_interstellar_handler = [](ui::Widget&, void*) {
        if (heap::get_free_space() > 5 * 1024) {// TEMP FIX: Make sure we're not about to crash! (Practice Mod savestates)
            auto& menu = static_cast<ui::Menu&>(ui::get_widget_manager().find("galmenu"));
            s_galactic_log_index = menu.get_active_index();
            ui::get_widget_manager().remove(menu);
            menu.free_inactive();
            create_interstellar_screen();
        }
        else {
            mkb::call_SoundReqID_arg_1(35);// Announcer saying "ZERO!"
        }
    };

    auto open_achievement_handler = [](ui::Widget&, void*) {
        if (heap::get_free_space() > 30 * 1024) {// TEMP FIX: Make sure we're not about to crash! (Practice Mod savestates)
            auto& menu = static_cast<ui::Menu&>(ui::get_widget_manager().find("galmenu"));
            s_galactic_log_index = menu.get_active_index();
            ui::get_widget_manager().remove(menu);
            menu.free_inactive();
            create_achievement_screen();
        }
        else {
            mkb::call_SoundReqID_arg_1(35);// Announcer saying "ZERO!"
        }
    };

    // Handle for 'Close' button, as well as B/Start buttons
    auto close_handler = [](ui::Widget& widget, void*) {
        if (mkb::main_mode == mkb::MD_GAME) {
            // Restores initial pause menu B button functionality
            patch::write_word(reinterpret_cast<void*>(0x80274b88), 0x40820030);
            // Restores initial pause menu start button functionality
            patch::write_word(reinterpret_cast<void*>(0x80274ba8), 0xa0030018);
            // Restores pause menu dim
            patch::write_word(reinterpret_cast<void*>(0x803e7a28), 0x43b40000);
        }

        // Prevents issue where inputs carry over to newly opened pause menu
        s_input_frame_lock_buffer = INPUT_FRAME_LOCK_DELAY;

        // Only execute this if we are closing via the 'Close' button
        auto& input_widget = (ui::Input&) widget;
        if (strcmp(input_widget.get_label(), "galclos") == 0) {
            // Restore initial pause menu A button close functionality
            patch::write_word(reinterpret_cast<void*>(0x80274b5c), 0x4082005c);// bne ...
        }

        s_galactic_log_index.reset();

        // Title screen input issues fix
        if (mkb::main_mode != mkb::MD_GAME) {
            s_main_menu_input_lock = false;
            s_input_frame_lock_buffer = INPUT_FRAME_LOCK_DELAY;
        }

        ui::get_widget_manager().remove("galmenu");
        LOG("After closing free heap: %dkb", heap::get_free_space() / 1024);
    };

    // Hack for making these children widgets appear above the pause menu screen overlay
    galactic_log_menu.set_depth(0.0055);
    auto& open_badge = galactic_log_menu.add(new ui::Text("Story Mode"));
    open_badge.set_callback(open_badge_handler);
    auto& open_interstellar = galactic_log_menu.add(new ui::Text("Interstellar"));
    open_interstellar.set_callback(open_interstellar_handler);
    auto& open_achievement = galactic_log_menu.add(new ui::Text("Achievements"));
    open_achievement.set_callback(open_achievement_handler);
    auto& open_about = galactic_log_menu.add(new ui::Text("About"));
    open_about.set_callback(open_about_handler);
    auto& open_credits = galactic_log_menu.add(new ui::Text("Credit & Special Thanks"));
    open_credits.set_callback(open_credits_handler);
    auto& close = galactic_log_menu.add(new ui::Text("Close"));
    close.set_label("galclos");
    close.set_callback(close_handler);
    galactic_log_menu.set_depth(0.002);

    // Close handler for B button
    auto& close_handler_widget = galactic_log_menu.add(new ui::Input(mkb::PAD_BUTTON_B, close_handler));
    close_handler_widget.set_sound_effect_id(0x70);
}

void update_page_number_display(bool should_display_world = false) {
    char* page_fmt;
    if (should_display_world) {
        page_fmt = "World\n%d/%d";
    }
    else {
        page_fmt = "Page\n%d/%d";
    }
    mkb::sprintf(s_page_number_buffer, page_fmt, s_log_page_number_visible + 1, s_log_page_count_visible);
}

u8 memory_card_indicator_index() {
    return static_cast<s32>(cardio::get_slot()) + 1;
}

// Common/shared elements in Galactic Log go here to avoid code duplication
ui::Layout& create_common_galactic_log_page_layout(
    const char* title,
    const char* label,
    ui::WidgetCallback previous_page_handler,
    ui::WidgetCallback next_page_handler) {
    LOG("Creating Galactic log screen...");

    // Load bmp_how.tpl (must be freed when closed.. TODO)
    mkb::load_bmp_by_id(0xc);

    // No pause menu dim
    if (mkb::main_mode == mkb::MD_GAME) {
        patch::write_word(reinterpret_cast<void*>(0x803e7a28), 0x00000000);

        // Prevent B & Start button from returning to pause menu
        patch::write_nop(reinterpret_cast<void*>(0x80274b88));
    }

    auto& menu_screen_root = ui::get_widget_manager().add(new ui::Layout(Vec2d{0, 0}, Vec2d{0, 0}));
    menu_screen_root.set_layout(ui::LayoutArrangement::NONE);
    menu_screen_root.set_label(label);
    menu_screen_root.set_depth(0.055);

    // Darkened screen sprite
    auto& menu_screen = menu_screen_root.add(new ui::Sprite(0x4b, Vec2d{0, 0}, Vec2d{64, 64}));
    menu_screen.set_scale(Vec2d{300, 200});
    menu_screen.set_alpha(0.6666f);
    menu_screen.set_mult_color({0x00, 0x00, 0x00});// black

    menu_screen_root.set_depth(0.050);

    // Header container
    auto& menu_header_container = menu_screen_root.add(new ui::Layout(Vec2d{0, 5}, Vec2d{640, 128}));
    menu_header_container.set_margin(0);
    menu_header_container.set_spacing(64);
    menu_header_container.set_layout(ui::LayoutArrangement::HORIZONTAL);


    if (previous_page_handler) {
        // Back arrow
        menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));

        // Text
        auto& text = menu_screen_root.add(new ui::Text("Back", Vec2d{16, 58}));
        text.set_alignment(ui::LEFT);
        text.set_scale(Vec2d{0.50, 0.50});
        text.set_color({0xff, 0xff, 0xff});

        // Input handler
        auto& previous_page_input_widget = menu_screen_root.add(new ui::Input(pad::DIR_LEFT, previous_page_handler));
        previous_page_input_widget.set_sound_effect_id(0x6f);
    }

    else {
        menu_header_container.add(new ui::Layout(Vec2d{0, 0}, Vec2d{64, 64}));// TODO: memory-efficient spacer of some kind
    }

    // Title box
    auto& title_box = menu_header_container.add(new ui::Window(Vec2d{0, 0}, Vec2d{384, 64}));
    title_box.set_alignment(mkb::ALIGN_CENTER);

    auto& title_text = title_box.add(new ui::Text(title));
    title_text.set_alignment(ui::CENTER);
    title_text.set_font_style(mkb::STYLE_TEGAKI);

    if (next_page_handler) {
        // Next arrow
        auto& next_arrow = menu_header_container.add(new ui::Sprite(0xc27, Vec2d{0, 0}, Vec2d{64, 64}));
        next_arrow.set_mirror(true);

        // Text
        auto& text = menu_screen_root.add(new ui::Text("Next", Vec2d{16 + 48 + 384 + 64 + 16 + 64, 58}));
        text.set_alignment(ui::LEFT);
        text.set_scale(Vec2d{0.50, 0.50});
        text.set_color({0xff, 0xff, 0xff});

        // Input handler
        auto& next_page_input_widget = menu_screen_root.add(new ui::Input(pad::DIR_RIGHT, next_page_handler));
        next_page_input_widget.set_sound_effect_id(0x6f);
    }

    // Page number
    if (previous_page_handler && next_page_handler) {
        auto& text = menu_screen_root.add(new ui::Text(s_page_number_buffer, Vec2d{16 + 48 + 384 + 64 + 16 + 16 + 6, 36}));
        text.set_alignment(ui::CENTER);
        text.set_scale(Vec2d{0.50, 0.50});
        text.set_color({0xff, 0xff, 0xff});
        update_page_number_display();
    }

    // Memory Card Indicator
    if (previous_page_handler && next_page_handler) {
        mkb::sprintf(s_memory_card_indicator_buffer, s_memory_card_indicator_names[memory_card_indicator_index()]);
        auto& text = menu_screen_root.add(new ui::Text(s_memory_card_indicator_buffer, Vec2d{90, 36}));
        text.set_alignment(ui::CENTER);
        text.set_scale(Vec2d{0.50, 0.50});
        text.set_color({0xff, 0xff, 0xff});
        update_page_number_display();
    }

    auto close_handler = [](ui::Widget&, void* close_label) {
        if (mkb::main_mode == mkb::MD_GAME) {
            patch::write_word(reinterpret_cast<void*>(0x803e7a28), 0x43b40000);
        }
        const char* label = static_cast<const char*>(close_label);
        ui::get_widget_manager().remove(label);
        mkb::free_bmp_by_id(0xc);
        create_galactic_log_menu();
    };

    // Close handler
    auto& close_handler_widget = menu_screen_root.add(new ui::Input(mkb::PAD_BUTTON_B, close_handler));
    close_handler_widget.set_user_data((void*) label);
    close_handler_widget.set_sound_effect_id(0x70);

    return menu_screen_root;
}

void create_about_screen() {
    // Initialize the correct page count/page index
    s_log_page_number = 0;
    s_log_page_number_visible = 0;
    s_log_page_count = 8;

    if (unlock::unlock_condition_met()) {
        s_log_page_count_visible = 7;
    }
    else {
        s_log_page_count_visible = 3;
    }

    auto previous_page_handler = [](ui::Widget&, void*) {
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
        if (s_log_page_number_visible == 0) {
            s_log_page_number_visible = s_log_page_count_visible - 1;
            ;
        }
        else {
            --s_log_page_number_visible;
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[s_log_page_number]);
        update_page_number_display();
    };

    auto next_page_handler = [](ui::Widget&, void*) {
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
        if (s_log_page_number_visible + 1 >= s_log_page_count_visible) {
            s_log_page_number_visible = 0;
        }
        else {
            ++s_log_page_number_visible;
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[s_log_page_number]);
        update_page_number_display();
    };

    // Create common layout
    auto& about_menu_screen = create_common_galactic_log_page_layout("About", "galabou", previous_page_handler, next_page_handler);

    auto& about_container = about_menu_screen.add(new ui::Layout(Vec2d{0, 64 + 5}, Vec2d{640, 480 - 65}));
    mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_about[s_log_page_number]);
    auto& about_text = about_container.add(new ui::Text(s_text_page_buffer));
    about_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    about_text.set_alignment(ui::LEFT);
    about_text.set_drop_shadow(false);
    about_text.set_color({0x00, 0x00, 0x00});
}

void create_credits_screen() {
    // Initialize the correct page count/page index
    s_log_page_number = 0;
    s_log_page_number_visible = 0;
    s_log_page_count = 2;
    s_log_page_count_visible = 2;

    auto previous_page_handler = [](ui::Widget&, void*) {
        if (s_log_page_number == 0) {
            s_log_page_number = s_log_page_count - 1;
        }
        else {
            --s_log_page_number;
        }
        if (s_log_page_number_visible == 0) {
            s_log_page_number_visible = s_log_page_count_visible - 1;
            ;
        }
        else {
            --s_log_page_number_visible;
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_credits[s_log_page_number]);
        update_page_number_display();
    };

    auto next_page_handler = [](ui::Widget&, void*) {
        update_page_number_display();
        if (s_log_page_number + 1 >= s_log_page_count) {
            s_log_page_number = 0;
        }
        else {
            ++s_log_page_number;
        }
        if (s_log_page_number_visible + 1 >= s_log_page_count_visible) {
            s_log_page_number_visible = 0;
        }
        else {
            ++s_log_page_number_visible;
        }
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_credits[s_log_page_number]);
        update_page_number_display();
    };

    // Create common layout
    auto& credits_menu_screen = create_common_galactic_log_page_layout("Credits & Special Thanks", "galcred", previous_page_handler, next_page_handler);

    // Credits container
    auto& credits_container = credits_menu_screen.add(new ui::Layout(Vec2d{0, 64 + 5}, Vec2d{640, 480 - 65}));
    mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_credits[s_log_page_number]);
    auto& credits_text = credits_container.add(new ui::Text(s_text_page_buffer));
    credits_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    credits_text.set_alignment(mkb::ALIGN_LOWER_RIGHT);
    credits_text.set_drop_shadow(false);
    credits_text.set_color({0x00, 0x00, 0x00});
}

void create_badge_list() {
    auto& badge_menu_screen = static_cast<ui::Layout&>(ui::get_widget_manager().find("galbadg"));

    auto& badge_container = badge_menu_screen.add(new ui::Layout(Vec2d{0, 64 + 5}, Vec2d{640, 480 - 65}));
    badge_container.set_label("galbdgc");
    badge_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    for (uint32_t stage_idx = 0; stage_idx < 10; stage_idx++) {
        auto& layout_row = badge_container.add(new ui::Layout(Vec2d{0, 0}, Vec2d{630, 36}));
        layout_row.set_margin(0);
        layout_row.set_spacing(0);
        layout_row.set_layout(ui::LayoutArrangement::HORIZONTAL);

        // Bar sprite - not happy with how I had to implement it this way but a lesson for the future I guess
        constexpr float margin = 5.f;
        mkb::Rgb24 bar_add_color = {0x99, 0x99, 0x99};
        auto& bar_sprite_start = layout_row.add(new ui::Sprite(0xc01, Vec2d{0, 0}, Vec2d{0, 36}));
        bar_sprite_start.set_scale(Vec2d{0.50 * 0.25, 0.50});
        bar_sprite_start.set_uv_2(Vec2d{0.25, 1});
        bar_sprite_start.set_sort(false);
        bar_sprite_start.set_offset(Vec2d{margin, 0});
        bar_sprite_start.set_add_color(bar_add_color);
        bar_sprite_start.set_mult_color(bar_colors[stage_idx]);
        auto& bar_sprite_mid = layout_row.add(new ui::Sprite(0xc01, Vec2d{0, 0}, Vec2d{0, 36}));
        bar_sprite_mid.set_scale(Vec2d{6.35, 0.50});
        bar_sprite_mid.set_uv_1(Vec2d{0.25, 0});
        bar_sprite_mid.set_uv_2(Vec2d{0.75, 1});
        bar_sprite_mid.set_sort(false);
        bar_sprite_mid.set_offset(Vec2d{315, 0});
        bar_sprite_mid.set_add_color(bar_add_color);
        bar_sprite_mid.set_mult_color(bar_colors[stage_idx]);
        auto& bar_sprite_end = layout_row.add(new ui::Sprite(0xc01, Vec2d{0, 0}, Vec2d{0, 36}));
        bar_sprite_end.set_scale(Vec2d{0.50 * 0.25, 0.50});
        bar_sprite_end.set_uv_1(Vec2d{0.75, 0});
        bar_sprite_end.set_uv_2(Vec2d{1, 1});
        bar_sprite_end.set_sort(false);
        bar_sprite_end.set_offset(Vec2d{640 - 10 - 5, 0});
        bar_sprite_end.set_add_color(bar_add_color);
        bar_sprite_end.set_mult_color(bar_colors[stage_idx]);
        auto& bar_circle = layout_row.add(new ui::Sprite(0xc02, Vec2d{0, 0}, Vec2d{0, 36}));
        bar_circle.set_scale(Vec2d{0.50, 0.50});
        bar_circle.set_depth(bar_sprite_start.get_depth() - 0.002);
        bar_circle.set_offset(Vec2d{16, 0});
        bar_circle.set_sort(false);

        // bar_sprite.set_offset(Vec2d{0, -16});
        auto& text_container = layout_row.add(new ui::Layout(Vec2d{0, 0}, Vec2d{540, 36}));
        text_container.set_layout(ui::LayoutArrangement::HORIZONTAL);
        auto& sprite_container = layout_row.add(new ui::Layout(Vec2d{0, 0}, Vec2d{84, 36}));
        sprite_container.set_layout(ui::LayoutArrangement::HORIZONTAL);
        sprite_container.set_spacing(0);
        sprite_container.set_margin(0);

        uint32_t stage_id = mkb::get_story_mode_stage_id(s_log_page_number, stage_idx);
        // LOG("Got id %d", stage_id);
        char stage_name_buffer[64] = {0};
        mkb::read_stage_name_from_dvd(stage_id, stage_name_buffer, 64);
        // LOG("Got name %s", stage_name_buffer)

        mkb::sprintf(s_badge_stage_number_buffer[stage_idx], "%d", stage_idx + 1);
        mkb::sprintf(s_badge_stage_name_buffer[stage_idx], "%s", stage_name_buffer);
        // LOG("Did sprintf to yield: %s", s_badge_stage_name_buffer[stage_idx])
        auto& number_text = text_container.add(new ui::Text(s_badge_stage_number_buffer[stage_idx]));
        number_text.set_depth(bar_circle.get_depth() - 0.002);
        number_text.set_drop_shadow(false);
        number_text.set_alignment(ui::LEFT);
        number_text.set_color({0x00, 0x00, 0x00});

        float spacer_size = 8;
        if (stage_idx == 9) {
            spacer_size = 0;
            number_text.set_offset(Vec2d{-5, 0});
            // number_text.set_scale(Vec2d{0.75, 1.0});// fit 10 inside bubbles
        }
        else if (stage_idx == 0) {
            spacer_size = 12;
            number_text.set_offset(Vec2d{3, 0});
        }
        else {
            number_text.set_offset(Vec2d{2, 0});
        }

        text_container.add(new ui::Layout(Vec2d{0, 0}, Vec2d{spacer_size, 0}));
        auto& name_text = text_container.add(new ui::Text(s_badge_stage_name_buffer[stage_idx]));
        name_text.set_depth(bar_circle.get_depth() - 0.002);
        name_text.set_drop_shadow(false);
        name_text.set_alignment(ui::LEFT);
        name_text.set_color({0x00, 0x00, 0x00});

        // 0xc3b = blue, 0xc3a = purple, 0xc39 = sweep, 0xc3c = achievement, 0xc3d = empty
        uint32_t id_1 = 0xc3d;
        uint32_t id_2 = 0xc3d;
        uint32_t id_3 = 0xc3d;
        if (savedata::true_in_slot(savedata::CLEAR_BADGE_START + s_log_page_number * 10 + stage_idx)) id_1 = 0xc3b;
        if (savedata::true_in_slot(savedata::STUNT_BADGE_START + s_log_page_number * 10 + stage_idx)) id_2 = 0xc3a;
        if (savedata::true_in_slot(savedata::SWEEP_BADGE_START + s_log_page_number * 10 + stage_idx)) id_3 = 0xc39;
        auto& blue = sprite_container.add(new ui::Sprite(id_1, Vec2d{28, 28}));
        blue.set_offset(Vec2d{0, 2});
        auto& purple = sprite_container.add(new ui::Sprite(id_2, Vec2d{28, 28}));
        purple.set_offset(Vec2d{0, 2});
        auto& sweep = sprite_container.add(new ui::Sprite(id_3, Vec2d{28, 28}));
        sweep.set_offset(Vec2d{0, 2});

        blue.set_scale(Vec2d{0.40, 0.40});
        purple.set_scale(Vec2d{0.40, 0.40});
        sweep.set_scale(Vec2d{0.40, 0.40});
    }
}
void create_badge_screen() {
    // Initialize the correct page count/page index
    s_log_page_number = 0;
    s_log_page_number_visible = 0;

    auto previous_page_handler = [](ui::Widget&, void*) {
        auto& badge_menu_screen = static_cast<ui::Layout&>(ui::get_widget_manager().find("galbadg"));
        badge_menu_screen.remove("galbdgc");
        if (s_log_page_number == 0) {
            s_log_page_number = s_log_page_count - 1;
        }
        else {
            --s_log_page_number;
        }
        if (s_log_page_number_visible == 0) {
            s_log_page_number_visible = s_log_page_count_visible - 1;
            ;
        }
        else {
            --s_log_page_number_visible;
        }
        badge_menu_screen.free_inactive();
        create_badge_list();
        update_page_number_display(true);
    };

    auto next_page_handler = [](ui::Widget&, void*) {
        auto& badge_menu_screen = static_cast<ui::Layout&>(ui::get_widget_manager().find("galbadg"));
        badge_menu_screen.remove("galbdgc");
        if (s_log_page_number + 1 >= s_log_page_count) {
            s_log_page_number = 0;
        }
        else {
            ++s_log_page_number;
        }
        if (s_log_page_number_visible + 1 >= s_log_page_count) {
            s_log_page_number_visible = 0;
        }
        else {
            ++s_log_page_number_visible;
        }
        badge_menu_screen.free_inactive();
        create_badge_list();
        update_page_number_display(true);
    };

    // Create common layout
    create_common_galactic_log_page_layout("Story Mode", "galbadg", previous_page_handler, next_page_handler);

    LOG("Creating stage name list...");
    create_badge_list();

    // Only display up to the highest unlocked world (using story cutscene unlock info)
    s_log_page_count = savedata::latest_played_world();
    s_log_page_count_visible = savedata::latest_played_world();

    update_page_number_display(true);
}

void create_interstellar_screen() {
    auto empty_handler = ui::WidgetCallback();

    // Create common layout
    auto& interstellar_menu_screen = create_common_galactic_log_page_layout("Interstellar", "galints", empty_handler, empty_handler);

    // Interstellar
    auto& interstellar_container = interstellar_menu_screen.add(new ui::Layout(Vec2d{0, 64 + 5}, Vec2d{640, 480 - 65}));
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
        mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_interstellar_pre_unlock);
    }
    auto& interstellar_text = interstellar_container.add(new ui::Text(s_text_page_buffer));
    interstellar_container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    interstellar_text.set_alignment(mkb::ALIGN_LOWER_RIGHT);
    interstellar_text.set_drop_shadow(false);
    interstellar_text.set_color({0x00, 0x00, 0x00});
}

void create_achievement_list() {
    auto& achievement_menu_screen = static_cast<ui::Layout&>(ui::get_widget_manager().find("galachv"));

    auto& achievement_container = achievement_menu_screen.add(new ui::Layout(Vec2d{0, 64 + 5}, Vec2d{640, 480 - 65}));
    achievement_container.set_label("galachl");
    achievement_container.set_alignment(mkb::ALIGN_UPPER_LEFT);

    // Page title
    float page_title_height = 32;// Page title WAS 2 lines on the shadow achievement pages (5 & 6)
    // if (s_log_page_number == 4 || s_log_page_number == 5) page_title_height = 64;
    auto& layout_row_page_title = achievement_container.add(new ui::Layout(Vec2d{0, 0}, Vec2d{630, page_title_height - (page_title_height / 4)}));
    layout_row_page_title.set_margin(0);
    layout_row_page_title.set_spacing(0);
    layout_row_page_title.set_layout(ui::LayoutArrangement::HORIZONTAL);
    auto& text_container_page_title = layout_row_page_title.add(new ui::Layout(Vec2d{0, 0}, Vec2d{640, page_title_height}));
    mkb::sprintf(s_text_page_buffer, "%s", s_achievement_page_titles[s_log_page_number]);
    auto& text_page_title = text_container_page_title.add(new ui::Text(s_text_page_buffer));
    text_page_title.set_alignment(ui::LEFT);
    text_page_title.set_drop_shadow(false);// temporary

    // Fill 7 rows with achievements
    for (uint32_t curr_row = 0; curr_row < 7; curr_row++) {
        auto& layout_row = achievement_container.add(new ui::Layout(Vec2d{0, 0}, Vec2d{630, 32 + 16}));
        layout_row.set_margin(0);
        layout_row.set_spacing(0);
        layout_row.set_layout(ui::LayoutArrangement::HORIZONTAL);
        auto& text_container = layout_row.add(new ui::Layout(Vec2d{0, 0}, Vec2d{470 + 100, 32 + 32}));
        auto& sprite_container = layout_row.add(new ui::Layout(Vec2d{0, 0}, Vec2d{32, 32}));
        sprite_container.set_layout(ui::LayoutArrangement::HORIZONTAL);

        u8 curr_id = 0;// Current achievement being displayed (0 = empty)
        bool show_badge_slot = true;
        // Show list of achievements (based on current page # & some conditionals)
        switch (s_log_page_number) {
            case 0: {// Stage Challenges (1/2)
                switch (curr_row) {
                    // Show id 1
                    case 0:
                        curr_id = 1;
                        break;
                    // If played world, show id 2-5
                    case 1 ... 4:
                        if (savedata::latest_played_world() > curr_row) curr_id = curr_row + 1;
                        break;
                    case 5 ... 6: {
                        curr_id = 0;
                        show_badge_slot = false;
                        break;// Empty rows w/out badge
                    }
                }
                break;
            }
            case 1: {// Stage Challenges (2/2)
                switch (curr_row) {
                    // If played world, show id 6-10
                    case 0 ... 4:
                        if (savedata::latest_played_world() > curr_row + 5) curr_id = curr_row + 6;
                        break;
                    case 5 ... 6: {
                        curr_id = 0;
                        show_badge_slot = false;
                        break;// Empty rows w/out badge
                    }
                }
                break;
            }
            case 2: {// Story Mode
                switch (curr_row) {
                    // Show id 11-17
                    case 0 ... 6:
                        curr_id = curr_row + 11;
                        break;
                }
                break;
            }
            case 3: {// Interstellar
                switch (curr_row) {
                    // Show id 21-27
                    case 0 ... 6:
                        curr_id = curr_row + 21;
                        break;
                }
                break;
            }
            case 4: {// Secret (1/2)
                switch (curr_row) {
                    // If completed, show id 31-34
                    case 0 ... 6:
                        if (savedata::true_in_slot(331 + curr_row - 1)) curr_id = curr_row + 31;
                        break;
                }
                break;
            }
        }

        if (show_badge_slot && curr_id == 0) curr_id = 20;// (HIDDEN)
        mkb::sprintf(s_achievement_name_buffer[curr_row], "%s", s_achievement_names[curr_id]);
        auto& text = text_container.add(new ui::Text(s_achievement_name_buffer[curr_row]));

        if (show_badge_slot) {
            // 0xc3b = blue, 0xc3a = purple, 0xc39 = sweep, 0xc3c = achievement, 0xc3d = empty
            uint32_t id_1 = 0xc3d;
            if (savedata::true_in_slot(300 + curr_id - 1)) id_1 = 0xc3c;
            auto& ach_icon = sprite_container.add(new ui::Sprite(id_1, Vec2d{32, 32}));

            ach_icon.set_offset(Vec2d{12, 12});
            ach_icon.set_scale(Vec2d{1.0, 1.0});
        }

        text.set_alignment(ui::LEFT);
        text.set_drop_shadow(false);// temporary
    }
}

/*
static void show_achievement(u8 id, auto& container) {
    mkb::sprintf(s_text_page_buffer, "%s", s_achievement_names[id]);
    auto& achievement_text = container.add(new ui::Text(s_text_page_buffer));
    container.set_alignment(mkb::ALIGN_UPPER_LEFT);
    achievement_text.set_alignment(ui::LEFT);
    achievement_text.set_drop_shadow(false);
    achievement_text.set_color({0x00, 0x00, 0x00});
    // TODO: Add badge
} */

/* static void show_achievement(u8 id, u8 row) {
    achievement_row[row] = s_achievement_names[id];
} */

void create_achievement_screen() {
    // Initialize the correct page count/page index
    s_log_page_number = 0;
    s_log_page_number_visible = 0;
    s_log_page_count = 5;
    s_log_page_count_visible = 0;

    bool is_page_shown[6] = {
        // Stage Challenges (1/2), always show
        true,
        // Stage Challenges (2/2), only show if a stage w6+ has been beaten
        (savedata::latest_played_world() > 5),
        // Story Mode, always show
        true,
        // Interstellar, show if unlocked
        unlock::unlock_condition_met(),
        // Secret, show if any secrets 1-7 are complete
        !savedata::consecutive_false_from_slot(330, 7)};

    for (auto page: is_page_shown) {
        if (page) s_log_page_count_visible++;
    }

    auto previous_page_handler = [](ui::Widget&, void*) {
        // Initialize which pages get skipped
        // (For some reason initializing this outside the lambda function causes "not captured" errors)
        bool is_page_shown[6] = {
            // Stage Challenges (1/2), always show
            true,
            // Stage Challenges (2/2), only show if a stage w6+ has been beaten
            (savedata::latest_played_world() > 5),
            // Story Mode, always show
            true,
            // Interstellar, show if unlocked
            unlock::unlock_condition_met(),
            // Secret, show if any secrets 1-7 are complete
            !savedata::consecutive_false_from_slot(330, 7)};

        auto& achievement_menu_screen = static_cast<ui::Layout&>(ui::get_widget_manager().find("galachv"));
        achievement_menu_screen.remove("galachl");
        // Loop until we reach a shown page.
        while (true) {
            if (s_log_page_number == 0) {
                s_log_page_number = s_log_page_count - 1;
            }
            else {
                --s_log_page_number;
            }
            // Stop the loop if we've reached a shown page
            if (is_page_shown[s_log_page_number]) break;
        }
        // mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_achievement[s_log_page_number]);
        achievement_menu_screen.free_inactive();
        create_achievement_list();

        if (s_log_page_number_visible == 0) {
            s_log_page_number_visible = s_log_page_count_visible - 1;
        }
        else {
            --s_log_page_number_visible;
        }
        update_page_number_display();
    };

    auto next_page_handler = [](ui::Widget&, void*) {
        // Initialize which pages get skipped
        // (For some reason initializing this outside the lambda function causes "not captured" errors)
        bool is_page_shown[5] = {
            // Stage Challenges (1/2), always show
            true,
            // Stage Challenges (2/2), only show if a stage w6+ has been beaten
            (savedata::latest_played_world() > 5),
            // Story Mode, always show
            true,
            // Interstellar, show if unlocked
            unlock::unlock_condition_met(),
            // Secret, show if any secrets 1-7 are complete
            !savedata::consecutive_false_from_slot(330, 7)};

        auto& achievement_menu_screen = static_cast<ui::Layout&>(ui::get_widget_manager().find("galachv"));
        achievement_menu_screen.remove("galachl");
        // Loop until we reach a shown page.
        while (true) {
            if (s_log_page_number + 1 >= s_log_page_count) {
                s_log_page_number = 0;
            }
            else {
                ++s_log_page_number;
            }
            // Stop the loop if we've reached a shown page
            if (is_page_shown[s_log_page_number]) break;
        }
        if (s_log_page_number_visible + 1 >= s_log_page_count_visible) {
            s_log_page_number_visible = 0;
        }
        else {
            ++s_log_page_number_visible;
        }
        // mkb::sprintf(s_text_page_buffer, "%s", s_log_pages_achievement[s_log_page_number]);
        achievement_menu_screen.free_inactive();
        create_achievement_list();
        update_page_number_display();
    };

    create_common_galactic_log_page_layout("Achievements", "galachv", previous_page_handler, next_page_handler);
    create_achievement_list();
}

void init_main_loop() {
    patch::hook_function(s_g_create_how_to_sprite_tramp, mkb::create_how_to_sprite, [](void) {
        if (heap::get_free_space() > 20 * 1024) {// TEMP FIX: Make sure we're not about to crash! (Practice Mod savestates)
            if (mkb::main_mode == mkb::MD_GAME) {
                mkb::g_some_pausemenu_var = 4;
                mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
            }
            mkb::call_SoundReqID_arg_1(10);
            LOG("Heap free before: %db", heap::get_free_space() );
            create_galactic_log_menu();
            LOG("Heap free after: %db", heap::get_free_space() );
        }
        else {
            mkb::call_SoundReqID_arg_1(35);// Announcer saying "ZERO!"
        }
        return;
    });

    patch::hook_function(s_check_pause_menu_input, mkb::check_pause_menu_input, [](mkb::Sprite* s) {
        if (s_input_frame_lock_buffer > 0) {
            s_input_frame_lock_buffer--;
            return;
        }

        s_check_pause_menu_input.dest(s);
    });
}
void init_main_game() {
}

void init_sel_ngc() {
    patch::hook_function(s_did_any_pad_press_input, mkb::did_any_pad_press_input, [](mkb::PadInputID id) {
        if (!s_main_menu_input_lock && !s_input_frame_lock_buffer) return s_did_any_pad_press_input.dest(id);
        else {
            if (s_main_menu_input_lock) {
                return false;
            }

            s_input_frame_lock_buffer--;
            return false;
        }
    });
}
// static u32 s_sound = 0;
void tick() {
    // if (pad::button_pressed(mkb::PAD_BUTTON_UP)) {
    //     mkb::call_SoundReqID_arg_1(s_sound);
    //     s_sound++;
    // }
    // if (pad::button_pressed(mkb::PAD_BUTTON_LEFT)) {
    //     s_sound = 0;
    // }
}

}// namespace galactic_log
