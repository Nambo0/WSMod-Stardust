#include "modlink.h"

#include "heap.h"
#include "version.h"

namespace modlink {

static constexpr u32 MODLINK_ADDR = 0x800a9cb4;
static constexpr u32 MAGIC = 0xFEEDC0DE;

static ModLinkPart2 s_part2 = {};

void write() {
    ModLink* link = reinterpret_cast<ModLink*>(MODLINK_ADDR);
    link->magic = MAGIC;
    link->modlink_version = {1, 1, 0};
    link->wsmod_version = version::WSMOD_VERSION;
    link->malloc_func = heap::alloc;
    link->heap_info = &heap::get_heap_info();
    link->part2 = &s_part2;
}

void set_card_work_area(void* buf) {
    s_part2.card_work_area = buf;
}

}// namespace modlink