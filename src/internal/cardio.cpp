#include "cardio.h"
#include "heap.h"
#include "mkb.h"
#include "modlink.h"
#include <optional>

namespace cardio {

// Corresponds to CARD call we're waiting for
enum class WriteState {
    Idle,
    Mount,
    Create,// If memcard file doesn't exist
    Delete,// Else, it exists but it's too small, delete and create from scratch
    Write,
};


struct WriteParams {
    const char* file_name;
    const void* buf;
    u32 buf_size;
    void (*callback)(mkb::CARDResult);
};

static Slot s_current_slot = Slot::None;

// We need a 40KB(!) buffer just for the privilege of accessing memory cards,
// this sucks! Reminder we only have ~550KB to work with for the entire mod,
// including savestates
static void* s_card_work_area;
static mkb::CARDFileInfo s_card_file_info;

static WriteState s_state = WriteState::Idle;
static WriteParams s_write_params;// Current params
static std::optional<WriteParams>
    s_write_request;// Params for use for next write
static u32
    s_write_size;// Sector size of memory card A which we read when probing it

void set_slot(Slot slot) {
    s_current_slot = slot;
}

Slot get_slot() {
    return s_current_slot;
}

static mkb::CARDResult read_file_internal(const Slot slot, const char* file_name,
                                          void** out_buf) {
    mkb::CARDResult res = mkb::CARD_RESULT_BUSY;

    // Probe and mount card
    do {
        res = mkb::CARDProbeEx(static_cast<s32>(slot), nullptr, nullptr);
        if (res != mkb::CARD_RESULT_READY && res != mkb::CARD_RESULT_BUSY) return res;
    } while (res == mkb::CARD_RESULT_BUSY);

    mkb::CARDMountAsync(static_cast<s32>(slot), s_card_work_area, nullptr, nullptr);
    do {
        res = mkb::CARDGetResultCode(static_cast<s32>(slot));
    } while (res == mkb::CARD_RESULT_BUSY);
    if (res != mkb::CARD_RESULT_READY) {
        return res;
    }

    // Open file
    res = mkb::CARDOpen(static_cast<s32>(slot), const_cast<char*>(file_name), &s_card_file_info);
    if (res != mkb::CARD_RESULT_READY) {
        mkb::CARDUnmount(static_cast<s32>(slot));
        return res;
    }

    // Get file size
    mkb::CARDStat stat;
    res = mkb::CARDGetStatus(static_cast<s32>(slot), s_card_file_info.fileNo, &stat);
    if (res != mkb::CARD_RESULT_READY) {
        mkb::CARDUnmount(static_cast<s32>(slot));
        return res;
    }

    u32 buf_size =
        (stat.length + mkb::CARD_READ_SIZE - 1) & ~(mkb::CARD_READ_SIZE - 1);
    void* buf = heap::alloc(buf_size);
    if (buf == nullptr) {
        // Not quite the right error (we're out of memory, not out of card space)
        mkb::CARDUnmount(static_cast<s32>(slot));
        return mkb::CARD_RESULT_INSSPACE;
    }

    mkb::CARDReadAsync(&s_card_file_info, buf, buf_size, 0, nullptr);
    do {
        res = mkb::CARDGetResultCode(static_cast<s32>(slot));
    } while (res == mkb::CARD_RESULT_BUSY);
    if (res != mkb::CARD_RESULT_READY) {
        heap::free(buf);
        mkb::CARDUnmount(static_cast<s32>(slot));
        return res;
    }

    *out_buf = buf;
    return mkb::CARD_RESULT_READY;
}

mkb::CARDResult read_file(const Slot slot, const char* file_name, void** out_buf) {
    mkb::CARDResult res = read_file_internal(slot, file_name, out_buf);
    return res;
}

void write_file(const char* file_name, const void* buf, u32 buf_size,
                void (*callback)(mkb::CARDResult)) {
    s_write_request.emplace(WriteParams{file_name, buf, buf_size, callback});
}

static void finish_write(mkb::CARDResult res) {
    mkb::CARDUnmount(
        static_cast<u32>(s_current_slot));// I'm assuming that trying to unmount when mounting failed is OK
    s_write_params.callback(res);
    s_state = WriteState::Idle;
}

void init() {
    // Artificial delay so that the memcard has time to initialize
    // Probably not an issue on console, but if read speed emulation is disabled in Dolphin, this can cause issues
    // auto current_tick = mkb::OSGetTick();
    // auto end_tick = current_tick + mkb::BUS_CLOCK_SPEED/32; // 0.125s delay
    // while (current_tick < end_tick) {
    //     current_tick = mkb::OSGetTick();
    // }

    s_card_work_area = heap::alloc(mkb::CARD_WORKAREA_SIZE);
    modlink::set_card_work_area(s_card_work_area);
}

void tick() {
    mkb::CARDResult res;

    switch (s_state) {
        case WriteState::Idle: {
            if (s_write_request.has_value()) {
                // Kick off write operation
                s_write_params = s_write_request.value();
                s_write_request.reset();

                // Probe and begin mounting card A
                s32 sector_size;
                do {
                    res = mkb::CARDProbeEx(static_cast<u32>(s_current_slot), nullptr, &sector_size);
                    if (res != mkb::CARD_RESULT_READY && res != mkb::CARD_RESULT_BUSY) finish_write(res);
                } while (res == mkb::CARD_RESULT_BUSY);

                s_write_size =
                    (s_write_params.buf_size + sector_size - 1) & ~(sector_size - 1);
                mkb::CARDMountAsync(static_cast<u32>(s_current_slot), s_card_work_area, nullptr, nullptr);
                s_state = WriteState::Mount;
            }
            break;
        }

        case WriteState::Mount: {
            res = mkb::CARDGetResultCode(static_cast<u32>(s_current_slot));
            if (res != mkb::CARD_RESULT_BUSY) {
                if (res == mkb::CARD_RESULT_READY) {
                    // Try to open the file
                    res = mkb::CARDOpen(static_cast<u32>(s_current_slot), const_cast<char*>(s_write_params.file_name),
                                        &s_card_file_info);
                    if (res == mkb::CARD_RESULT_READY) {
                        // Check if file is too small
                        mkb::CARDStat stat;
                        res = mkb::CARDGetStatus(static_cast<u32>(s_current_slot), s_card_file_info.fileNo, &stat);
                        if (res != mkb::CARD_RESULT_READY) {
                            finish_write(res);
                        }
                        else if (stat.length < s_write_size) {
                            // Recreate file
                            mkb::CARDFastDeleteAsync(static_cast<u32>(s_current_slot), s_card_file_info.fileNo, nullptr);
                            s_state = WriteState::Delete;
                        }
                        else {
                            // Card opened successfully, proceed directly to writing
                            mkb::CARDWriteAsync(&s_card_file_info,
                                                const_cast<void*>(s_write_params.buf),
                                                s_write_size, 0, nullptr);
                            s_state = WriteState::Write;
                        }
                    }
                    else if (res == mkb::CARD_RESULT_NOFILE) {
                        // Create new file
                        mkb::CARDCreateAsync(static_cast<u32>(s_current_slot), const_cast<char*>(s_write_params.file_name),
                                             s_write_size, &s_card_file_info, nullptr);
                        s_state = WriteState::Create;
                    }
                    else {
                        // Some other error, fail entire write operation
                        finish_write(res);
                    }
                }
                else {
                    // Error mounting
                    finish_write(res);
                }
            }
            break;
        }

        case WriteState::Create: {
            res = mkb::CARDGetResultCode(static_cast<u32>(s_current_slot));
            if (res != mkb::CARD_RESULT_BUSY) {
                if (res == mkb::CARD_RESULT_READY) {
                    mkb::CARDWriteAsync(&s_card_file_info,
                                        const_cast<void*>(s_write_params.buf),
                                        s_write_size, 0, nullptr);
                    s_state = WriteState::Write;
                }
                else {
                    finish_write(res);
                }
            }
            break;
        }

        case WriteState::Delete: {
            res = mkb::CARDGetResultCode(static_cast<u32>(s_current_slot));
            if (res != mkb::CARD_RESULT_BUSY) {
                if (res == mkb::CARD_RESULT_READY) {
                    mkb::CARDCreateAsync(static_cast<u32>(s_current_slot), const_cast<char*>(s_write_params.file_name),
                                         s_write_size, &s_card_file_info, nullptr);
                    s_state = WriteState::Create;
                }
                else {
                    finish_write(res);
                }
            }
            break;
        }

        case WriteState::Write: {
            res = mkb::CARDGetResultCode(static_cast<u32>(s_current_slot));
            if (res != mkb::CARD_RESULT_BUSY) {
                // Either succeeded or failed, either way we're done
                finish_write(res);
            }
            break;
        }
    }
}

}// namespace cardio
