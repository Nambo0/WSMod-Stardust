#include "pausecooldown.h"

#include "internal/patch.h"
#include "internal/tickable.h"
#include "mkb/mkb.h"

namespace pausecooldown {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-pause-cooldown",
        .description = "Pause cooldown",
        .enabled = true,
        .init_main_loop = init, ))

static patch::Tramp<decltype(&mkb::sprite_pausemenu_disp)> s_sprite_pausemenu_disp_tramp;

static void limit_pauses() {
    if (mkb::unlock_info.g_movies_watched != 0x0fff) {// Story mode NOT completed (Unlocks with Pracc Mod Unlock)
        mkb::g_repause_cooldown_counter = 60;
    }
}

void init() {
    patch::hook_function(s_sprite_pausemenu_disp_tramp, mkb::sprite_pausemenu_disp, [](mkb::Sprite* sprite) {
        s_sprite_pausemenu_disp_tramp.dest(sprite);
        limit_pauses();
    });
}

}// namespace pausecooldown