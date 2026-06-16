#ifndef coRInputs_H
#define coRInputs_H

#include "coRState.h"
#include "wlr/backend/libinput.h"
#include <wayland-server-core.h>

struct coR_keyboard_input {
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener keyListener;
  struct wl_listener modifierListener;
  struct wl_listener destroyListener;
};

struct coR_pointer_input {
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener buttonListener;
  struct wl_listener motionListener;
  struct wl_listener motionAbsoluteListener;
  struct wl_listener axisListener;
};

void newInputHandler(struct wl_listener *listener, void *data);
void keyKeyboardHandler(struct wl_listener *listener, void *data);

void cursorButtonHandler(struct wl_listener *listener, void *data);
void cursorMotionHandler(struct wl_listener *listener, void *data);
void cursorMotionAbsoluteHandler(struct wl_listener *listener, void *data);
void cursorAxisHandler(struct wl_listener *listener, void *data);

#endif
