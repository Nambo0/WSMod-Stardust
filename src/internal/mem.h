#pragma once

#include "arena.h"
#include "heap.h"

//
// wsmod-wide dynamic memory model, consisting of arenas and heaps
//

namespace mem {
    // Arena spanning the remaining mainloop.rel relocation data free space.
    extern arena::Arena wsmod_arena;

    // Separate heap for wsmod's dynamic allocations. Allocated at the start of wsmod_arena.
    extern heap::Heap wsmod_heap;

    // Heap allocated at end of remaining space in wsmod arena. This is what additional chainloaded
    // mods (like Practice Mod) are loaded in and provided as their own heap.
    extern heap::Heap chainload_heap;
}

