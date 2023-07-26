#include "include/mkb.h"
#include "include/mkb2_ghidra.h"
#include "include/validate.h"
#include "include/patch.h"

namespace badge {

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

void on_goal(){
    mkb::balls[mkb::curr_player_idx].banana_count = stage_id_to_stage_number(mkb::g_current_stage_id);
}

} // namespace badge