#include "include/big_bunch.h"
#include "include/mkb.h"
#include "include/patch.h"
#include "include/utils.h"

namespace big_bunch {

void tick() {

  for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
    if (mkb::item_pool_info.status_list[i] == 0)
      continue; // skip if its inactive
    mkb::Item &item =
        mkb::items[i]; // shorthand: current item in the list = "item"
    if (item.coin_type != 1)
      continue; // skip if its not a bunch
    if (!(mkb::stagedef->coli_header_list[item.itemgroup_idx].anim_group_id >=
              11000 &&
          mkb::stagedef->coli_header_list[item.itemgroup_idx].anim_group_id <=
              12000))
      continue; // skip if anything but 101 anim ID
    item.scale = 5;
  }
}

static patch::Tramp<decltype(&mkb::item_coin_disp)> s_item_coin_disp_tramp;

void remove_closest_bunch_indicator() {
  u32 closest_dist = 1000000;
  Vec3f ball_pos = mkb::balls[mkb::curr_player_idx].pos;
  u32 this_dist = 0;
  u32 closest_id = 1000000;
  for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
    if (mkb::stagedef->coli_header_list[i].anim_group_id != 12001)
      continue;
    Vec3f ig_pos = mkb::stagedef->coli_header_list[i].origin;
    this_dist = VEC_DIST_SQ(ball_pos, ig_pos);
    if (this_dist < closest_dist) {
      closest_dist = this_dist;
      closest_id = i;
    }
  }
  if (closest_id != 1000000) {
    mkb::itemgroups[closest_id].playback_state = 0;
    mkb::itemgroups[closest_id].anim_frame = 2;
  }
}

