#include "menu_bg.h"

#include "../internal/patch.h"
#include "internal/tickable.h"
#include "../mkb/mkb.h"

namespace menu_bg {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-menu-bg",
        .description = "Menu BG",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick, ))

// Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

void tick() {

}

// static patch::Tramp<decltype(&mkb::g_load_stage_for_menu_bg)> s_g_load_stage_for_menu_bg_tramp;

void init() {
    /*
    patch::hook_function(s_g_load_stage_for_menu_bg_tramp, mkb::g_load_stage_for_menu_bg, [](char param_1, int param_2) {
        s_g_load_stage_for_menu_bg_tramp.dest(param_1, param_2);
    });
    */
}

}// namespace menu_bg