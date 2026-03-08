#pragma once

#include QMK_KEYBOARD_H

// Function declarations.

bool is_handed_mods_ignored_key(uint16_t);
bool is_key_on_left_side(keypos_t);

// Define which mods are considered left and right handed.

#ifndef LH_MOD_BITS
#define LH_MOD_BITS (MOD_BIT(KC_LSFT)|MOD_BIT(KC_LCTL)|MOD_BIT(KC_LALT)|MOD_BIT(KC_LGUI))
#endif

#ifndef RH_MOD_BITS
#define RH_MOD_BITS (MOD_BIT(KC_RSFT)|MOD_BIT(KC_RCTL)|MOD_BIT(KC_RALT)|MOD_BIT(KC_RGUI))
#endif