void new_item_coin_coli(mkb::Item *item, mkb::PhysicsBall *phys_ball) {
  mkb::Effect effect;

  if ((mkb::main_game_mode != mkb::COMPETITION_MODE) ||
      ((mkb::current_ball->phys_flags & mkb::PHYS_DISABLE_CONTROLS) ==
       mkb::PHYS_NONE)) {
    item->g_some_bitfield = item->g_some_bitfield & 0xfffffffd;
    item->g_some_flag = 3;
    (item->velocity).y = (item->velocity).y + item->scale * 0.1875;
    (item->angular_velocity).y = (s16)((int)(item->angular_velocity).y << 2);
    (item->velocity).x = (item->velocity).x + (phys_ball->vel).x * 0.25;
    (item->velocity).y = (item->velocity).y + (phys_ball->vel).y * 0.25;
    (item->velocity).z = (item->velocity).z + (phys_ball->vel).z * 0.25;
    if ((item->g_some_frame_counter < 0) &&
        (((mkb::current_ball->g_effect_flags & 0x100) == 0 &&
          (mkb::sub_mode != mkb::SMD_ADV_DEMO_MAIN)))) {
      item->g_some_frame_counter = mkb::mode_info.stage_time_frames_remaining;
      int bananas_to_add = (int)mkb::coin_types[item->coin_type].banana_count;
      int score_to_add = (int)mkb::coin_types[item->coin_type].g_score_value;
      // For big bananas: Bunch count = # specified in IG anim ID
      if (item->coin_type == 1 &&
          mkb::stagedef->coli_header_list[item->itemgroup_idx].anim_group_id >=
              11000 &&
          mkb::stagedef->coli_header_list[item->itemgroup_idx].anim_group_id <=
              12000) {
        bananas_to_add = (mkb::stagedef->coli_header_list[item->itemgroup_idx]
                              .anim_group_id -
                          11000) *
                         10;
        score_to_add = (mkb::stagedef->coli_header_list[item->itemgroup_idx]
                            .anim_group_id -
                        11000) *
                       1000;
      }
      mkb::add_bananas(bananas_to_add);
      mkb::increment_score(
          (int)mkb::coin_types[item->coin_type].g_score_increment_flag,
          score_to_add);
      item->g_some_flag = 0;
      item->g_some_bitfield = item->g_some_bitfield | 1;
      item->g_some_bitfield = item->g_some_bitfield & 0xfffffffd;
      memset(&effect, 0, 0xb0);
      effect.type = mkb::EFFECT_HOLDING_BANANA;
      effect.g_ball_idx = (short)(char)mkb::current_ball->idx;
      mkb::mtxa_from_mtx(
          (mkb::Mtx *)mkb::itemgroups[phys_ball->itemgroup_idx].transform);
      mtxa_tf_point(&item->position, &effect.g_pos);
      mtxa_tf_vec(&item->velocity, &effect.g_some_vec);
      effect.g_some_rot.x = (item->rotation).x;
      effect.g_some_rot.y = (item->rotation).y;
      effect.g_some_rot.z = (item->rotation).z;
      effect.g_pointer_to_some_struct = (s32)mkb::g_something_with_coins(
          (int **)item->g_something_with_gma_model);
      effect.g_scale.x =
          (item->scale / *(float *)(effect.g_pointer_to_some_struct + 0x14)) *
          1.5;
      effect.g_scale.y = effect.g_scale.x;
      effect.g_scale.z = effect.g_scale.x;
      if (item->coin_type == 1 &&
          mkb::stagedef->coli_header_list[item->itemgroup_idx].anim_group_id >=
              11000 &&
          mkb::stagedef->coli_header_list[item->itemgroup_idx].anim_group_id <=
              12000) {
        effect.g_scale = Vec3f{2.0, 2.0, 2.0};
        big_bunch::remove_closest_bunch_indicator(); // REMOVES THE INDICATOR
      }
      spawn_effect(&effect);
    }
    if (item->coin_type == 1) {
      mkb::call_SoundReqID_arg_0(0x39);
      if (((mkb::mode_info.ball_mode & mkb::BALLMODE_IN_TUTORIAL_SEQUENCE) !=
           mkb::BALLMODE_NONE) ||
          ((mkb::mode_info.ball_mode & mkb::BALLMODE_IN_REPLAY) ==
           mkb::BALLMODE_NONE)) {
        mkb::call_SoundReqID_arg_0(0x2820);
      }
      mkb::call_item_coin_coli_func_for_cur_world_theme();
    } else {
      mkb::call_SoundReqID_arg_0(3);
      if (((mkb::mode_info.ball_mode & mkb::BALLMODE_IN_TUTORIAL_SEQUENCE) !=
           mkb::BALLMODE_NONE) ||
          ((mkb::mode_info.ball_mode & mkb::BALLMODE_IN_REPLAY) ==
           mkb::BALLMODE_NONE)) {
        mkb::call_SoundReqID_arg_0(0x281f);
      }
      mkb::call_item_coin_coli_func_for_cur_world_theme();
    }
  }
}

void init() {
  patch::hook_function(
      s_item_coin_disp_tramp, mkb::item_coin_disp, [](mkb::Item *item) {
        if (item->coin_type == 1 &&
            mkb::stagedef->coli_header_list[item->itemgroup_idx]
                    .anim_group_id >= 11000 &&
            mkb::stagedef->coli_header_list[item->itemgroup_idx]
                    .anim_group_id <= 12000) {
          item->scale = 2;
        }
        s_item_coin_disp_tramp.dest(item);
        if (item->coin_type == 1 &&
            mkb::stagedef->coli_header_list[item->itemgroup_idx]
                    .anim_group_id >= 11000 &&
            mkb::stagedef->coli_header_list[item->itemgroup_idx]
                    .anim_group_id <= 12000) {
          item->scale = 5;
        }
      });
  //   // patch::write_branch_bl(reinterpret_cast<void*>(0x80316c9c),
  //   // reinterpret_cast<void*>(new_effect_draw));
  patch::hook_function(mkb::item_coin_coli, new_item_coin_coli);
}

} // namespace big_bunch