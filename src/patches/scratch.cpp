#include "scratch.h"
#include "internal/heap.h"
#include "internal/log.h"
#include "internal/pad.h"
#include "../internal/patch.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/ui/widget_container.h"
#include "internal/ui/widget_text.h"
#include "internal/ui/widget_window.h"
#include "widget_button.h"
#include "widget_sprite.h"

namespace scratch {

TICKABLE_DEFINITION((
        .name = "scratch",
        .description = "Scratch",
        .enabled = true,
        .init_main_loop = init,
        .tick = tick))

bool open_galactic_log = false;

void init() {
}

void tick() {
    if (open_galactic_log) {
        open_galactic_log = false;

        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);
        //sent = true;
    }

    if (pad::button_pressed(mkb::PAD_BUTTON_Y)) {
        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);
        ui::get_widget_manager().clear();
        LOG_DEBUG("free: %dkb", heap::get_free_space() / 1024);
        //sent = false;
    }

    /*
    if (base_ptr) {
        base_ptr->tick();
    }*/
}

void disp() {}

}// namespace scratch
