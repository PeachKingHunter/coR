#ifndef coRInputs_H
#define coRInputs_H

#include "coRState.h"
#include "wlr/backend/libinput.h"
#include <wayland-server-core.h>

struct coR_input_d {
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener keyListener;
};

void newInputHandler(struct wl_listener *listener, void *data);
void grabKeyboardBeginHandler(struct wl_listener *listener, void *data);

#endif
