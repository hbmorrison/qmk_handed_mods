#include "handed_mods.h"

// Declare internal functions.

void handed_mods_process_handed_mod_key(uint16_t, keyrecord_t *);
void handed_mods_process_affected_key(uint16_t, keyrecord_t *);
bool handed_mods_is_hold_action(uint16_t, keyrecord_t *);
bool handed_mods_get_interrupt(uint16_t);
void handed_mods_set_interrupt(uint8_t);
void handed_mods_clear_interrupt(uint16_t);
uint8_t handed_mods_get_mod_bit(uint16_t);

// As handed modifier keys are pressed, the mod bits associated with each key
// are added to these mod masks, and then removed when the handed modifier key
// is released.

static uint8_t left_mask  = 0;
static uint8_t right_mask = 0;

// When handed modifier keys are released without being interrupted by another
// key press, then the associated mod bits are copied into these oneshot mod
// masks before the mod bits are removed.

static uint8_t left_oneshot_mask  = 0;
static uint8_t right_oneshot_mask = 0;

// When another key is pressed while the handed modifier key is active, its
// interrupted state is set to true.

static bool handed_mods_sft_interrupted = false;
static bool handed_mods_ctl_interrupted = false;
static bool handed_mods_alt_interrupted = false;
static bool handed_mods_gui_interrupted = false;

// Determine how key events will affect and be influenced by the handed
// modifiers.

bool process_record_handed_mods(uint16_t keycode, keyrecord_t *record) {

  // Return when an ignored key is pressed.

  if (handed_mods_is_ignored_key(keycode))
    return true;

  // Reset all of the mod masks when a reset key is pressed.

  if (handed_mods_is_reset_key(keycode)) {
    left_mask = 0;
    right_mask = 0;
    left_oneshot_mask = 0;
    right_oneshot_mask = 0;
    return true;
  }

  // Process handed modifier key actions and stop further processing of this key
  // action.

  switch (keycode) {
    case HM_SFT:
    case HM_CTL:
    case HM_ALT:
    case HM_GUI:
      handed_mods_process_handed_mod_key(keycode, record);
      return false;
  }

  // Process how the handed modifiers will affect other key actions.

  handed_mods_process_affected_key(keycode, record);
  return true;
}

// Process key actions for handed modifier keys.

void handed_mods_process_handed_mod_key(uint16_t keycode, keyrecord_t *record) {
  uint8_t bit = handed_mods_get_mod_bit(keycode);
  if (record->event.pressed) {

    // If this action is a key press, clear the interrupt state for this handed
    // modifier and add the modifier bit to the correct mask.

    handed_mods_clear_interrupt(keycode);
    if (handed_mods_is_left_key(record->event.key))
      left_mask |= bit;
    else
      right_mask |= bit;

    // Clear any pre-existing oneshot modifiers now that there is at least one
    // handed modifier key being pressed.

    left_oneshot_mask = 0;
    right_oneshot_mask = 0;
  } else {

    // If this action is a key release, remove the mod bit from the correct mod
    // mask.

    if (handed_mods_is_left_key(record->event.key))
      left_mask &= ~bit;
    else
      right_mask &= ~bit;

    // If no other key has interrupted this handed modifier key press, then set
    // the modifier as a oneshot. If the same oneshot modifier is currently
    // active on the opposite side then it is cleared.

    if (! handed_mods_get_interrupt(keycode)) {
      if (handed_mods_is_left_key(record->event.key)) {
        left_oneshot_mask |= bit;
        right_oneshot_mask &= ~bit;
      } else {
        left_oneshot_mask &= ~bit;
        right_oneshot_mask |= bit;
      }
    }
  }
}

// Process key actions for other keys that will be affected by the handed
// modifiers.

void handed_mods_process_affected_key(uint16_t keycode, keyrecord_t *record) {

  // Return without affecting the state of the handed modifiers if this is a
  // hold action on a mod-tap or layer-tap key.

  if (handed_mods_is_hold_action(keycode, record)) return;

  // Only interested in key presses.

  if (record->event.pressed) {
    uint8_t mods;

    // Combine the current modifiers and oneshot modifiers so that they can be
    // applied together.

    if (handed_mods_is_left_key(record->event.key))
      mods = right_mask | right_oneshot_mask;
    else
      mods = left_mask | left_oneshot_mask;

    // Clear the oneshot modifiers so that they will not be applied to
    // subsequent key presses. Both sets of oneshot modifiers are cleared
    // because any key press should clear all oneshot modifiers, even if
    // the modifiers are not going to be applied to the key.

    left_oneshot_mask = 0;
    right_oneshot_mask = 0;

    // Remove the shift modifier if it appears on its own in the mod mask and is
    // about to be applied to a key that should not be shifted.

    if ((mods & ~SFT_MOD_BIT) == 0 && handed_mods_ignore_bare_shift(keycode))
      mods = 0;

    // Set the modifiers so that they will only apply to the this key press.

    register_weak_mods(mods);

    // Indicate that this key press has interrupted the currently active handed
    // modifiers.

    handed_mods_set_interrupt(mods);
  }
}

