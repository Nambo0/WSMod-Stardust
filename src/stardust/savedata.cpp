#include "savedata.h"
#include "../internal/cardio.h"
#include "../internal/draw.h"
#include "../internal/heap.h"
#include "../mkb/mkb.h"
#include "../internal/log.h"

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
    // mkb::OSReport("[stardust] Saving to slot [%d].\n", cardio::get_slot());
    if (cardio::get_slot() == cardio::Slot::None) return;
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

// world indexes are 1-10 (not 0-9)
static bool world_cleared(u8 world) {
    // check cutscene first
    if (mkb::unlock_info_ptr->g_movies_watched & (1 << (world))) { return true; }
    // otherwise check
    for (u8 k = 0; k < 10; k++) {
        u32 clear_achieved = read_bool_from_array(savedata, CLEAR_BADGE_START + ((world - 1) * 10) + k);
        u32 stunt_achieved = read_bool_from_array(savedata, STUNT_BADGE_START + ((world - 1) * 10) + k);
        if (!(clear_achieved || stunt_achieved)) {
            return false;
        }
    }
    return true;
}

u32 latest_played_world() {
    for (u8 world = 10; world > 1; world--) {
        bool prev_world_complete = world_cleared(world - 1);
        bool clear_badge_in_curr_world = !consecutive_false_from_slot(savedata::CLEAR_BADGE_START + ((world - 1) * 10), 10);
        bool stunt_badge_in_curr_world = !consecutive_false_from_slot(savedata::STUNT_BADGE_START + ((world - 1) * 10), 10);

        if (prev_world_complete || clear_badge_in_curr_world || stunt_badge_in_curr_world) {
            return world;
        }
    }

    return 1;
}

bool consecutive_true_from_slot(u16 slot, u16 count) {
    for (int i = slot; i < slot + count; i++) {
        if (!read_bool_from_array(savedata, i)) return false;
    }
    return true;
}

bool consecutive_false_from_slot(u16 slot, u16 count) {
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

u16 get_stellar_level(u8 level) {
    return savedata[50 + level - 1];
}

void erase_all_data() {
    for (u8 i = 0; i < 60; i++) {
        savedata[i] = 0;
    }
}

void write_bool_to_slot(u16 slot, bool value) {
    write_bool_to_array(savedata, slot, value);
}

bool beat_the_game_override = false;

void debug_display_mode() {
    cardio::set_slot(cardio::Slot::None);
    erase_all_data();
    // Remove Beat the Game achievement
    beat_the_game_override = true;
}

bool is_debug_display_mode() {
    return beat_the_game_override;
}
// =============================================================================================

void init() {
    // first try reading a pre-existing savefile from Slot A
    FileHeader* header = nullptr;
    s32 result_A = cardio::read_file(cardio::Slot::A, FILENAME, reinterpret_cast<void**>(&header));
    if (result_A == mkb::CARD_RESULT_READY) {
        to_array(header);
        heap::free(header);
        // mkb::OSReport("[stardust] Savedata loaded from Slot A!\n");
        cardio::set_slot(cardio::Slot::A);
        return;
    }

    // pre-existing save wasn't found, try reading a savefile from Slot B
    header = nullptr;
    s32 result_B = cardio::read_file(cardio::Slot::B, FILENAME, reinterpret_cast<void**>(&header));
    if (result_B == mkb::CARD_RESULT_READY) {
        to_array(header);
        heap::free(header);
        // mkb::OSReport("[stardust] Savedata loaded from Slot B!\n");
        cardio::set_slot(cardio::Slot::B);
        return;
    }

    /* OLD SAVE CODE
    // pre-existing save files weren't found on either memcards
    // try creating a new savefile on Slot A
    if (result_A == mkb::CARD_RESULT_NOFILE) {
        // mkb::OSReport("[stardust] Creating new save in Slot A.\n");
        cardio::set_slot(cardio::Slot::A);
        save();
        return;
    }

    // card A had some other error, try creating the new savefile on Slot B
    if (result_B == mkb::CARD_RESULT_NOFILE) {
        // mkb::OSReport("[stardust] Creating new save in Slot B.\n");
        cardio::set_slot(cardio::Slot::B);
        save();
        return;
    }
    */
    // pre-existing save files weren't found on either memcards
    // try creating a new savefile on Slot A
    if (result_A == mkb::CARD_RESULT_NOFILE) {
        update_special_bools();
        to_buffer();
        // temporarily set slot to A to see if saving works
        cardio::set_slot(cardio::Slot::A);
        cardio::write_file(FILENAME, card_buffer, sizeof(card_buffer),
        [](mkb::CARDResult res) -> void {
            // slot A save finished
            if (res != mkb::CARD_RESULT_READY) {
                //slot A save failed, try saving to slot B
                cardio::set_slot(cardio::Slot::B);
                cardio::write_file(FILENAME, card_buffer, sizeof(card_buffer),
                [](mkb::CARDResult res2) -> void {
                    // slot B save finished
                    if (res2 != mkb::CARD_RESULT_READY) {
                        // slot B save failed :(
                        cardio::set_slot(cardio::Slot::None);
                        LOG("Unable to create savedata :(");
                    }
                });
            }
        });

        return;
    }

    // something wrong happened with both memcards :(
    // mkb::OSReport("[stardust] Error loading savedata. Slot A Error [%d]. Slot B Error [%d].\n", result_A, result_B);
}

}// namespace savedata
