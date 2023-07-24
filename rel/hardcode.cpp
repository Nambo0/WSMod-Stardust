#include "mkb.h"

namespace hardcode {

void init() {}

void tick() {
  switch (mkb::g_current_stage_id) {
  // 5-4 Spleef
  case 31:
    // Count number of activated green buttons
    u32 activated_count = 0;
    for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
      u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
      if (((anim_id >= 3 && anim_id <= 5) || (anim_id >= 10 && anim_id <= 12) ||
           (anim_id >= 15 && anim_id <= 35) ||
           (anim_id >= 38 && anim_id <= 40) ||
           (anim_id >= 45 && anim_id <= 47) ||
           (anim_id >= 51 && anim_id <= 53) ||
           (anim_id >= 56 && anim_id <= 58) ||
           (anim_id >= 61 && anim_id <= 63) ||
           (anim_id >= 66 && anim_id <= 68)) &&
          mkb::itemgroups[i].anim_frame < 1) {
        activated_count++;
      }
    }
    // for (u32 i = 0; i < mkb::stagedef->button_count; i++) {
    //   mkb::StagedefButton button = mkb::stagedef->button_list[i];
    //   if (button.anim_group_id < 1000 && button.playback_state == 1) {
    //     activated_count++;
    //   }
    // }

    // If all 45 green buttons are activated, open stunt goal (id 69)
    if (activated_count < 1) {
      for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
        if (mkb::stagedef->coli_header_list[i].anim_group_id != 69) {
          continue;
        }
        mkb::itemgroups[i].playback_state = 0;
        break;
      }
    }
    break;
  }
}
} // namespace hardcode