// Return the mod bit associated with the given handed modifier key.

uint8_t handed_mods_get_mod_bit(uint16_t keycode) {
  switch (keycode) {
    case HM_SFT: return SFT_MOD_BIT;
    case HM_CTL: return CTL_MOD_BIT;
    case HM_ALT: return ALT_MOD_BIT;
    case HM_GUI: return GUI_MOD_BIT;
  }
  return 0;
}

// Return the interrupt state associated with the given handed modifier key.

bool handed_mods_get_interrupt(uint16_t keycode) {
  switch (keycode) {
    case HM_SFT: return handed_mods_sft_interrupted;
    case HM_CTL: return handed_mods_ctl_interrupted;
    case HM_ALT: return handed_mods_alt_interrupted;
    case HM_GUI: return handed_mods_gui_interrupted;
  }
  return false;
}

// Clear the interrupt state associated with the given handed modifier key.

void handed_mods_clear_interrupt(uint16_t keycode) {
  switch (keycode) {
    case HM_SFT: handed_mods_sft_interrupted = false; break;
    case HM_CTL: handed_mods_ctl_interrupted = false; break;
    case HM_ALT: handed_mods_alt_interrupted = false; break;
    case HM_GUI: handed_mods_gui_interrupted = false; break;
  }
}

// Set the interrupts associated with the given mod bits.

void handed_mods_set_interrupt(uint8_t bits) {
  if ((bits & SFT_MOD_BIT) > 0) handed_mods_sft_interrupted = true;
  if ((bits & CTL_MOD_BIT) > 0) handed_mods_ctl_interrupted = true;
  if ((bits & ALT_MOD_BIT) > 0) handed_mods_alt_interrupted = true;
  if ((bits & GUI_MOD_BIT) > 0) handed_mods_gui_interrupted = true;
}

// Returns true if the given key event is a hold action associated with a
// mod-tap or layer-tap key.

bool handed_mods_is_hold_action(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
    case QK_MOD_TAP ... QK_MOD_TAP_MAX:
    case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
      return ! record->tap.count;
  }
  return false;
}

// Returns true if the shift modifier on its own should not be applied to the
// given key. This is used to prevent accidental shifts in higher layers
// producing unexpected symbols. By default the comma, full stop and slash
// keycodes are not included, so that they can be shifted to issue less than,
// greater than and question mark when they appear in the base layer.

__attribute__((weak)) bool handed_mods_ignore_bare_shift(uint16_t keycode) {
  switch (keycode) {
    case KC_GRAVE:
    case KC_1:
    case KC_2:
    case KC_3:
    case KC_4:
    case KC_5:
    case KC_6:
    case KC_7:
    case KC_8:
    case KC_9:
    case KC_0:
    case KC_MINUS:
    case KC_EQUAL:
    case KC_LEFT_BRACKET:
    case KC_RIGHT_BRACKET:
    case KC_BACKSLASH:
    case KC_SEMICOLON:
    case KC_QUOTE:
      return true;
  }
  return false;
}

// Override this function in keymap.c to return true if the given keycode should
// be ignored during the handed modifier key processing.

__attribute__((weak)) bool handed_mods_is_ignored_key(uint16_t keycode) {
  return false;
}

// Override this function in keymap.c to return true if the given keycode should
// reset the state of all handed modifiers when it is pressed. Defaults to the
// esc key.

__attribute__((weak)) bool handed_mods_is_reset_key(uint16_t keycode) {
  switch (keycode) {
    case KC_ESC:
      return true;
  }
  return false;
}

// By default, this module assumes that keys on the left side of the keyboard
// are found in the lower half of the matrix, but some keyboards are wired
// differently. For example, on the ziplzalp keyboard, keys on the left side are
// found in even rows in the matrix and keys on the right side are found in odd
// rows. This function can be overriden in keymap.c to handle different keyboard
// matrix layouts. So, in the case of zilpzalp, this function should be
// overriden to return "(key.row % 2) == 0".

__attribute__((weak)) bool handed_mods_is_left_key(keypos_t key) {
  return key.row < MATRIX_ROWS / 2;
}
