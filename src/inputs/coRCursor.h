#ifndef CoRCursor_H
#define CoRCursor_H
#pragma once

// Wlroot
#include "wlr/backend/libinput.h"
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xdg_shell.h>

// My Lib
#include "../coRState.h"

// Structure
struct coR_pointer_input {
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener buttonListener;
  struct wl_listener motionListener;
  struct wl_listener motionAbsoluteListener;
  struct wl_listener axisListener;
};

// Functions
void cursorButtonHandler(struct wl_listener *listener, void *data);
void cursorMotionHandler(struct wl_listener *listener, void *data);
void cursorMotionAbsoluteHandler(struct wl_listener *listener, void *data);
void cursorAxisHandler(struct wl_listener *listener, void *data);

struct wlr_surface *getSurfaceBelowCursor(struct coR_state *coRState,
                                          double *sX, double *sY);

#endif
