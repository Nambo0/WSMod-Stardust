#include "fix_spooky_action.h"
#include "../internal/patch.h"
#include "../internal/tickable.h"
#include "../utils/vecutil.h"
#include "mkb/mkb.h"

namespace {
// e
// Our special collision flag that's reset for each itemgroup
constexpr u32 COLI_FLAG_IG = 1 << 7;

patch::Tramp<decltype(&mkb::init_physicsball_from_ball)> s_init_physicsball_tramp;
patch::Tramp<decltype(&mkb::tf_physball_to_itemgroup_space)> s_tf_physicsball_tramp;
// Later ghidra updates call this function collide_ball_with_plane
patch::Tramp<decltype(&mkb::g_something_with_physicsball_restitution)> s_collide_physicsball_tramp;
// void stobj_bumper_coli(struct Stobj * stobj, struct PhysicsBall * physicsball);
patch::Tramp<decltype(&mkb::stobj_bumper_coli)> s_stobj_bumper_coli_tramp;
// void stobj_goalbag_coli(struct Stobj * stobj, struct PhysicsBall * physicsball);
patch::Tramp<decltype(&mkb::stobj_goalbag_coli)> s_stobj_goalbag_coli_tramp;
// void stobj_goaltape_coli(struct Stobj * stobj, struct PhysicsBall * physicsball);
patch::Tramp<decltype(&mkb::stobj_goaltape_coli)> s_stobj_goaltape_coli_tramp;
// void stobj_returngate_coli(struct Stobj * stobj, struct PhysicsBall * physicsball);
patch::Tramp<decltype(&mkb::stobj_returngate_coli)> s_stobj_returngate_coli_tramp;

mkb::PhysicsBall s_clean_physicsball;

void init_physicsball_from_ball(mkb::Ball* ball, mkb::PhysicsBall* physicsball) {
    s_init_physicsball_tramp.dest(ball, physicsball);
    s_clean_physicsball = *physicsball;
}

void tf_physicsball_to_itemgroup_space(mkb::PhysicsBall* physicsball, int dest_ig_idx) {
    if (physicsball->flags & COLI_FLAG_IG) {
        physicsball->flags &= ~COLI_FLAG_IG;
        s_clean_physicsball = *physicsball;
    }
    else {
        if(mkb::current_stage_id >= 91 && mkb::current_stage_id <= 100) { // Silent Supernova stage IDs
            *physicsball = s_clean_physicsball;
        }
    }
    s_tf_physicsball_tramp.dest(physicsball, dest_ig_idx);
}

void collide_ball_with_plane(mkb::PhysicsBall* physicsball, mkb::ColiPlane* plane) {
    Vec prev_pos = physicsball->pos;
    Vec prev_vel = physicsball->vel;
    s_collide_physicsball_tramp.dest(physicsball, plane);
    if (!VEC_EQUAL_EXACT(prev_pos, physicsball->pos) ||
        !VEC_EQUAL_EXACT(prev_vel, physicsball->vel)) {
        physicsball->flags |= COLI_FLAG_IG;
    }
}

// Bumper collision
void bumper_coli(mkb::Stobj* stobj, mkb::PhysicsBall * physicsball) {
    Vec prev_pos = physicsball->pos;
    Vec prev_vel = physicsball->vel;
    s_stobj_bumper_coli_tramp.dest(stobj, physicsball);
    if (!VEC_EQUAL_EXACT(prev_pos, physicsball->pos) ||
        !VEC_EQUAL_EXACT(prev_vel, physicsball->vel)) {
        physicsball->flags |= COLI_FLAG_IG;
    }
}

// Party Ball collision
void goalbag_coli(mkb::Stobj* stobj, mkb::PhysicsBall * physicsball) {
    Vec prev_pos = physicsball->pos;
    Vec prev_vel = physicsball->vel;
    s_stobj_goalbag_coli_tramp.dest(stobj, physicsball);
    if (!VEC_EQUAL_EXACT(prev_pos, physicsball->pos) ||
        !VEC_EQUAL_EXACT(prev_vel, physicsball->vel)) {
        physicsball->flags |= COLI_FLAG_IG;
    }
}

// Goal tape resistance
void goaltape_coli(mkb::Stobj* stobj, mkb::PhysicsBall * physicsball) {
    Vec prev_pos = physicsball->pos;
    Vec prev_vel = physicsball->vel;
    s_stobj_goaltape_coli_tramp.dest(stobj, physicsball);
    if (!VEC_EQUAL_EXACT(prev_pos, physicsball->pos) ||
        !VEC_EQUAL_EXACT(prev_vel, physicsball->vel)) {
        physicsball->flags |= COLI_FLAG_IG;
    }
}

// Wormhole surface resistance
void returngate_coli(mkb::Stobj* stobj, mkb::PhysicsBall * physicsball) {
    Vec prev_pos = physicsball->pos;
    Vec prev_vel = physicsball->vel;
    s_stobj_returngate_coli_tramp.dest(stobj, physicsball);
    if (!VEC_EQUAL_EXACT(prev_pos, physicsball->pos) ||
        !VEC_EQUAL_EXACT(prev_vel, physicsball->vel)) {
        physicsball->flags |= COLI_FLAG_IG;
    }
}

}// namespace

namespace fix_spooky_action {

TICKABLE_DEFINITION((
        .name = "fix-spooky-action",
        .description = "Prevent accumulated floating point error from transforming ball during collision",
        .init_main_loop = init_main_loop, ))

void init_main_loop() {

    patch::hook_function(s_init_physicsball_tramp, mkb::init_physicsball_from_ball,
                         init_physicsball_from_ball);
    patch::hook_function(s_tf_physicsball_tramp, mkb::tf_physball_to_itemgroup_space,
                         tf_physicsball_to_itemgroup_space);
    patch::hook_function(s_collide_physicsball_tramp, mkb::g_something_with_physicsball_restitution,
                         collide_ball_with_plane);
    patch::hook_function(s_stobj_bumper_coli_tramp, mkb::stobj_bumper_coli,
                         bumper_coli);
    patch::hook_function(s_stobj_goalbag_coli_tramp, mkb::stobj_goalbag_coli,
                         goalbag_coli);
    patch::hook_function(s_stobj_goaltape_coli_tramp, mkb::stobj_goaltape_coli,
                         goaltape_coli);
    patch::hook_function(s_stobj_returngate_coli_tramp, mkb::stobj_returngate_coli,
                         returngate_coli);
}

}// namespace fix_spooky_action