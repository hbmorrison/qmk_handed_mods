#pragma once

#include QMK_KEYBOARD_H

// Function declarations.

bool handed_mods_ignore_bare_shift(uint16_t);
bool handed_mods_is_ignored_key(uint16_t);
bool handed_mods_is_reset_key(uint16_t);
bool handed_mods_is_left_key(keypos_t);

// Define mod bits.

#define SFT_MOD_BIT (MOD_BIT(KC_LSFT))
#define CTL_MOD_BIT (MOD_BIT(KC_LCTL))
#define ALT_MOD_BIT (MOD_BIT(KC_LALT))
#define GUI_MOD_BIT (MOD_BIT(KC_LGUI))
