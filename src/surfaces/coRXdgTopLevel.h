#ifndef CoRXdgSurface_H
#define CoRXdgSurface_H
#pragma once

#include "coRSurface.h"

#include <stddef.h>
#include <wayland-util.h>

// wlroot
#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>

#define TYPE_XDG_TOPLEVEL 0

// Struture
struct coR_xdg_toplevel {
  struct coR_surface coRSurface;

  // Listeners
  struct wl_listener mapListener;
  struct wl_listener unMapListener;
  struct wl_listener destroyListener;
  struct wl_listener commitListener;
  struct wl_listener fullscreenListener;

  // Other
  struct wlr_xdg_toplevel_decoration_v1 *decoration;
};

// Methods
void newXdgTopLevelHandler(struct wl_listener *listener, void *data);

void resizeTopLevel(struct coR_surface *resizingTopLevel,
                    struct coR_state *coRState, int startCursorPosX,
                    int startCursorPosY, int startSizeX, int startSizeY,
                    int startPosX, int startPosY);

/*
  Resize all the surface in xdgTopLevelsList (on X/Y axis) for take an free area
  determined by start....X/Y
  -> Return 0 for no changement
  -> Return 1 for minimum one surface have size changed
*/
int resizeXOnEmptyArea(int startPosX, int startPosY, int startSizeX,
                       int startSizeY, struct wl_list *xdgTopLevelsList);
int resizeYOnEmptyArea(int startPosX, int startPosY, int startSizeX,
                       int startSizeY, struct wl_list *xdgTopLevelsList);

void newDecorationHandler(struct wl_listener *listener, void *data);

#endif
