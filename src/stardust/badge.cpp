#include "badge.h"

#include "mkb/mkb.h"
#include "mkb/mkb2_ghidra.h"
#include "stardust/validate.h"
#include "internal/patch.h"
#include "internal/tickable.h"

namespace badge {

// Patch is enabled by default
TICKABLE_DEFINITION((
.name = "stardust-badge",
.description = "Badges",
.enabled = true,
.on_goal = on_goal,
))

int stage_id_to_stage_number(int stage_id){
    switch (stage_id) {
        case 201 ... 204: return stage_id - 200; break;
        case 1 ... 16:    return stage_id + 4;   break;
        case 231 ... 239: return stage_id - 210; break;
        case 17 ... 47:   return stage_id + 13;  break;
        case 281 ... 289: return stage_id - 220; break;
        case 48 ... 68:   return stage_id + 22;  break;
        case 341 ... 350: return stage_id - 250; break;
        default: return 0;
    }
}

bool detect_sweep(){
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue; // skip if its inactive
        mkb::Item &item = mkb::items[i]; // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue; // skip if its not a bunch
        if(item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd){ // True if banana is gone
            continue;
        }
        else return false; // Returns false if any bunches are un-collected
    }
    return true;
}

void on_goal(){
    mkb::balls[mkb::curr_player_idx].banana_count = stage_id_to_stage_number(mkb::g_current_stage_id);
}

} // namespace badge