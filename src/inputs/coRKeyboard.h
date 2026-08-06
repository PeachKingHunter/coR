#ifndef CoRKeyboard_H
#define CoRKeyboard_H

// Wlroot
#include "wlr/backend/libinput.h"

// My Lib
#include "../coRState.h"
#include "../surfaces/coRXSurface.h"
#include "coRCursor.h"

// Lib c
#include <sys/types.h>
#include <unistd.h>
#include <wayland-util.h>

// Settings
#define MAX_KEYS_SYM 65536

// Structure
struct coR_keyboard_input {
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;
  u_int32_t pressedKeys[MAX_KEYS_SYM / 8];

  // Listeners
  struct wl_listener keyListener;
  struct wl_listener modifierListener;
  struct wl_listener destroyListener;
};

// Functions
void keyKeyboardHandler(struct wl_listener *listener, void *data);
void modifierKeyboardHandler(struct wl_listener *listener, void *data);

uint8_t isKeyPressed(struct coR_keyboard_input *coRKeyboardI, uint32_t keyCode);
void keySetPress(struct coR_keyboard_input *coRKeyboardI, uint32_t keyCode,
                 int isPressed);

#endif
