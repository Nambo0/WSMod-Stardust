#pragma once

#include "../mkb/mkb.h"

namespace pad {

enum Dir {
    DIR_UP,
    DIR_UPRIGHT,
    DIR_RIGHT,
    DIR_DOWNRIGHT,
    DIR_DOWN,
    DIR_DOWNLEFT,
    DIR_LEFT,
    DIR_UPLEFT,
    DIR_NONE = -1,
};

// Simple wrappers about internal MKB2 bitfields. Represents OR-ed inputs of all controllers.

// Accept a mkb::PadDigitalInput
bool button_down(u16 digital_input, bool priority = false);
bool button_pressed(u16 digital_input, bool priority = false);
bool button_released(u16 digital_input, bool priority = false);
bool button_chord_pressed(u16 btn1, u16 btn2, bool priority = false);

// Accept a mkb::PadAnalogInput
bool analog_down(u16 analog_input, bool priority = false);
bool analog_pressed(u16 analog_input, bool priority = false);
bool analog_released(u16 analog_input, bool priority = false);
bool analog_chord_pressed(u16 analog1, u16 analog2, bool priority = false);

s32 get_cstick_dir(bool priority = false);
bool dir_down(u16 dir, bool priority = false);   // Only works for cardinal directions
bool dir_pressed(u16 dir, bool priority = false);// Only works for cardinal directions

}// namespace pad
