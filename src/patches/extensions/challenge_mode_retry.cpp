#include "challenge_mode_retry.h"

#include "internal/patch.h"
#include "internal/tickable.h"
#include "utils/ppcutil.h"

namespace challenge_mode_retry {

TICKABLE_DEFINITION((.name = "challenge-mode-retry",
                     .description = "Challenge mode retry",
                     .enabled = true,
                     .init_main_loop = init_main_loop))

void new_handle_pausemenu_selection(int param_1) {
      mkb::Ball *ball;
  mkb::OSHeapHandle OVar1;
  int goal_score;
  int ball_index;
  void *local_158;
  mkb::uint local_154;
  void *local_150;
  mkb::uint local_14c;
  void *local_148;
  mkb::uint local_144;
  mkb::byte abStack_140 [12];
  mkb::byte abStack_134 [12];
  mkb::byte abStack_128 [12];
  mkb::byte local_11c [8];
  int local_114;
  void *local_110;
  mkb::uint local_10c;
  char *local_108;
  mkb::byte local_104 [8];
  int local_fc;
  void *local_f8;
  mkb::uint local_f4;
  char *local_f0;
  mkb::byte local_ec [8];
  int local_e4;
  void *local_e0;
  mkb::uint local_dc;
  char *local_d8;
  char acStack_d4 [64];
  char acStack_94 [64];
  char acStack_54 [76];
  
  ball_index = mkb::curr_player_idx;
  mkb::g_some_other_flags = mkb::g_some_other_flags & ~mkb::OF_GAME_PAUSED;
  switch(mkb::pausemenu_type) {
  case mkb::PMT_UNKNOWN0:
    if (mkb::g_current_focused_pause_menu_entry == 1) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 1) {
      if (0 < mkb::g_current_focused_pause_menu_entry) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
        if (mkb::main_game_mode == mkb::MONKEY_BILLIARDS) {
          mkb::g_some_status_bitflag = mkb::g_some_status_bitflag | 0x20;
        }
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry < 3) {
      mkb::g_some_pausemenu_var = 1;
      mkb::g_return_to_sel_mode(0xffffffff);
    }
    break;
  case mkb::PMT_CHALLENGE:
    if (mkb::g_current_focused_pause_menu_entry == 3) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry == 0) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
      }
      else if (mkb::g_current_focused_pause_menu_entry == 1) {
        mkb::mode_info.attempt_count = mkb::mode_info.attempt_count + 1;
        ball = mkb::balls;
        for (ball_index = 0; ball_index < 8; ball_index = ball_index + 1) {
          if (ball->status == mkb::STAT_NORMAL) {
            ball->phys_flags = ball->phys_flags;
            ball->g_effect_flags = ball->g_effect_flags | 0x80;
          }
          ball = ball + 1;
        }
        mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
        mkb::g_fade_track_volume(100,'\n');
      }
    else if (mkb::g_current_focused_pause_menu_entry == 2) {
        if ((mkb::g_some_status_bitflag & 4) == 0) {
          mkb::g_some_pausemenu_var = 2;
          mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
          mkb::event_init(mkb::EVENT_VIEW);
          if (param_1 != 0) {
            *(mkb::uint *)(param_1 + 0x8c) = *(mkb::uint *)(param_1 + 0x8c) | 1;
          }
        }
        else {
          mkb::g_some_pausemenu_var = 3;
          mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
          mkb::g_some_replay_func(&local_148,&local_144);
          mkb::g_some_replay_func2(abStack_128);
          mkb::g_some_replay_func3(abStack_128,acStack_54);
          if (local_148 != (void *)0x0) {
            mkb::memset(local_ec,0,0x18);
            local_ec[0] = 2;
            local_d8 = acStack_54;
            local_e4 = (int)local_148 + 0x24;
            local_dc = local_144;
            local_e0 = local_148;
            mkb::g_some_replay_func4(0,4,local_ec);
          }
        }
    }
    else if (mkb::g_current_focused_pause_menu_entry == 4) {
      if ((mkb::main_game_mode == mkb::CHALLENGE_MODE) && (mkb::num_players == 1)) {
        mkb::g_some_pausemenu_var = 5;
        mkb::sub_mode_request = mkb::SMD_GAME_INTR_SEL_INIT;
      }
      else {
        mkb::g_some_pausemenu_var = 1;
        mkb::g_return_to_sel_mode(0xffffffff);
      }
    }
    break;
  case mkb::PMT_PRACTICE:
    if (mkb::g_current_focused_pause_menu_entry == 3) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 3) {
      if (mkb::g_current_focused_pause_menu_entry == 1) {
        mkb::mode_info.attempt_count = mkb::mode_info.attempt_count + 1;
        ball = mkb::balls;
        for (ball_index = 0; ball_index < 8; ball_index = ball_index + 1) {
          if (ball->status == mkb::STAT_NORMAL) {
            ball->phys_flags = ball->phys_flags;
            ball->g_effect_flags = ball->g_effect_flags | 0x80;
          }
          ball = ball + 1;
        }
        mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
        mkb::g_fade_track_volume(100,'\n');
      }
      else if (mkb::g_current_focused_pause_menu_entry < 1) {
          mkb::g_some_pausemenu_var = 0;
          mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
          mkb::g_fade_track_volume(100,'\n');
      }
      else if ((mkb::g_some_status_bitflag & 4) == 0) {
        mkb::g_some_pausemenu_var = 2;
        mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
        mkb::event_init(mkb::EVENT_VIEW);
        if (param_1 != 0) {
          *(mkb::uint *)(param_1 + 0x8c) = *(mkb::uint *)(param_1 + 0x8c) | 1;
        }
      }
      else {
        mkb::g_some_pausemenu_var = 3;
        mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
        mkb::g_some_replay_func(&local_150,&local_14c);
        mkb::g_some_replay_func2(abStack_134);
        mkb::g_some_replay_func3(abStack_134,acStack_94);
        if (local_150 != (void *)0x0) {
          mkb::memset(local_104,0,0x18);
          local_104[0] = 2;
          local_f0 = acStack_94;
          local_fc = (int)local_150 + 0x24;
          local_f4 = local_14c;
          local_f8 = local_150;
          mkb::g_some_replay_func4(0,4,local_104);
        }
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry == 5) {
      mkb::g_some_pausemenu_var = 1;
      mkb::g_return_to_sel_mode(0xffffffff);
    }
    else if (mkb::g_current_focused_pause_menu_entry < 5) {
      mkb::mode_flags = mkb::mode_flags | 0x8000;
      mkb::g_some_pausemenu_var = 1;
      mkb::g_return_to_sel_mode(0xffffffff);
    }
    break;
  case mkb::PMT_UNKNOWN3:
    if (mkb::g_current_focused_pause_menu_entry == 2) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 2) {
      if (mkb::g_current_focused_pause_menu_entry == 0) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
      }
      else if (0 < mkb::g_current_focused_pause_menu_entry) {
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::mode_flags = mkb::mode_flags | 0x4000;
        mkb::g_fade_track_volume(100,'\n');
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry < 4) {
      mkb::g_some_pausemenu_var = 1;
      mkb::g_return_to_sel_mode(0xffffffff);
    }
    break;
  case mkb::PMT_UNKNOWN4:
    if (mkb::g_current_focused_pause_menu_entry == 2) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 2) {
      if (mkb::g_current_focused_pause_menu_entry == 0) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
        mkb::g_some_status_bitflag = mkb::g_some_status_bitflag | 0x20;
      }
      else if (0 < mkb::g_current_focused_pause_menu_entry) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
        mkb::g_some_status_bitflag = mkb::g_some_status_bitflag | 0x20;
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry < 4) {
      mkb::g_some_pausemenu_var = 1;
      mkb::g_return_to_sel_mode(0xffffffff);
    }
    break;
  case mkb::PMT_STORY_STAGE_SELECT:
    if (mkb::g_current_focused_pause_menu_entry == 2) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 2) {
      if (mkb::g_current_focused_pause_menu_entry == 0) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
      }
      else if (0 < mkb::g_current_focused_pause_menu_entry) {
        mkb::g_save_storymode_progress((void *)0x0);
        mkb::g_some_pausemenu_var = 8;
        mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
        if (mkb::g_some_pausemenu_var2 == '\0') {
          mkb::g_some_pausemenu_var2 = '\x01';
        }
        mkb::g_some_pausemenu_var3 = 0;
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry < 4) {
      mkb::g_save_storymode_progress((void *)0x0);
      mkb::mode_flags = mkb::mode_flags | 0x8000000;
      mkb::g_some_pausemenu_var = 5;
      mkb::sub_mode_request = mkb::SMD_GAME_INTR_SEL_INIT;
    }
    break;
  case mkb::PMT_STORY_PLAY:
    if (mkb::g_current_focused_pause_menu_entry == 3) {
      mkb::g_some_pausemenu_var = 4;
      if (param_1 != 0) {
        *(mkb::undefined4 *)(param_1 + 0x4c) = 6;
      }
      if ((mkb::main_mode == mkb::MD_MINI) && (mkb::main_game_mode == mkb::MONKEY_GOLF)) {
        OVar1 = mkb::OSSetCurrentHeap(mkb::stage_heap);
        mkb::load_bmp_by_id(0xc);
        mkb::OSSetCurrentHeap(OVar1);
      }
      else {
        mkb::load_bmp_by_id(0xc);
      }
      mkb::g_create_how_to_sprite();
      mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 3) {
      if (mkb::g_current_focused_pause_menu_entry == 1) {
        mkb::mode_info.attempt_count = mkb::mode_info.attempt_count + 1;
        ball = mkb::balls;
        for (ball_index = 0; ball_index < 8; ball_index = ball_index + 1) {
          if (ball->status == mkb::STAT_NORMAL) {
            ball->phys_flags = ball->phys_flags;
            ball->g_effect_flags = ball->g_effect_flags | 0x80;
          }
          ball = ball + 1;
        }
        mkb::sub_mode_request = mkb::SMD_GAME_READY_INIT;
        mkb::g_fade_track_volume(100,'\n');
      }
      else if (mkb::g_current_focused_pause_menu_entry < 1) {
          mkb::g_some_pausemenu_var = 0;
          mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
          mkb::g_fade_track_volume(100,'\n');
      }
      else if ((mkb::g_some_status_bitflag & 4) == 0) {
        mkb::g_some_pausemenu_var = 2;
        mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
        mkb::event_init(mkb::EVENT_VIEW);
        if (param_1 != 0) {
          *(mkb::uint *)(param_1 + 0x8c) = *(mkb::uint *)(param_1 + 0x8c) | 1;
        }
      }
      else {
        mkb::g_some_pausemenu_var = 3;
        mkb::g_some_other_flags = mkb::g_some_other_flags | mkb::OF_GAME_PAUSED;
        mkb::g_some_replay_func(&local_158,&local_154);
        mkb::g_some_replay_func2(abStack_140);
        mkb::g_some_replay_func3(abStack_140,acStack_d4);
        if (local_158 != (void *)0x0) {
          mkb::memset(local_11c,0,0x18);
          local_11c[0] = 2;
          local_108 = acStack_d4;
          local_114 = (int)local_158 + 0x24;
          local_10c = local_154;
          local_110 = local_158;
          mkb::g_some_replay_func4(0,4,local_11c);
        }
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry == 5) {
      mkb::g_save_storymode_progress((void *)0x0);
      mkb::mode_flags = mkb::mode_flags | 0x8000000;
      mkb::g_some_pausemenu_var = 5;
      mkb::sub_mode_request = mkb::SMD_GAME_INTR_SEL_INIT;
    }
    else if (mkb::g_current_focused_pause_menu_entry < 5) {
      mkb::g_some_pausemenu_var = 6;
      mkb::mode_info.ball_mode = mkb::mode_info.ball_mode & ~mkb::BALLMODE_IN_REPLAY;
      mkb::main_mode_request = mkb::MD_GAME;
      mkb::sub_mode_request = mkb::SMD_GAME_SCENARIO_RETURN;
      if (mkb::stage_complete) {
        goal_score = mkb::get_goal_score((mkb::uint *)0x0,(int *)0x0);
        mkb::set_storymode_score(mkb::balls[ball_index].score + goal_score);
        mkb::set_storymode_bananas(mkb::balls[ball_index].banana_count);
        ball_index = mkb::g_get_next_stage_id();
        if (-1 < ball_index) {
          mkb::play_track_and_fade_out_other_tracks(0,0x3c,100);
        }
      }
      else {
        mkb::play_track_and_fade_out_other_tracks(0,0x3c,100);
      }
    }
    break;
  case mkb::PMT_UNKNOWN7:
    if (mkb::g_current_focused_pause_menu_entry == 1) {
      mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
      mkb::g_some_pausemenu_var4 = 1;
      mkb::g_fade_track_volume(100,'\n');
    }
    else if (mkb::g_current_focused_pause_menu_entry < 1) {
      if (0 < mkb::g_current_focused_pause_menu_entry) {
        mkb::g_some_pausemenu_var = 0;
        mkb::destroy_sprite_with_unique_id(mkb::SPRITE_PAUSE_MENU);
        mkb::g_fade_track_volume(100,'\n');
      }
    }
    else if (mkb::g_current_focused_pause_menu_entry < 3) {
      mkb::g_some_pausemenu_var = 1;
      mkb::g_return_to_sel_mode(0xffffffff);
    }
  }
  return;
}

// Clears the per-player death counter, then nops the instruction that
// decrements the life counter on player death and the instruction that
// grants 1UPs. Then, hooks into the decrement life counter function,
// and runs the update_death_count func. Then, hooks into the monkey
// counter sprite tick function, and calls the death counter sprite
// tick function instead.
void init_main_loop() {
    patch::write_branch(reinterpret_cast<void*>(mkb::handle_pausemenu_selection),
                        reinterpret_cast<void*>(new_handle_pausemenu_selection));
    patch::write_word(reinterpret_cast<void*>(0x8047f1dc), 0x8047ee80);
    patch::write_word(reinterpret_cast<void*>(0x8047f1f4), 0x8047eda4);
    patch::write_word(reinterpret_cast<void*>(0x8047f20c), 0x8047edf4);
    patch::write_word(reinterpret_cast<void*>(0x8047f4a8), 0x8047f1ac);
    mkb::pausemenu_entry_counts[1] = 5;
}

}// namespace challenge_mode_retry
