#include "mkb.h"

namespace hardcode {

void init() {}

void tick() {
  switch (mkb::g_current_stage_id) {
  // 5-4 Spleef
  // Make the stunt goal activate when all 45 buttons are pressed
  case 31:
    // Count number of activated green buttons
    u32 inactive_count = 0;
    for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
      u32 anim_id = mkb::stagedef->coli_header_list[i].anim_group_id;
      if ((anim_id >= 1 && anim_id <= 45) &&
          mkb::itemgroups[i].anim_frame < 1) {
        inactive_count++;
      }
    }

    // If all 45 green buttons are activated, open stunt goal (id 69)
    if (inactive_count < 1) {
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