#ifndef CoRKeyboard_H
#define CoRKeyboard_H

// Wlroot
#include "wlr/backend/libinput.h"

// My Lib
#include "../coRState.h"

// Lib c
#include <unistd.h>

// Structure
struct coR_keyboard_input {
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener keyListener;
  struct wl_listener modifierListener;
  struct wl_listener destroyListener;
};

// Functions
void keyKeyboardHandler(struct wl_listener *listener, void *data);
void modifierKeyboardHandler(struct wl_listener *listener, void *data);

#endif
