#include "mem.h"

void* operator new(u32 size) {
    return mem::wsmod_heap.alloc(size);
}

void* operator new[](u32 size) {
    return mem::wsmod_heap.alloc(size);
}

void operator delete(void* ptr) {
    mem::wsmod_heap.free(ptr);
}

void operator delete[](void* ptr) {
    mem::wsmod_heap.free(ptr);
}

void operator delete(void* ptr, u32 size) {
    mem::wsmod_heap.free(ptr);
}

void operator delete[](void* ptr, u32 size) {
    mem::wsmod_heap.free(ptr);
}
