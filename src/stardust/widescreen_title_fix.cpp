#include "widescreen_title_fix.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "mkb/mkb.h"
#include "../stardust/savedata.h"

namespace widescreen_title_fix {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-widescreen-title-fix",
        .description = "Widescreen Title Fix",
        .enabled = true,
        .init_main_loop = init, ))

// void menu_option_screen_tick(void);
static patch::Tramp<decltype(&mkb::menu_option_screen_tick)> s_menu_option_screen_tick_tramp;
// void create_title_sprite(s32 param_1);
static patch::Tramp<decltype(&mkb::create_title_sprite)> s_create_title_sprite_tramp;

void init() {
    patch::hook_function(s_menu_option_screen_tick_tramp, mkb::menu_option_screen_tick, [](void) {
        s_menu_option_screen_tick_tramp.dest();
        // Check for misalignment between actual vs saved widescreen mode, and save the correct value 
        if(savedata::true_in_slot(savedata::WIDESCREEN_MODE) && mkb::widescreen_mode == mkb::NORMAL){
            savedata::write_bool_to_slot(savedata::WIDESCREEN_MODE, false);
            savedata::save();
        }
        if(!savedata::true_in_slot(savedata::WIDESCREEN_MODE) && mkb::widescreen_mode != mkb::NORMAL){
            savedata::write_bool_to_slot(savedata::WIDESCREEN_MODE, true);
            savedata::save();
        }
    });

    patch::hook_function(s_create_title_sprite_tramp, mkb::create_title_sprite, [](s32 param_1) {
        if(savedata::true_in_slot(savedata::WIDESCREEN_MODE)){
            mkb::widescreen_mode = mkb::WIDESCREEN;
        }
        s_create_title_sprite_tramp.dest(param_1);
    });
}

}// namespace widescreen_title_fix