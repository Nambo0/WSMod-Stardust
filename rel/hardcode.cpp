#include "include/mkb.h"
#include "include/pad.h"

namespace hardcode {

static bool flipped_yet = false;

void init() {}

void tick() {
  switch (mkb::g_current_stage_id) {
  // 5-4 Spleef
  // Make the stunt goal activate when all 45 buttons are pressed
  case 31:
  {
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
  // 9-7 Drawbridges
  // Make the buttons only un-press when the animation reaches frame 0
  case 65:
  {
    // Make sure we're not messing with stuff before or after the stage starts/ends
    if(mkb::sub_mode != mkb::SMD_GAME_PLAY_INIT ||
    mkb::sub_mode != mkb::SMD_GAME_PLAY_MAIN){
      for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
        // Jam reversing buttons, repeat frames 3 -> 2
        if(mkb::stagedef->coli_header_list[i].anim_group_id == 1 &&
        mkb::itemgroups[i].playback_state == 2 &&
        mkb::itemgroups[i].anim_frame == 2){
          mkb::itemgroups[i].anim_frame = 3;
        }
        // Jam playing buttons, repeat frames 58 -> 59
        if(mkb::stagedef->coli_header_list[i].anim_group_id == 1 &&
        mkb::itemgroups[i].playback_state == 0 &&
        mkb::itemgroups[i].anim_frame == 59){
          mkb::itemgroups[i].anim_frame = 58;
        }
      }
    }
    break;
  }
  // 10-8 Butterfly, 8-2 Leaning Tower
  // Make wormhole not flip horizontal momentum
  case 348:
  case 50:
  {
    for (int i = 10; i < 74; i++) {
        if(mkb::did_ball_enter_wormhole(&mkb::balls[mkb::curr_player_idx], &i)){
            flipped_yet = true;
            break;
        }
    }
    if(flipped_yet){
      mkb::balls[mkb::curr_player_idx].vel.x *= -1;
      mkb::balls[mkb::curr_player_idx].pos.x *= -1;
      mkb::cameras[mkb::curr_player_idx].rot.x *= -1;
      mkb::cameras[mkb::curr_player_idx].rot.y *= -1;
    }
    flipped_yet = false;
    break;
  }
  // 2-9 Pitfall
  // Make wormholes not flip horizontal momentum
  case 15:
  {
    int worm_id = 0;
    for (int i = 1; i < 20; i++) {
        if(mkb::did_ball_enter_wormhole(&mkb::balls[mkb::curr_player_idx], &i)){
            flipped_yet = true;
            worm_id = i;
            break;
        }
    }
      if (flipped_yet) {
          auto ball_velocity = &mkb::balls[mkb::curr_player_idx].vel;
          auto camera_rotation = &mkb::cameras[mkb::curr_player_idx].rot;
          if (worm_id != 5 && worm_id != 13 && worm_id != 16 && worm_id != 17) {
              ball_velocity->x *= -1;
              camera_rotation->x -= camera_rotation->x * 2;
              camera_rotation->y -= camera_rotation->y * 2;
          }
          else {
              ball_velocity->z *= -1;
              // Rotate by 180deg to mirror across correct axis
              camera_rotation->x += 32768;
              camera_rotation->y += 32768;
              camera_rotation->x -= camera_rotation->x * 2;
              camera_rotation->y -= camera_rotation->y * 2;
          }
      }
      flipped_yet = false;
    break;
  }
  }
}
} // namespace hardcode