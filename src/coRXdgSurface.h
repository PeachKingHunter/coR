#ifndef CoRXdgSurface_H
#define CoRXdgSurface_H

#include "coRInputs.h"

// wlroot
#include <wlr/types/wlr_xdg_shell.h>
#include <wayland-util.h>

// Struture
struct coR_xdg_surface {
  struct wlr_xdg_surface *xdgSurface;
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
// static void mapXdgSurfaceHandler(struct wl_listener *listener, void *data);
// static void unmapXdgSurfaceHandler(struct wl_listener *listener, void *data);
// static void destroyXdgSurfaceHandler(struct wl_listener *listener, void
// *data);
void newXdgSurfaceHandler(struct wl_listener *listener, void *data);

#endif
