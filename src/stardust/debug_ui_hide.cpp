#include "debug_ui_hide.h"

#include "../internal/pad.h"
#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"

namespace debug_ui_hide {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-debug-ui-hide",
        .description = "Debug UI Hide",
        .enabled = true,
        .init_main_loop = init, ))

bool is_debug_ui_hide_mode = false;

void toggle_debug_ui_hide_mode() {
    if(is_debug_ui_hide_mode) is_debug_ui_hide_mode = false;
    else is_debug_ui_hide_mode = true;
}

/*
void make_sprite_transparent(mkb::Sprite * sprite) {
    sprite->alpha = 0.35;
}
*/

// void sprite_timer_ball_tick(u8 * status, struct Sprite * sprite);
// static patch::Tramp<decltype(&mkb::sprite_timer_ball_tick)> s_sprite_timer_ball_tick_tramp;
// void draw_sprite(struct Sprite * sprite);
static patch::Tramp<decltype(&mkb::draw_sprite)> s_draw_sprite_tramp;

void init() {
    patch::hook_function(s_draw_sprite_tramp, mkb::draw_sprite, [](mkb::Sprite* sprite) {
        // make every sprite transparent, except the pause menu
        bool correct_mode = mkb::main_mode == mkb::MD_GAME;
        bool is_pausemenu_sprite = sprite->disp_func == mkb::sprite_pausemenu_disp;
        if (!(is_debug_ui_hide_mode && correct_mode && !is_pausemenu_sprite && pad::button_down(mkb::PAD_TRIGGER_R))) {
            s_draw_sprite_tramp.dest(sprite);
        }
    });
}

}// namespace debug_ui_hide