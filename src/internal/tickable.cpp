#include "tickable.h"
#include "internal/patch.h"

#include "etl/iterator.h"
#include "log.h"

#define STREQ(x, y) (mkb::strcmp(const_cast<char*>(x), const_cast<char*>(y)) == 0)

namespace tickable {

void TickableManager::push(Tickable* tickable) {
    auto tick_ptr = etl::unique_ptr<Tickable>(tickable);
    m_tickables.push_back(std::move(tick_ptr));
}

const TickableManager::TickableVec& TickableManager::get_tickables() const {
    return m_tickables;
}

void TickableManager::init() const {
    static patch::Tramp<decltype(&mkb::draw_debugtext)> s_draw_debugtext_tramp;
    static patch::Tramp<decltype(&mkb::load_additional_rel)> s_load_additional_rel_tramp;
    static patch::Tramp<decltype(&mkb::smd_game_goal_init)> s_smd_game_goal_init_tramp;

    // Hook for mkb::draw_debugtext
    patch::hook_function(s_draw_debugtext_tramp, mkb::draw_debugtext, []() {
        // Drawing hook for UI elements.
        // Gets run at the start of smb2's function which draws debug text windows,
        // which is called at the end of smb2's function which draws the UI in general.

        // Disp functions (REL patches)
        for (const auto& tickable: get_tickable_manager().get_tickables()) {
            if (tickable->enabled && tickable->disp) {
                (*tickable->disp)();
            }
        }
        s_draw_debugtext_tramp.dest();
    });

    // Hook for mkb::load_additional_rel
    patch::hook_function(
        s_load_additional_rel_tramp,
        mkb::load_additional_rel, [](char* rel_filepath, mkb::RelBufferInfo* rel_buffer_ptrs) {
            s_load_additional_rel_tramp.dest(rel_filepath, rel_buffer_ptrs);

            // Functions that need to be initialized when mkb2.main_game.rel is loaded
            if (STREQ(rel_filepath, "mkb2.main_game.rel")) {
                for (const auto& tickable: get_tickable_manager().get_tickables()) {
                    if (tickable->enabled && tickable->init_main_game) {
                        mkb::OSReport("Calling init_main_game for: %s\n", tickable->name);
                        (*tickable->init_main_game)();
                    }

                    // On-goal-entry functions
                    patch::hook_function(s_smd_game_goal_init_tramp, mkb::smd_game_goal_init, []() {
                        s_smd_game_goal_init_tramp.dest();

                        for (const auto& tickable: get_tickable_manager().get_tickables()) {
                            if (tickable->enabled && tickable->on_goal) {
                                (*tickable->on_goal)();
                            }
                        }
                    });
                }
            }

            // Functions that need to be initialized when mkb2.sel_ngc.rel is loaded
            else if (STREQ(rel_filepath, "mkb2.sel_ngc.rel")) {
                for (const auto& tickable: get_tickable_manager().get_tickables()) {
                    if (tickable->enabled && tickable->init_sel_ngc) {
                        mkb::OSReport("Calling sel_ngc\n");
                        (*tickable->init_sel_ngc)();
                    }
                }
            }
        });
}

bool TickableManager::get_tickable_status(const char* name) const {
    for (const auto& t: m_tickables) {
        if (strcmp(t->name, name) == 0) {
            return t->enabled;
        }
    }
    return false;
}

TickableManager& get_tickable_manager() {
    static uint8_t s_tickable_manager[sizeof(TickableManager)];
    static bool s_buf_init = false;

    if (!s_buf_init) {
        new (s_tickable_manager) TickableManager();
        s_buf_init = true;
    }
    auto ptr = reinterpret_cast<TickableManager*>(s_tickable_manager);
    return *ptr;
}

}// namespace tickable