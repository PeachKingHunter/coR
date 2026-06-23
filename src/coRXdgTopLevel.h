#ifndef CoRXdgSurface_H
#define CoRXdgSurface_H

#include "coRInputs.h"

// wlroot
#include <wlr/types/wlr_xdg_shell.h>
#include <wayland-util.h>

// Struture
struct coR_xdg_toplevel {
  struct wlr_xdg_toplevel *xdgTopLevel;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener mapListener;
  struct wl_listener unMapListener;
  struct wl_listener destroyListener;
  struct wl_listener commitListener;

  // List
  struct wl_list link;
};

// Methods
void newXdgTopLevelHandler(struct wl_listener *listener, void *data);

#endif
