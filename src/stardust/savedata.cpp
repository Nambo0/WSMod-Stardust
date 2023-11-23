#include "savedata.h"
#include "../internal/cardio.h"
#include "../internal/draw.h"
#include "../internal/heap.h"
#include "../mkb/mkb.h"

namespace savedata {

/* SAVEDATA u8 ARRAY FORMAT:
60 u8’s = 480 bits
000-099 = clear badges
100-199 = stunt badges
200-299 = sweep badges
300-309 = stage challenges
310-379 = other achievements
380-399 = misc bools
400-479 = 10 u8’s for interstellar best run (starts at u8 #50)

Specific Values:
380 = Widescreen (for widescreen_title_fix)
*/


static u8 savedata[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct FileHeader {
    char magic[4];// STAR
    u16 version;
} __attribute__((__packed__));

static u8 card_buffer[sizeof(FileHeader) + (sizeof(u8)) * 60]
    __attribute__((__aligned__(32)));// CARD API requires 32-byte alignment

static constexpr char* FILENAME = "star";

// convert memory card buffer to stardust save struct
static void to_array(void* buffer) {
    FileHeader* header = static_cast<FileHeader*>(buffer);
    if (header->version > 1) { return; }

    u8* value_list = reinterpret_cast<u8*>(buffer) + sizeof(FileHeader);
    for (u32 i = 0; i < 60; i++) {
        savedata[i] = value_list[i];
    }
}

// convert stardust save struct to memory card buffer
static void to_buffer() {
    FileHeader* header = static_cast<FileHeader*>(static_cast<void*>(card_buffer));

    u8* value_list = reinterpret_cast<u8*>(card_buffer) + sizeof(FileHeader);

    header->magic[0] = 'S';
    header->magic[1] = 'T';
    header->magic[2] = 'A';
    header->magic[3] = 'R';
    header->version = 1;

    for (u32 i = 0; i < 60; i++) {
        value_list[i] = savedata[i];
    }
}

// load save file with no completions
void load_default_save() {
    for (u32 i = 0; i < 60; i++) {
        savedata[i] = 0x00;
    }
}

void update_special_bools() {
    if (mkb::widescreen_mode == mkb::NORMAL) {
        write_bool_to_slot(savedata::WIDESCREEN_MODE, false);
    }
    else if (mkb::widescreen_mode != mkb::NORMAL) {
        write_bool_to_slot(savedata::WIDESCREEN_MODE, true);
    }
}

void save() {
    update_special_bools();
    to_buffer();
    cardio::write_file(FILENAME, card_buffer, sizeof(card_buffer),
                       [](mkb::CARDResult res) -> void {
                           // error handling
                       });
}

// =============================================================================================

// Functions for reading/writing the array
bool read_bool_from_array(u8* array, u16 slot) {
    u32 array_index = slot / 8;
    u32 bit_offset = slot % 8;
    if (array[array_index] & (1 << bit_offset)) return true;
    else return false;
}

void write_bool_to_array(u8* array, u16 slot, bool value_to_write) {
    u32 array_index = slot / 8;
    u32 bit_offset = slot % 8;
    if (value_to_write) {
        array[array_index] |= 1 << bit_offset;
    }
    else {
        array[array_index] &= ~(1 << bit_offset);
    }
}

// Functions for reading/writing the array
bool true_in_slot(u16 slot) {
    return read_bool_from_array(savedata, slot);
}

bool consecutive_true_from_slot(u16 slot, u16 count) {
    for (int i = slot; i < slot + count; i++) {
        if (!read_bool_from_array(savedata, i)) return false;
    }
    return true;
}

bool consecutive_false_from_slot(u16 slot, u16 count){
    for (int i = slot; i < slot + count; i++) {
        if (read_bool_from_array(savedata, i)) return false;
    }
    return true;
}

void update_stellar_bunch_counts(u8* bunch_counts) {
    for (u8 i = 0; i < 10; i++) {
        savedata[50 + i] = bunch_counts[i];
    }
}

u16 stellar_best_run_total() {
    u16 total = 0;
    for (u8 i = 0; i < 10; i++) {
        total += savedata[50 + i];
    }
    return total;
}

u8 best_stellar_rank() {
    switch (stellar_best_run_total()) {
        case 0 ... 99:
            return 0;
        case 100 ... 199:
            return 1;
        case 200 ... 299:
            return 2;
        case 300 ... 399:
            return 3;
        case 400 ... 499:
            return 4;
    }
    return 5;
}

void write_bool_to_slot(u16 slot, bool value) {
    write_bool_to_array(savedata, slot, value);
}
// =============================================================================================

s32 init() {
    FileHeader* header = nullptr;
    s32 result = cardio::read_file(FILENAME, reinterpret_cast<void**>(&header));
    if (result == mkb::CARD_RESULT_READY) {
        mkb::OSReport("[stardust] Savedata loaded!");
        to_array(header);
        heap::free(header);
    }
    else {
        mkb::OSReport("[stardust] Error loading savedata. Error type %d", result);
    }

    return result;
}

}// namespace savedata
