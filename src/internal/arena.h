#pragma once

#include "mkb.h"

namespace arena {

class Arena {
 public:
    void init(const char* name, void* start, u32 size);
    void reset();
    void* alloc(u32 size);

    template <typename T>
    T* alloc() {
        return static_cast<T*>(alloc(sizeof(T)));
    }

    template <typename T>
    T* alloc(u32 elem_count) {
        return static_cast<T*>(alloc(sizeof(T) * elem_count));
    }

    void* get_start() { return m_start; }
    void* get_pos() { return reinterpret_cast<void*>(reinterpret_cast<u32>(m_start) + m_occupied); }
    u32 get_occupied() { return m_occupied; }
    u32 get_free() { return m_capacity - m_occupied; }

 private:
    const char* m_name;
    void* m_start = nullptr;
    u32 m_capacity = 0;
    u32 m_occupied = 0;
};

}  // namespace arena

