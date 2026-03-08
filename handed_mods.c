#include "handed_mods.h"

// Declare internal functions.

void process_key_press(uint16_t, keyrecord_t *);
void process_key_release(void);
bool get_key_handedness(keypos_t key);

// State of the left and right modifiers when handedness is being enforced.

static uint16_t lh_mod_state = false;
static uint16_t rh_mod_state = false;

// Enforce handed modifiers.

bool process_record_handed_mods(uint16_t keycode, keyrecord_t *record) {

  // Do not process keys that should be ignored.

  if (! is_handed_mods_ignored_key(keycode)) {
    if (record->event.pressed)
      process_key_press(keycode, record);
    else
      process_key_release();
  }

  // Always return true as this process_...() function does not issue keys.

  return true;
}

// Process whether modifiers should be temporarily removed for the given
// keycode, based on the handedness of the modifiers and whether the keycode is
// on the left or right side of the keyboard.

void process_key_press(uint16_t keycode, keyrecord_t *record) {
  uint8_t mod_state = get_mods();
  uint8_t os_mod_state = get_oneshot_mods();

  // Check whether the key press is on the left or right side of the keyboard.

  if (is_key_on_left_side(record->event.key)) {

    // For a key on the left side, check if any left modifiers are currently
    // active. If there are, make a note of them and deactivate them.

    lh_mod_state = mod_state & LH_MOD_BITS;
    if (lh_mod_state)
      del_mods(LH_MOD_BITS);

    // Remove any left oneshot modifiers since this key press effectively "uses
    // them up" even if the modifier does not end up being applied.

    if (os_mod_state & LH_MOD_BITS)
      del_oneshot_mods(LH_MOD_BITS);
  } else {

    // For a key on the right side, make a note of any right modifiers and
    // deactivate them.

    rh_mod_state = mod_state & RH_MOD_BITS;
    if (rh_mod_state)
      del_mods(RH_MOD_BITS);

    // Remove any right oneshot modifiers.

    if (os_mod_state & RH_MOD_BITS)
      del_oneshot_mods(RH_MOD_BITS);
  }
}

// Restore any modifiers that were temporarily removed on press.

void process_key_release() {
  if (lh_mod_state) {
    lh_mod_state = 0;
    add_mods(lh_mod_state);
  }
  if (rh_mod_state) {
    rh_mod_state = 0;
    add_mods(rh_mod_state);
  }
}

// User defined function that will return true if the given keycode should be
// ignored by the handed modifier processing.

__attribute__((weak)) bool is_handed_mods_ignored_key(uint16_t keycode) {
  return false;
}

// User overridable function that returns true if the given key is on the left
// side of the keyboard and false if it is on the right side of the keyboard.

__attribute__((weak)) bool is_key_on_left_side(keypos_t key) {
  return key.row < MATRIX_ROWS / 2;
}
