#include "arena.h"

#include "log.h"

// TODO default to 4-byte alignment

namespace arena {

void Arena::init(const char* name, void* start, u32 size) {
    m_name = name;
    m_start = start;
    m_capacity = size;
    reset();
}

void Arena::reset() {
    m_occupied = 0;
    mkb::memset(m_start, 0, m_capacity);
}

void* Arena::alloc(u32 size) {
    u32 new_occupied = m_occupied + size;
    if (new_occupied > m_capacity) {
        // Not ideal way to log this
        LOG("%s arena out of memory", m_name);
        MOD_ASSERT(new_occupied <= m_capacity);
    }
    void* ret = reinterpret_cast<void*>(reinterpret_cast<u32>(m_start) + m_occupied);
    m_occupied = new_occupied;
    return ret;
}

}  // namespace arena

