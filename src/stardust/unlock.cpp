#include "unlock.h"

#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "mkb/mkb.h"
#include "../stardust/savedata.h"
#include "../stardust/achievement.h"

namespace unlock {

// Patch is enabled by default
TICKABLE_DEFINITION((
        .name = "stardust-unlock",
        .description = "Bonus Modes Unlock",
        .enabled = true,
        .init_sel_ngc = init_sel_ngc, ))

// Condition: 1 stunt goal in each world
// Check each world for if they have ANY stunt goals
// If any world is missing all 10 stunt goals, fail the condition
// If none of the worlds fail, there's 1 in each world, so succeed the condition
bool unlock_condition_met(){
    if(mkb::unlock_info.monkeys == 99) return true;  // Allows Practice Mod Unlock to unlock these modes
    if(mkb::unlock_info.g_movies_watched != 0x0fff){ // Beat the Game
        return achievement::detect_stunt_pilot();    // 1 stunt goal in each world
    }
    else return false;
}

void init_sel_ngc() {
    if(unlock_condition_met()) {
        // Unlock bonus modes (Challenge Mode)
        patch::write_word(reinterpret_cast<void*>(0x808ff800), 0x38000004);
        patch::write_word(reinterpret_cast<void*>(0x808ff80c), 0x38000000);
    }
    else {
        // Lock bonus modes (Challenge Mode)
        patch::write_word(reinterpret_cast<void*>(0x808ff800), 0x38000006);
        patch::write_word(reinterpret_cast<void*>(0x808ff80c), 0x38000002);
    }
    // Lock Supernova (Master)
    patch::write_word(reinterpret_cast<void*>(0x808fbe7c), 0x38000008);
}

}// namespace unlock