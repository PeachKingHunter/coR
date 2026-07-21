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

/*
  Just change the size/position of an xdgTopLevel with a coR_xdg_toplevel
*/
int setXdgTopLevelSize(struct coR_xdg_toplevel *xdgTopLevel, float newSizeX, float newSizeY);
int setXdgTopLevelPos(struct coR_xdg_toplevel *xdgTopLevel, float newPosX, float newPosY);

/*
  Just change the size/position of an xdgTopLevel with a coR_xdg_toplevel
  But not permanently (don't change the value in the coR_xdg_toplevel structure)
*/
int setXdgTopLevelSizeTemp(struct coR_xdg_toplevel *xdgTopLevel, float newSizeX, float newSizeY);
int setXdgTopLevelPosTemp(struct coR_xdg_toplevel *xdgTopLevel, float newPosX, float newPosY);


int splitXdgTopLevel(struct coR_xdg_toplevel *toSplit,
                     struct coR_xdg_toplevel *newXdgTopLevel);
void resizeTopLevel(struct coR_xdg_toplevel *resizingTopLevel,
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
#endif
