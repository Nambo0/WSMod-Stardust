#include "config/config.h"
#include "internal/assembly.h"
#include "internal/cardio.h"
#include "internal/heap.h"
#include "internal/log.h"
#include "internal/modlink.h"
#include "internal/patch.h"
#include "internal/tickable.h"
#include "internal/ui/ui_manager.h"
#include "internal/version.h"
#include "internal/mem.h"
#include "mkb/mkb.h"
#include "relutil.h"
#include "stardust/savedata.h"

namespace main {
static patch::Tramp<decltype(&mkb::mode_tick)> s_mode_tick_tramp;

bool debug_mode_enabled = false;

static void perform_assembly_patches() {
    // Inject the run function at the start of the main game loop
    patch::write_branch_bl(reinterpret_cast<void*>(0x80270700),
                           reinterpret_cast<void*>(start_main_loop_assembly));

    /* Remove OSReport call ``PERF : event is still open for CPU!``
since it reports every frame, and thus clutters the console */
    // Only needs to be applied to the US version
    patch::write_nop(reinterpret_cast<void*>(0x80033E9C));

    // Nop the conditional that guards `draw_debugtext`, enabling it even when debug mode is disabled
    patch::write_nop(reinterpret_cast<void*>(0x80299f54));
}

static constexpr u32 WSMOD_HEAP_SIZE = 1024 * 80;

static void init_memory_model() {
    // Allocate wsmod arena from available mainloop REL space
    u32 start = mkb::OSRoundUp32B(
        *reinterpret_cast<u32*>(0x8000452C));  // Set by REL loader to region after wsmod REL/BSS
    void* end_ptr = relutil::compute_mainloop_reldata_boundary(reinterpret_cast<void*>(start));
    u32 end = mkb::OSRoundDown32B(reinterpret_cast<u32>(end_ptr));
    u32 size = end - start;
    mem::wsmod_arena.init("wsmod", reinterpret_cast<void*>(start), size);

    // Allocate separate heap for wsmod
    void* wsmod_heap_start = mem::wsmod_arena.alloc(WSMOD_HEAP_SIZE);
    mem::wsmod_heap.init(wsmod_heap_start, WSMOD_HEAP_SIZE);

    // Allocate chainload heap from remaining wsmod arena
    u32 chainload_heap_size = mem::wsmod_arena.get_free();
    void* chainload_heap_start = mem::wsmod_arena.alloc(chainload_heap_size);
    mem::chainload_heap.init(chainload_heap_start, chainload_heap_size);
}

void init() {
    LOG("SMB2 Workshop Mod v%d.%d.%d loaded",
        version::WSMOD_VERSION.major,
        version::WSMOD_VERSION.minor,
        version::WSMOD_VERSION.patch);

    init_memory_model();
    cardio::init();
    modlink::write();

    perform_assembly_patches();

    // Load our config file
    config::parse_config();

    // Init all tickables/patches
    tickable::get_tickable_manager().init();

    patch::hook_function(
        s_mode_tick_tramp, mkb::mode_tick, []() {
            // These run after process_inputs has been ran to minimize input delay.
            // It runs after pracmod sets s_exclusive_mode though
            // (so practice mod can still override the pack's pad inputs)
            cardio::tick();

            // Tick functions (REL patches)
            for (const auto& tickable: tickable::get_tickable_manager().get_tickables()) {
                if (tickable->enabled && tickable->tick) {
                    tickable->tick();
                }
            }

            ui::get_widget_manager().tick();
            // after all our code has ran, run the current mode's tick function
            s_mode_tick_tramp.dest();
        });

    savedata::init();
}

/*
 * This runs at the very start of the main game loop. Most per-frame code runs after
 * controller inputs have been read and processed however, to ensure the lowest input delay.
 */
void tick() {
}

}// namespace main
