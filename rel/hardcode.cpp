#include "mkb.h"

namespace hardcode {

void init() {}

void tick() {
  switch (mkb::g_current_stage_id) {
  // 5-4 Spleef
  case 31:
    // Count number of activated green buttons
    u32 deactivated_count = 0;
    for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
      u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
      if ((anim_id >= 1 && anim_id <= 45) &&
          mkb::itemgroups[i].anim_frame < 1) {
        deactivated_count++;
      }
    }
    // for (u32 i = 0; i < mkb::stagedef->button_count; i++) {
    //   mkb::StagedefButton button = mkb::stagedef->button_list[i];
    //   if (button.anim_group_id < 1000 && button.playback_state == 1) {
    //     deactivated_count++;
    //   }
    // }

    // If all 45 green buttons are activated, open stunt goal (id 69)
    if (deactivated_count < 1) {
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