#pragma once

#include "../mkb/mkb.h"

namespace cardio {

enum class Slot : s32 {
    A = 0,
    B = 1,
    None = -1,
};

void init();
void tick();

void set_slot(Slot slot);
Slot get_slot();

// Caller gets a heap-allocated buffer containing file, which they own
// Synchronous at the moment. Also, do not call while write_file() is running!
mkb::CARDResult read_file(const Slot slot, const char* file_name, void** out_buf);

// Writes asynchronously
void write_file(const char* file_name, const void* buf, u32 buf_size,
                void (*callback)(mkb::CARDResult));

}// namespace cardio
