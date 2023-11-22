#include "fix_transparency_type_b_texture_scroll.h"

#include "internal/patch.h"
#include "internal/tickable.h"
#include "utils/ppcutil.h"

namespace fix_transparency_type_b_texture_scroll {

TICKABLE_DEFINITION((
        .name = "fix-transparency-type-b-texture-scroll",
        .description = "Transparency Type B texture scroll fix",
        .enabled = true,
        .init_main_loop = init_main_loop, ))

// Nop a call to a function that breaks type B texture scroll during gameplay
void init_main_loop() {
    patch::write_nop(reinterpret_cast<void*>(0x802c94fc));
}

}// namespace fix_transparency_type_b_texture_scroll
