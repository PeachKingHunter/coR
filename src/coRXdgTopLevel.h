#ifndef CoRXdgSurface_H
#define CoRXdgSurface_H

#include "coRInputs.h"

// wlroot
#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>

// Struture
struct coR_xdg_toplevel {
  struct wlr_xdg_toplevel *xdgTopLevel;
  struct coR_state *coRState;

  // Surface arrangement
  struct coR_xdg_toplevel *shrunkedTopLevel;
  struct coR_xdg_toplevel *shrunkerTopLevel;
  int posX;
  int posY;

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
void splitXdgTopLevel(struct coR_xdg_toplevel *toSplit,
                      struct coR_xdg_toplevel *newXdgTopLevel);

#endif
