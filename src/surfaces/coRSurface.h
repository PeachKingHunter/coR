#ifndef Surface_Abstraction_H
#define Surface_Abstraction_H
#include "coRLayerSurface.h"
#pragma once

#include "../coRState.h"
#include "../coRState.h"
#include <wlr/xwayland/xwayland.h>


// WARNNING: void *coRSurface can be coRXdgTopLevel, coRXSurface or
// coRLayerSurface (coRLayerSurface not yet implemented in these functions)

#define TYPE_XDG_TOPLEVEL 0
#define TYPE_XSURFACE 1

// Structure
struct coR_surface {
  // Should not be moved
  char type;
  struct wl_list link;

  // Main components
  void *surfaceAbs;
  struct coR_state *coRState;
  int onWorkspaceNum;

  // Placement
  int posX, posY;
  float sizeX, sizeY;
};

int surfaceIsFullScreen(struct coR_surface *coRSurface);
struct wlr_scene_node *surfaceGetNode(struct coR_surface  *coRSurface);
struct wlr_surface *surfaceGetSurface(struct coR_surface  *coRSurface);

int surfaceSplit(struct coR_surface *toSplit, struct coR_surface *newCoRSurface);

/*
  Just change the size/position of an surface with a coR_surface
*/
int surfaceSetSize(struct coR_surface  *coRSurface, float newSizeX, float newSizeY);
int surfaceSetPos(struct coR_surface  *coRSurface, float newPosX, float newPosY);

/*
  Just change the size/position of an surface with a coR_surface
  But not permanently (don't change the value in the coR_xdg_toplevel structure)
*/
int surfaceSetSizeTemp(struct coR_surface  *coRSurface, float newSizeX, float newSizeY);
int surfaceSetPosTemp(struct coR_surface  *coRSurface, float newPosX, float newPosY);

// Set the mode fullscreen of an surface
int surfaceSetFullscreenMode(struct coR_surface *coRSurface, bool mode);
void surfaceChangeFullscreen(struct coR_state *coRState, struct coR_surface *coRSurface);

#endif
