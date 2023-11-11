#include "bunch_count.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../mkb/mkb.h"

namespace a {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-bunch-count",
        .description = "Bunch Count",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick, ))

static u8 bunches_collected = 0;
static u8 bunches_total = 0;

static void count_bunches() {
    bunches_collected = 0;
    bunches_total = 0;
    for (u32 i = 0; i < mkb::item_pool_info.upper_bound; i++) {
        if (mkb::item_pool_info.status_list[i] == 0) continue;                                       // skip if its inactive
        mkb::Item& item = mkb::items[i];                                                             // shorthand: current item in the list = "item"
        if (item.coin_type != 1) continue;                                                           // skip if its not a bunch
        bunches_total++;
        if (item.g_some_flag == 0 && item.g_some_bitfield & 1 && item.g_some_bitfield & 0xfffffffd) {// True if banana is gone
            bunches_collected++;
        }
    }
}

static void display_counter(){
    // For euc <3
}

void tick() {
    display_counter();
}

static patch::Tramp<decltype(&mkb::item_coin_disp)> s_item_coin_disp_tramp;

void init() {
    patch::hook_function(s_item_coin_disp_tramp, mkb::item_coin_disp, [](mkb::Item* item) {
        s_item_coin_disp_tramp.dest(item);
        count_bunches();
    });
}

}// namespace bunch_count