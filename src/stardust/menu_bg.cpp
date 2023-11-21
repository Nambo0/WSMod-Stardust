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
        .init_main_game = init_main_game, ))

u16 stage_id = 381;

void init_main_game() {
    patch::write_word(reinterpret_cast<void*>(0x808fd958), PPC_INSTR_LI(PPC_R3, stage_id));
    patch::write_word(reinterpret_cast<void*>(0x808fe7d8), PPC_INSTR_LI(PPC_R3, stage_id));
}

static void decide_bg(){
    // Easter egg for special filenames
    char debug_egg_name[] = {'G', 'O', 'O', 'B', 'A'}; // GOOBA
    char nova_egg_name[] = {'S', 'N', 'O', 'V', 'A'}; // SNOVA
    for (u8 file = 0; file < 3; file++){
        bool is_debug_egg = true;
        bool is_nova_egg = true;
        for(u8 letter = 0; letter < 5; letter++){
            if(mkb::storymode_save_files[file].file_name[letter] != debug_egg_name[letter]){
                is_debug_egg = false;
            }
            if(mkb::storymode_save_files[file].file_name[letter] != nova_egg_name[letter]){
                is_nova_egg = false;
            }
        }
        if(is_debug_egg){
            stage_id = 205; // Hey Goobz Play SMAL
            return;
        }
        if(is_nova_egg){
            stage_id = 89; // Flowers
            return;
        }
    }

    // Decides which BG to display (World of first occupied story save, 0 = w1)
    for (u8 file = 0; file < 3; file++){
        if(mkb::storymode_save_files[file].is_valid){
            stage_id = 381 + mkb::storymode_save_files[file].current_world;
            break;
        }
    }
}

static void set_main_bg(){
    patch::write_word(reinterpret_cast<void*>(0x80282c10), PPC_INSTR_LI(PPC_R31, stage_id));
    patch::write_word(reinterpret_cast<void*>(0x80282c20), PPC_INSTR_LI(PPC_R31, stage_id));
}

static patch::Tramp<decltype(&mkb::g_load_stage_for_menu_bg)> s_g_load_stage_for_menu_bg_tramp;

void init() {
    patch::hook_function(s_g_load_stage_for_menu_bg_tramp, mkb::g_load_stage_for_menu_bg, [](char param_1, int param_2) {
        decide_bg();
        set_main_bg();
        s_g_load_stage_for_menu_bg_tramp.dest(param_1, param_2);
    });
}

}// namespace menu_bg