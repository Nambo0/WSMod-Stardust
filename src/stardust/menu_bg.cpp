#include "menu_bg.h"

#include "../internal/patch.h"
#include "internal/tickable.h"
#include "../mkb/mkb.h"
#include "utils/ppcutil.h"

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
    // Decides which BG to display (World of first occupied story save, 0 = w1)
    u8 world = 0;
    int menu_stages[10] = {381, 382, 383, 384, 385, 386, 387, 388, 389, 390};
    for (u8 i = 0; i < 3; i++){
        if(mkb::storymode_save_files[i].is_valid){
            world = mkb::storymode_save_files[i].current_world;
            break;
        }
    }
    patch::write_word(reinterpret_cast<void*>(0x80282c10), PPC_INSTR_LI(PPC_R31, menu_stages[world]));
    patch::write_word(reinterpret_cast<void*>(0x80282c20), PPC_INSTR_LI(PPC_R31, menu_stages[world]));
    if (mkb::main_mode == mkb::MD_GAME) {
        patch::write_word(reinterpret_cast<void*>(0x808fd958), PPC_INSTR_LI(PPC_R3, menu_stages[world]));
        patch::write_word(reinterpret_cast<void*>(0x808fe7d8), PPC_INSTR_LI(PPC_R3, menu_stages[world]));
    }
    // ball.banana_count = world;
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