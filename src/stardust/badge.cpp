#include "badge.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "mkb/mkb.h"
#include "../stardust/validate.h"
#include "../stardust/savedata.h"

namespace badge {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-badge",
        .description = "Badges",
        .enabled = true,
        .on_goal = on_goal, ))

int stage_id_to_stage_number(int stage_id) {
    switch (stage_id) {
        case 201 ... 204:
            return stage_id - 200;
            break;
        case 1 ... 16:
            return stage_id + 4;
            break;
        case 231 ... 239:
            return stage_id - 210;
            break;
        case 17 ... 47:
            return stage_id + 13;
            break;
        case 281 ... 289:
            return stage_id - 220;
            break;
        case 48 ... 68:
            return stage_id + 22;
            break;
        case 341 ... 350:
            return stage_id - 250;
            break;
        default:
            return 0; // Any non-story mode stage
    }
}

bool detect_sweep() {
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue;                                       // skip if its inactive
        mkb::Item& item = mkb::items[i];                                                             // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue;                                                           // skip if its not a bunch
        if (item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd) {// True if banana is gone
            continue;
        }
        else return false;// Returns false if any bunches are un-collected
    }
    return true;
}

void claim_blue_goal(u16 stage_number){
    if(stage_number == 0) return; // Don't try to claim anything if we're not on a story mode stage
    u32 claimed_slot = stage_number - 1; // Blue goals start at slot 0
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
    }
}

void claim_stunt_goal(u16 stage_number){
    if(stage_number == 0) return; // Don't try to claim anything if we're not on a story mode stage
    u32 claimed_slot = 100 + stage_number - 1; // Stunt goals start at slot 100
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
    }
}

void claim_sweep(u16 stage_number){
    if(stage_number == 0) return; // Don't try to claim anything if we're not on a story mode stage
    u32 claimed_slot = 200 + stage_number - 1; // Sweep badges start at slot 200
    if(!savedata::true_in_slot(claimed_slot)){
        savedata::write_bool_to_slot(claimed_slot, true);
        savedata::save();
    }
}

void on_goal() {
    if(validate::is_currently_valid()){
        if(mkb::mode_info.entered_goal_type == mkb::Blue){
            claim_blue_goal(stage_id_to_stage_number(mkb::g_current_stage_id));
        }
        else if(mkb::mode_info.entered_goal_type == mkb::Red){
            claim_stunt_goal(stage_id_to_stage_number(mkb::g_current_stage_id));
        }
        if(detect_sweep()){
            claim_sweep(stage_id_to_stage_number(mkb::g_current_stage_id));
        }
    }
    else{
        // TODO: Error message
    }
}

}// namespace badge