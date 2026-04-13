#include "menu_bg.h"

#include "../internal/patch.h"
#include "../mkb/mkb.h"
#include "internal/tickable.h"
#include "utils/ppcutil.h"
#include "savedata.h" // for 42guy easter egg

namespace menu_bg {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-menu-bg",
        .description = "Menu BG",
        .enabled = true,
        .init_main_loop = init,
        .init_main_game = init_main_game, ))

u16 stage_id = 381;

static void decide_stgname(/*int locale_index*/) {
    char guy42_egg_name[] = {'4', '2', 'G', 'U', 'Y'};// 42GUY
    bool is_42guy_egg_any_file = false;
    for (u8 file = 0; file < 3; file++) {
        bool is_42guy_egg = true;
        for (u8 letter = 0; letter < 5; letter++) {
            if (mkb::storymode_save_files[file].file_name[letter] != guy42_egg_name[letter]) {
                is_42guy_egg = false;
            }
        }
        if(is_42guy_egg) is_42guy_egg_any_file = true;
    }

    // Old Method: Run function instantly
    //if (is_42guy_egg_any_file) mkb::g_load_stgname_file(mkb::LOCALE_ITALIAN);
    //else mkb::g_load_stgname_file(mkb::LOCALE_ENGLISH);

    // New Method: Used as write-bl in original function
    // if (is_42guy_egg_any_file) mkb::g_load_stgname_dvd_entrynum(mkb::LOCALE_ITALIAN);
    // else mkb::g_load_stgname_dvd_entrynum(locale_index);

    // Newer method: Use savedata::save()
    bool old_egg_value = savedata::true_in_slot(savedata::GUY42_EASTER_EGG);
    savedata::write_bool_to_slot(savedata::GUY42_EASTER_EGG, is_42guy_egg_any_file);
    if(old_egg_value != is_42guy_egg_any_file) savedata::save();
}

static void deliver_stgname(int locale_index) {
    if(savedata::true_in_slot(savedata::GUY42_EASTER_EGG)) mkb::g_load_stgname_file(mkb::LOCALE_ITALIAN);
    else mkb::g_load_stgname_file(locale_index);
}

static void decide_bg() {
    // Easter egg for special filenames
    char debug_egg_name[] = {'G', 'O', 'O', 'B', 'A'};// GOOBA
    char nova_egg_name[] = {'S', 'N', 'O', 'V', 'A'}; // SNOVA
    char silent_egg_name[] = {'T', 'O', 'P', ' ','1'};// TOP 1
    char walkr_egg_name[] = {'W', 'A', 'L', 'K', 'R'};// WALKR
    char old6_egg_name[] = {'O', 'L', 'D', 'W', '6'}; // OLDW6
    char disco_egg_name[] = {'D', 'I', 'S', 'C', 'O'};// DISCO
    char jelly_egg_name[] = {'J', 'E', 'L', 'L', 'Y'};// JELLY
    for (u8 file = 0; file < 3; file++) {
        bool is_debug_egg = true;
        bool is_nova_egg = true;
        bool is_silent_egg = true;
        bool is_walkr_egg = true;
        bool is_old6_egg = true;
        bool is_disco_egg = true;
        bool is_jelly_egg = true;
        for (u8 letter = 0; letter < 5; letter++) {
            if (mkb::storymode_save_files[file].file_name[letter] != debug_egg_name[letter]) {
                is_debug_egg = false;
            }
            if (mkb::storymode_save_files[file].file_name[letter] != nova_egg_name[letter]) {
                is_nova_egg = false;
            }
            if (mkb::storymode_save_files[file].file_name[letter] != silent_egg_name[letter]) {
                is_silent_egg = false;
            }
            if (mkb::storymode_save_files[file].file_name[letter] != walkr_egg_name[letter]) {
                is_walkr_egg = false;
            }
            if (mkb::storymode_save_files[file].file_name[letter] != old6_egg_name[letter]) {
                is_old6_egg = false;
            }
            if (mkb::storymode_save_files[file].file_name[letter] != disco_egg_name[letter]) {
                is_disco_egg = false;
            }
            if (mkb::storymode_save_files[file].file_name[letter] != jelly_egg_name[letter]) {
                is_jelly_egg = false;
            }
        }
        if (is_debug_egg) {
            stage_id = 205;// Hey Goobz Play SMAL
            return;
        }
        if (is_nova_egg) {
            stage_id = 89;// Flowers
            return;
        }
        if (is_silent_egg) {
            stage_id = 391;// Silent Supernova
            return;
        }
        if (is_walkr_egg) {
            stage_id = 392;// Cityfall (Walkr Splits) 
            return;
        }
        if (is_old6_egg) {
            stage_id = 393;// Old World 6
            return;
        }
        if (is_disco_egg) {
            stage_id = 394;// Disco Saturn (Trailblazing 10)
            return;
        }
        if (is_jelly_egg) {
            stage_id = 395;// Buoyant Depths (Jams 7)
            return;
        }
    }

    // Decides which BG to display (World of first occupied story save, 0 = w1)
    for (u8 file = 0; file < 3; file++) {
        if (mkb::storymode_save_files[file].is_valid) {
            stage_id = 381 + mkb::storymode_save_files[file].current_world;
            break;
        }
    }
}

static void set_main_bg() {
    patch::write_word(reinterpret_cast<void*>(0x80282c10), PPC_INSTR_LI(PPC_R31, stage_id));
    patch::write_word(reinterpret_cast<void*>(0x80282c20), PPC_INSTR_LI(PPC_R31, stage_id));
}

static patch::Tramp<decltype(&mkb::g_load_stage_for_menu_bg)> s_g_load_stage_for_menu_bg_tramp;
static patch::Tramp<decltype(&mkb::g_related_to_loading_story_stageselect)> s_g_related_to_loading_story_stageselect_tramp;

void init_main_game() {
    patch::write_word(reinterpret_cast<void*>(0x808fd958), PPC_INSTR_LI(PPC_R3, stage_id));
    patch::write_word(reinterpret_cast<void*>(0x808fe7d8), PPC_INSTR_LI(PPC_R3, stage_id));
    
    patch::hook_function(s_g_related_to_loading_story_stageselect_tramp, mkb::g_related_to_loading_story_stageselect, [](mkb::uint param_1) {
        decide_stgname();
        s_g_related_to_loading_story_stageselect_tramp.dest(param_1);
    });
}

void init() {
    patch::hook_function(s_g_load_stage_for_menu_bg_tramp, mkb::g_load_stage_for_menu_bg, [](char param_1, int param_2) {
        decide_bg();
        set_main_bg();
        decide_stgname();
        s_g_load_stage_for_menu_bg_tramp.dest(param_1, param_2);
    });

    // Decide stagename during mkb::g_load_stgname_file
    patch::write_branch_bl(reinterpret_cast<void*>(0x802C6C10), reinterpret_cast<void*>(deliver_stgname));
}

}// namespace menu_bg