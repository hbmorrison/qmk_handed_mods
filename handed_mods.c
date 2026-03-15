#include "handed_mods.h"

// Declare internal functions.

void hm_process_handed_mod_key(uint16_t, keyrecord_t *);
void hm_process_handed_mod_press(uint16_t, keyrecord_t *);
void hm_process_handed_mod_release(uint16_t, keyrecord_t *);
void hm_process_other_key(uint16_t, keyrecord_t *);
void hm_process_other_press(uint8_t);
void hm_process_oneshots_on_other_press(keypos_t);
void hm_process_other_release(void);
bool hm_is_hold_action(uint16_t, keyrecord_t *);
bool hm_is_interrupted(uint16_t);
void hm_set_interrupts_for(uint8_t);
void hm_reset_interrupt(uint16_t);
uint8_t hm_mod_bit(uint16_t);

// As the handed modifier keys are pressed, the mod bits associated with each
// key are added to these mod masks, and then removed when the handed modifier
// key is released.

static uint8_t hm_left_mods          = 0;
static uint8_t hm_left_oneshot_mods  = 0;
static uint8_t hm_right_mods         = 0;
static uint8_t hm_right_oneshot_mods = 0;

// When another key is pressed, the appropriate handed modifier bit masks are
// applied as actual modifiers, but only those mod bits that are not currently
// applied by other means. There may be other keys on the keymap with modifiers
// applied in the usual way, e.g. LCTL(KC_C). These mod masks store the mod bits
// that have been applied when a key is pressed so that the correct modifiers
// can be removed when the key is released.

static uint8_t hm_applied_mods = 0;
static uint8_t hm_applied_oneshot_mods = 0;

// When a handed modifier key is pressed, its interrupted state is cleared. When
// another key is pressed while the handed modifier key remains pressed, its
// interrupted state is set to true. If the handed modifier key is released and
// the interrupted state is still false, it is converted into a oneshot handed
// modifier.

static bool hm_sft_interrupted = false;
static bool hm_ctl_interrupted = false;
static bool hm_alt_interrupted = false;
static bool hm_gui_interrupted = false;

// Determine how key events will affect and be influenced by the handed
// modifiers.

bool process_record_handed_mods(uint16_t keycode, keyrecord_t *record) {

  // If the key is to be ignored then return and allow the key to continue being
  // processed elsewhere.

  if (handed_mods_is_ignored_key(keycode))
    return true;

  // If the key is a reset key, remove any applied mods and reset all of the mod
  // masks.

  if (handed_mods_is_reset_key(keycode)) {
    if (hm_applied_mods) {
      del_mods(hm_applied_mods);
      hm_applied_mods = 0;
    }
    if (hm_applied_oneshot_mods) {
      del_mods(hm_applied_oneshot_mods);
      hm_applied_oneshot_mods = 0;
    }
    hm_left_mods = 0;
    hm_left_oneshot_mods = 0;
    hm_right_mods = 0;
    hm_right_oneshot_mods = 0;
    return true;
  }

  // Otherwise, determine how to process the key event.

  switch (keycode) {
    case HM_SFT:
    case HM_CTL:
    case HM_ALT:
    case HM_GUI:
      hm_process_handed_mod_key(keycode, record);
      return false;
    default:
      hm_process_other_key(keycode, record);
      return true;
  }
}

// Process handed modifier key events.

void hm_process_handed_mod_key(uint16_t keycode, keyrecord_t *record) {
  if (record->event.pressed)
    hm_process_handed_mod_press(keycode, record);
  else
    hm_process_handed_mod_release(keycode, record);
}

// When a handed modifier key is pressed, add its associated mod bit to the
// correct mod mask.

void hm_process_handed_mod_press(uint16_t keycode, keyrecord_t *record) {

  // Get the mod bit associated with the handed modifier.

  uint8_t bit = hm_mod_bit(keycode);

  // Add the mod bit to the correct mask and remove the mod bit from the
  // opposite side.

  if (handed_mods_is_left_key(record->event.key)) {
    hm_left_mods |= bit;
    hm_right_mods &= ~bit;
  } else {
    hm_right_mods |= bit;
    hm_left_mods &= ~bit;
  }

  // Set the state of the handed modifier to not interrupted.

  hm_reset_interrupt(keycode);
}

// When a handed modifier key is released, remove its mod bit from the correct
// mod masks.

void hm_process_handed_mod_release(uint16_t keycode, keyrecord_t *record) {

  // Get the mod bit associated with the handed modifier.

  uint8_t bit = hm_mod_bit(keycode);

  // Remove the mod bit from the left or right mod mask.

  if (handed_mods_is_left_key(record->event.key))
    hm_left_mods &= ~bit;
  else
    hm_right_mods &= ~bit;

  // If no other key has been pressed while the handed modifier key has remained
  // pressed, then treat the handed modifier key event as a oneshot.

  if (! hm_is_interrupted(keycode)) {
    if (handed_mods_is_left_key(record->event.key))
      hm_left_oneshot_mods |= bit;
    else
      hm_right_oneshot_mods |= bit;
  }
}

