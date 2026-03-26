#include "handed_mods.h"

// Declare internal functions.

bool handed_mods_process_press(uint16_t, keyrecord_t *);
bool handed_mods_process_release(uint16_t, keyrecord_t *);
uint8_t handed_mods_get_mod_bit(uint16_t);
bool handed_mods_get_interrupt(uint16_t);
void handed_mods_set_interrupt(uint8_t);
void handed_mods_clear_interrupt(uint16_t);
bool handed_mods_ignore_bare_shift(uint16_t);
bool handed_mods_is_hold_action(uint16_t, keyrecord_t *);
bool handed_mods_is_ignored_key(uint16_t);
bool handed_mods_is_reset_key(uint16_t);

// Declare a timer for the oneshot modifiers.

static uint16_t handed_mods_oneshot_timer = 0;

// As handed modifier keys are pressed, the mod bits associated with each key
// are added to these mod masks, and then removed when the handed modifier key
// is released.

static uint8_t handed_mods_mask  = 0;

// When handed modifier keys are released without being interrupted by another
// key press, then the associated mod bits are copied into these oneshot mod
// masks before the mod bits are removed.

static uint8_t handed_mods_oneshot_mask = 0;

static uint8_t handed_mods_applied_mask = 0;

// When another key is pressed while the handed modifier key is active, its
// interrupted state is set to true.

static bool handed_mods_sft_interrupted = false;
static bool handed_mods_ctl_interrupted = false;
static bool handed_mods_alt_interrupted = false;
static bool handed_mods_gui_interrupted = false;

bool process_record_handed_mods(uint16_t keycode, keyrecord_t *record) {

  if (handed_mods_get_mod_bit(keycode)) {
    if (record->event.pressed)
      return handed_mods_process_press(keycode, record);
    else
      return handed_mods_process_release(keycode, record);
  }

  // Return if the keycode is an ignored key.

  if (handed_mods_is_ignored_key(keycode))
    return true;

  // Reset the handed modifiers state and return if the keycode is a reset key.

  if (handed_mods_is_reset_key(keycode)) {
    handed_mods_mask = 0;
    handed_mods_oneshot_mask = 0;
    handed_mods_oneshot_timer = 0;
    return true;
  }

  // Return without affecting the state of the handed modifiers if this is a
  // hold action on a mod-tap or layer-tap key.

  if (handed_mods_is_hold_action(keycode, record))
    return true;

  if (record->event.pressed) {

    // Apply any modifiers and oneshot modifiers together, then clear the
    // oneshot modifiers so that the do not apply to subsequent key presses.

    handed_mods_applied_mask = handed_mods_mask | handed_mods_oneshot_mask;
    handed_mods_oneshot_mask = 0;

    // Remove the shift modifier if it appears on its own in the mod mask and is
    // about to be applied to a key that should not be shifted. This prevents
    // number and unshifted symbol key presses unexpectedly issuing their shifted
    // varient.

    if ((handed_mods_applied_mask & ~SFT_MOD_BIT) == 0 && handed_mods_ignore_bare_shift(keycode))
      handed_mods_applied_mask = 0;

    // Set weak modifiers so that they will only apply to this key press.

    if (handed_mods_applied_mask)
      register_mods(handed_mods_applied_mask);

    // Finally, indicate that this key press has interrupted the currently
    // active modifiers.

    handed_mods_set_interrupt(handed_mods_applied_mask);
  } else {
    if (handed_mods_applied_mask) {
      unregister_mods(handed_mods_applied_mask);
      handed_mods_applied_mask = 0;
    }
  }

  // Continue processing this record.

  return true;
}

bool handed_mods_process_press(uint16_t keycode, keyrecord_t *record) {
  uint8_t bit = handed_mods_get_mod_bit(keycode);

  // Clear the interrupt state for this handed modifier.

  handed_mods_clear_interrupt(keycode);

  // Get the modifier bit associated with this handed modifier keycode and
  // add it to the mask.

  handed_mods_mask |= bit;

  // Clear any pre-existing oneshot modifiers now that there is at least one
  // handed modifier key being pressed.

  handed_mods_oneshot_mask &= ~bit;

  return true;
}

bool handed_mods_process_release(uint16_t keycode, keyrecord_t *record) {
  uint8_t bit = handed_mods_get_mod_bit(keycode);

  // Get the modifier bit associated with this handed modifier keycode and
  // remove it from the mask.

  handed_mods_mask &= ~bit;

  // If this handed modifier key has been released without other keys being
  // pressed in the interim, then set the modifier as a oneshot.

  if (! handed_mods_get_interrupt(keycode))
    handed_mods_oneshot_mask |= bit;

  // If ONESHOT_TIMEOUT is defined, set the oneshot timer to the current time
  // so that the oneshot modifiers can be cleared if they are unused.

#if (defined(ONESHOT_TIMEOUT) && (ONESHOT_TIMEOUT > 0))
  handed_mods_oneshot_timer = timer_read();
#endif

  return true;
}

// Clear the oneshot modifiers at the end of the scan if the oneshot timout has
// been exceeded.

#if (defined(ONESHOT_TIMEOUT) && (ONESHOT_TIMEOUT > 0))
void housekeeping_task_handed_mods(void) {
  if (handed_mods_oneshot_timer > 0) {
    if (TIMER_DIFF_16(timer_read(), handed_mods_oneshot_timer) >= ONESHOT_TIMEOUT) {
      handed_mods_oneshot_timer = 0;
      handed_mods_oneshot_mask = 0;
    }
  }
}
#endif

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
