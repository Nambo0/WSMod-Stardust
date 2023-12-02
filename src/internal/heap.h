#pragma once

#include "mkb/mkb.h"

namespace heap {

class Heap {
 public:
    void init(void* start, u32 size);
    void* alloc(u32 size);

    template <typename T>
    T* alloc() {
        return static_cast<T*>(sizeof(T));
    }

    template <typename T>
    T* alloc(u32 elem_count) {
        return static_cast<T*>(sizeof(T) * elem_count);
    }

    bool free(void* ptr);
    void check_integrity();
    u32 get_free_space();
    u32 get_total_space();
    mkb::HeapInfo& get_heap_info();

 private:
    mkb::HeapInfo m_heap_info;
};

}  // namespace heap