// Process other key events using the correct set of handed modifier bits.

void hm_process_other_key(uint16_t keycode, keyrecord_t *record) {
  if (record->event.pressed) {

    // Do not continue if this is a hold action on a mod-tap or layer-tap key.

    if (hm_is_hold_action(keycode, record))
      return;

    // If the key event is from the left side of the keyboard, apply the right
    // handed modifiers to it, and vice versa.

    if (handed_mods_is_left_key(record->event.key))
      hm_process_other_press(hm_right_mods);
    else
      hm_process_other_press(hm_left_mods);

    hm_process_oneshots_on_other_press(record->event.key);
  } else {

    // Process key releases.

    hm_process_other_release();
  }
}

// Apply the given modifiers so that they affect the other key press when it is
// processed later on.

void hm_process_other_press(uint8_t mods) {

  // Mark the currently active handed modifiers as interrupted.

  hm_set_interrupts_for(mods);

  // The active handed modifiers are only applied if the modifiers are not
  // already applied by some other means, so remove all of the currently applied
  // modifiers from the mod mask and apply the remaining modifiers.

  hm_applied_mods = mods & ~get_mods();
  add_mods(hm_applied_mods);
}

void hm_process_oneshots_on_other_press(keypos_t key) {

  // Just as with the standard modifiers, oneshot modifiers are only applied if
  // they are not already applied by some other means. So, work out which
  // oneshot mods can be applied, then clear those mods from the mod mask that
  // they came from, since they have been "used up" by this other key press.

  if (handed_mods_is_left_key(key)) {
    hm_applied_oneshot_mods = hm_right_oneshot_mods & ~get_oneshot_mods();
    hm_right_oneshot_mods &= ~hm_applied_oneshot_mods;
  } else {
    hm_applied_oneshot_mods = hm_left_oneshot_mods & ~get_oneshot_mods();
    hm_left_oneshot_mods &= ~hm_applied_oneshot_mods;
  }

  // Apply any oneshot mods that can be applied, then immediately clear them
  // so they cannot be applied again by a subsequent key press.

  if (hm_applied_oneshot_mods) {
    add_oneshot_mods(hm_applied_oneshot_mods);
    hm_applied_oneshot_mods = 0;
  }
}

// Once the other key is released, remove any previously applied modifiers.

void hm_process_other_release() {
  if (hm_applied_mods) {
    del_mods(hm_applied_mods);
    hm_applied_mods = 0;
  }
}

// Return the mod bits associated with the custom keys defined by this module.

uint8_t hm_mod_bit(uint16_t keycode) {
  switch (keycode) {
    case HM_SFT: return SFT_MOD_BIT;
    case HM_CTL: return CTL_MOD_BIT;
    case HM_ALT: return ALT_MOD_BIT;
    case HM_GUI: return GUI_MOD_BIT;
  }
  return 0;
}

// Return true if the key being pressed is a mod-tap or layer-tap key, and the
// key is in a hold state.

bool hm_is_hold_action(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
    case QK_MOD_TAP ... QK_MOD_TAP_MAX:
    case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
      return ! record->tap.count;
  }
  return false;
}

// Return the stored interrupted state for the given custom key.

bool hm_is_interrupted(uint16_t keycode) {
  switch (keycode) {
    case HM_SFT: return hm_sft_interrupted;
    case HM_CTL: return hm_ctl_interrupted;
    case HM_ALT: return hm_alt_interrupted;
    case HM_GUI: return hm_gui_interrupted;
  }
  return false;
}

// Set the stored interrupt state for each modifier that is active in the given
// bit mask.

void hm_set_interrupts_for(uint8_t bits) {
  hm_sft_interrupted = (bits & SFT_MOD_BIT) > 0;
  hm_ctl_interrupted = (bits & CTL_MOD_BIT) > 0;
  hm_alt_interrupted = (bits & ALT_MOD_BIT) > 0;
  hm_gui_interrupted = (bits & GUI_MOD_BIT) > 0;
}

// Clear the stored interrupt state for the modifier associated with the given
// custom keycode.

void hm_reset_interrupt(uint16_t keycode) {
  switch (keycode) {
    case HM_SFT: hm_sft_interrupted = false; break;
    case HM_CTL: hm_ctl_interrupted = false; break;
    case HM_ALT: hm_alt_interrupted = false; break;
    case HM_GUI: hm_gui_interrupted = false; break;
  }
}

// This function can be overriden in keymap.c to return true if the given
// keycode should be ignored during the handed modifier key processing.

__attribute__((weak)) bool handed_mods_is_ignored_key(uint16_t keycode) {
  return false;
}

// This function can be overriden in keymap.c to return true if the given
// keycode should reset the state of all handed modifiers when it is pressed.
// Defaults to the esc key.

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
