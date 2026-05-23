#ifndef CoRXdgSurface_H
#define CoRXdgSurface_H

#include "coRState.h"

// wlroot
#include <wlr/types/wlr_xdg_shell.h>

// LibC
#include <stdlib.h>

// Struture
struct coR_xdg_surface {
  struct wlr_xdg_surface *xdgSurface;

  // Listeners
  struct wl_listener mapListener;
  struct wl_listener unMapListener;
  struct wl_listener destroyListener;

  // List
  struct wl_list link;
};

// Methods
// static void mapXdgSurfaceHandler(struct wl_listener *listener, void *data);
// static void unmapXdgSurfaceHandler(struct wl_listener *listener, void *data);
// static void destroyXdgSurfaceHandler(struct wl_listener *listener, void *data);
void newXdgSurfaceHandler(struct wl_listener *listener, void *data) ;


#endif
