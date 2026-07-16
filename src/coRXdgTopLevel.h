#ifndef CoRXdgSurface_H
#define CoRXdgSurface_H
#pragma once

// wlroot
#include <wayland-util.h>
#include <wlr/types/wlr_xdg_shell.h>

// Struture
struct coR_xdg_toplevel {
  struct wlr_xdg_toplevel *xdgTopLevel;
  struct coR_state *coRState;

  // Surface arrangement
  int posX;
  int posY;
  float sizeX;
  float sizeY;
  int onWorkspaceNum;

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
int splitXdgTopLevel(struct coR_xdg_toplevel *toSplit,
                      struct coR_xdg_toplevel *newXdgTopLevel);
void resizeTopLevel(struct coR_xdg_toplevel *resizingTopLevel,
                    struct coR_state *coRState, int startCursorPosX,
                    int startCursorPosY, int startSizeX, int startSizeY,
                    int startPosX, int startPosY);
#endif
