#include "coRXdgTopLevel.h"

#include "../coRState.h"
#include "../inputs/coRCursor.h"
#include "../inputs/coRInputs.h"
#include "coRSurface.h"
#include "src/surfaces/coRLayerSurface.h"
#include "wlr/types/wlr_xdg_decoration_v1.h"
#include "wlr/util/box.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <wayland-util.h>

#define EPSILON 2

static void commitXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  // printf("-> commitXdgTopLevelHandler\n");
  // Variables
  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, commitListener);
  struct coR_surface *coRSurface = (struct coR_surface *)coRXdgTopLevel;

  struct wlr_xdg_toplevel *xdgTopLevel = coRSurface->surfaceAbs;
  struct wlr_xdg_surface *xdgSurface = xdgTopLevel->base;

  // if (coRXdgTopLevel->xdgTopLevel->base->initial_commit) {
  if (!xdgSurface->initialized || xdgSurface->configured) {
    return;
  }

  printf("-> first commit XdgSurface\n");
  wlr_xdg_surface_schedule_configure(xdgSurface);

  // Variables
  struct coR_state *coRState = coRSurface->coRState;
  struct coR_surface *focusedCoRSurface = coRState->focusedCoRSurface;
  struct coR_workspace *workspace =
      coRState->workspaces + coRState->focusedWorkspaceNum;

  // Change workspace
  coRSurface->onWorkspaceNum = coRState->focusedWorkspaceNum;
  wlr_scene_node_reparent(surfaceGetNode(coRSurface), workspace->rootNode);

  printf("start P1\n");
  // Change the focus on it
  coRState->focusedSurface = xdgSurface->surface;
  coRState->focusedCoRSurface = coRXdgTopLevel;
  inputsChangeSurfaceToFocus(coRState, xdgSurface->surface, 0, 0);

  printf("start P2\n");
  // If Focused surface is on the focused workspace (by cursor)
  // then split the focused surface in two
  if (focusedCoRSurface != NULL) {
    if (focusedCoRSurface->onWorkspaceNum == coRState->focusedWorkspaceNum) {
      surfaceSplit(focusedCoRSurface, coRSurface);
      wl_list_insert(&workspace->xdgTopLevels, &coRSurface->link);
      return;
    }
  }

  printf("start P3\n");
  // Get another surface because of focused one not on focused workspace
  // And split it
  struct wl_list *xdgTopLevelsList = &workspace->xdgTopLevels;
  if (!wl_list_empty(xdgTopLevelsList)) {
    focusedCoRSurface =
        wl_container_of(xdgTopLevelsList->next, focusedCoRSurface, link);
    surfaceSplit(focusedCoRSurface, coRSurface);
    wl_list_insert(&workspace->xdgTopLevels, &coRSurface->link);
    return;
  }

  printf("start P4\n");
  // Calcul usable_area (not taken by docks)
  struct wlr_box box;
  getUsableArea(coRState, workspace->currentOutput, &box);

  // Take the entire screen (exept docks)
  surfaceSetPos(coRSurface, box.x, box.y);
  surfaceSetSize(coRSurface, box.width, box.height);
  wl_list_insert(&workspace->xdgTopLevels, &coRSurface->link);
}

static void mapXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> map XdgTopLevel\n");

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, mapListener);
  struct coR_state *coRState = coRXdgTopLevel->coRSurface.coRState;

  struct wlr_xdg_toplevel *xdgTopLevel = coRXdgTopLevel->coRSurface.surfaceAbs;
  struct wlr_surface *surface = xdgTopLevel->base->surface;
  inputsChangeSurfaceToFocus(coRState, surface, 0, 0);

  if (coRXdgTopLevel->decoration != NULL) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
        coRXdgTopLevel->decoration,
        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
  }
}

static void unmapXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> unmap XdgTopLevel\n");

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, unMapListener);
  struct coR_state *coRState = coRXdgTopLevel->coRSurface.coRState;

  struct wlr_xdg_toplevel *xdgTopLevel = coRXdgTopLevel->coRSurface.surfaceAbs;
  struct wlr_surface *surface = xdgTopLevel->base->surface;

  if (coRState->focusedSurface == surface) {
    coRState->focusedSurface = NULL;
    coRState->focusedCoRSurface = NULL;
    wlr_seat_keyboard_clear_focus(coRState->seat);
    wlr_seat_pointer_clear_focus(coRState->seat);
  }
}

static void destroyXdgTopLevelHandler(struct wl_listener *listener,
                                      void *data) {
  printf("-> destroy XdgTopLevel\n");
  /*
    0. Resize, Reposition les surfaces
    1. Retirer de la liste dans struct coR_state
    2. Retire les listeners de leur listes
    3. Clear la mémoire utilisé
  */

  // Variables
  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, destroyListener);
  struct coR_surface *coRSurface = (struct coR_surface *)coRXdgTopLevel;
  struct coR_state *coRState = coRSurface->coRState;

  // 0. Resize all surface to take the place left
  // Variables
  int startPosX = coRSurface->posX;
  int startPosY = coRSurface->posY;
  float startSizeX = coRSurface->sizeX;
  float startSizeY = coRSurface->sizeY;

  struct coR_workspace *lastWorkspace =
      coRState->workspaces + coRSurface->onWorkspaceNum;
  struct wl_list *lastXdgTopLevelsList = &lastWorkspace->xdgTopLevels;

  // Resize all surface to take the place left
  if (startSizeX < startSizeY) {
    // Resize on X axis
    if (resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                           lastXdgTopLevelsList)) {
      goto endResizeInDestroyFunc;
    }
  }

  // Resize on Y axis
  if (resizeYOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                         lastXdgTopLevelsList)) {
    goto endResizeInDestroyFunc;
  }

  // Resize on X axis
  resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                     lastXdgTopLevelsList);

endResizeInDestroyFunc:
  // 1.
  wl_list_remove(&coRSurface->link);

  // 2.
  wl_list_remove(&coRXdgTopLevel->mapListener.link);
  wl_list_remove(&coRXdgTopLevel->unMapListener.link);
  wl_list_remove(&coRXdgTopLevel->destroyListener.link);
  wl_list_remove(&coRXdgTopLevel->commitListener.link);
  wl_list_remove(&coRXdgTopLevel->fullscreenListener.link);

  // 3.
  free(coRXdgTopLevel);
  printf("<- destroy XdgTopLevel\n");
}

static void fullscreenXdgTopLevelHandler(struct wl_listener *listener,
                                         void *data) {
  printf("-> fullscreen request XdgTopLevel\n");

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, fullscreenListener);
  struct coR_state *coRState = coRXdgTopLevel->coRSurface.coRState;

  surfaceChangeFullscreen(coRState, (struct coR_surface *)coRXdgTopLevel);
}

void newXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> obtain new xdg TopLevel\n");

  struct wlr_xdg_surface *surface = data;
  printf("new xdg surface role=%d\n", surface->role);

  if (surface->role == WLR_XDG_SURFACE_ROLE_POPUP)
    return;
  /*
    1.Structure de donnée
    2.Stockage (liste) -> Pas ici mais dans map
    3.Scene for position management
    4.Decoration
    5.Listeners
  */

  // Variables
  struct wlr_xdg_toplevel *xdgTopLevel = data;
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newXdgTopLevelListener);

  // 1.
  struct coR_xdg_toplevel *coRXdgTopLevel =
      calloc(1, sizeof(struct coR_xdg_toplevel));
  if (coRXdgTopLevel == NULL) {
    printf("newXdgTopLevelHandler -> Malloc failled\n");
    return;
  }
  struct coR_surface *coRSurface = (struct coR_surface *)coRXdgTopLevel;
  coRSurface->surfaceAbs = xdgTopLevel;
  coRSurface->coRState = coRState;
  coRSurface->posX = 0;
  coRSurface->posY = 0;
  coRSurface->type = TYPE_XDG_TOPLEVEL;

  // 2.
  // wl_list_insert(&coRState->xdgTopLevels, &coRXdgTopLevel->link);

  // 3.
  struct wlr_scene_tree *topLevelSceneTree = wlr_scene_xdg_surface_create(
      coRState->workspaces[coRState->focusedWorkspaceNum].rootNode,
      xdgTopLevel->base);
  coRSurface->onWorkspaceNum = coRState->focusedWorkspaceNum;

  topLevelSceneTree->node.data = coRXdgTopLevel;
  xdgTopLevel->base->data = topLevelSceneTree;
  xdgTopLevel->base->surface->data = coRXdgTopLevel;
  printf("-> TopLevel saved\n");

  // 4.
  float color[4] = {0.8, 0.8, 0.8, 1};

  // Top border
  coRSurface->decoration[0] =
      wlr_scene_rect_create(topLevelSceneTree, coRSurface->sizeX, 2, color);
  wlr_scene_node_set_position(&coRSurface->decoration[0]->node, 0, 0);

  // Left border
  coRSurface->decoration[1] =
      wlr_scene_rect_create(topLevelSceneTree, 2, coRSurface->sizeY, color);
  wlr_scene_node_set_position(&coRSurface->decoration[1]->node, 0, 0);

  // Down border
  coRSurface->decoration[2] =
      wlr_scene_rect_create(topLevelSceneTree, coRSurface->sizeX, 2, color);
  wlr_scene_node_set_position(&coRSurface->decoration[2]->node, 0,
                              coRSurface->sizeY - 2);

  // Right border
  coRSurface->decoration[3] =
      wlr_scene_rect_create(topLevelSceneTree, 2, coRSurface->sizeY, color);
  wlr_scene_node_set_position(&coRSurface->decoration[3]->node,
                              coRSurface->sizeX - 2, 0);

  // 5.
  coRXdgTopLevel->mapListener.notify = mapXdgTopLevelHandler;
  wl_signal_add(&xdgTopLevel->base->surface->events.map,
                &coRXdgTopLevel->mapListener);

  coRXdgTopLevel->unMapListener.notify = unmapXdgTopLevelHandler;
  wl_signal_add(&xdgTopLevel->base->surface->events.unmap,
                &coRXdgTopLevel->unMapListener);

  coRXdgTopLevel->commitListener.notify = commitXdgTopLevelHandler;
  wl_signal_add(&xdgTopLevel->base->surface->events.commit,
                &coRXdgTopLevel->commitListener);

  coRXdgTopLevel->destroyListener.notify = destroyXdgTopLevelHandler;
  wl_signal_add(&xdgTopLevel->events.destroy, &coRXdgTopLevel->destroyListener);

  coRXdgTopLevel->fullscreenListener.notify = fullscreenXdgTopLevelHandler;
  wl_signal_add(&xdgTopLevel->events.request_fullscreen,
                &coRXdgTopLevel->fullscreenListener);
}

#define SURFACE_MIN_SIZE 50
/*
  Resize toplevel for cursor motion
  startPosX & startPosY are surface pos
  startCursorPosX & startCursorPosY are cursor pos
  startSizeX and startSizeY are size of the resizingTopLevel at default
  Axis X & Axis Y separated
*/
void resizeTopLevelX(struct coR_surface *resizingTopLevel,
                     struct coR_state *coRState, int startCursorPosX,
                     int startCursorPosY, int startSizeX, int startSizeY,
                     int startPosX, int startPosY) {
  printf("-> resizeTopLevel\n");
  // Verif args
  if (resizingTopLevel == NULL || coRState == NULL)
    return;

  struct coR_workspace *workspace =
      coRState->workspaces + coRState->focusedWorkspaceNum;
  struct wl_list *xdgTopLevelsList = &workspace->xdgTopLevels;

  // More than 1 surface
  if (wl_list_length(xdgTopLevelsList) <= 1)
    return;

  // Var
  struct wlr_output *output = coRState->focusedOutput;

  // Get usable area of the output
  struct wlr_box box;
  getUsableArea(coRState, output, &box);

  // -- resize in axis X --
  // Variables
  int deltaX = (int)(coRState->cursor->x) - startCursorPosX;
  float currentSizeX = resizingTopLevel->sizeX;
  int currentPosX = resizingTopLevel->posX;
  float newSizeX = currentSizeX;
  int newPosX = startPosX;

  // get the side to resize
  float threshold = startPosX + startSizeX / 2.;
  int side = startCursorPosX -
                 coRState->workspaces[resizingTopLevel->onWorkspaceNum].posX <
             threshold;
  int possibleSides = 2;

  // No resize sides glued to output's border
  if (startPosX <= box.x + EPSILON) {
    side = 0;
    possibleSides--;
  }

  if (startPosX + startSizeX + EPSILON >= box.width + box.x) {
    side = 1;
    possibleSides--;
  }

  // Change new value
  if (possibleSides == 0) {
    return;
  }

  // Resize with the perfect side
  if (side) {
    // Resize left side
    newPosX = startPosX + deltaX;
    newSizeX = startSizeX - deltaX;
  } else {
    // Resize right side
    newSizeX = startSizeX + deltaX;
  }

  // Verif minimal size
  if (newSizeX <= SURFACE_MIN_SIZE)
    return;

  // No resize sides glued to output's border
  if (newPosX < box.x - EPSILON / 2.) {
    return;
  }
  if (newPosX + newSizeX > box.width + box.x + EPSILON / 2.) {
    return;
  }

  // Resize with the perfect side
  if (side) {
    // Left side
    struct coR_surface *tmpCoRSurface;
    // Verif before resizing
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      // Snap other surfaces on same column
      if (tmpCoRSurface->posX == currentPosX) {
        if (tmpCoRSurface->sizeX - (newPosX - currentPosX) < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }

      // Resize surface side by side and at left with resizingTopLevel
      if (fabsf(tmpCoRSurface->posX + tmpCoRSurface->sizeX - currentPosX) < 1) {
        if (tmpCoRSurface->sizeX + (newPosX - currentPosX) < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }
    }

    // Resize other surfaces
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      int deltaPosX = newPosX - currentPosX;

      // Snap other surfaces on same column
      if (tmpCoRSurface->posX == currentPosX) {
        surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX + deltaPosX,
                      tmpCoRSurface->posY);
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX - deltaPosX,
                       tmpCoRSurface->sizeY);
      }

      // Resize surface side by side and at left with resizingTopLevel
      if (fabsf(tmpCoRSurface->posX + tmpCoRSurface->sizeX - currentPosX) < 1) {
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX + deltaPosX,
                       tmpCoRSurface->sizeY);
      }
    }

  } else {
    // Right side
    struct coR_surface *tmpCoRSurface;
    // Verif before resizing
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      // Variables
      int tmpPosX = tmpCoRSurface->posX;
      float tmpSizeX = tmpCoRSurface->sizeX;

      int deltaSizeX = newSizeX - currentSizeX;

      // Snap other surfaces on same column
      if (fabsf(tmpPosX + tmpSizeX - currentPosX - currentSizeX) < 1) {
        if (tmpCoRSurface->sizeX + deltaSizeX < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }

      // Resize surface side by side and at right with resizingTopLevel
      if (fabsf(resizingTopLevel->posX + resizingTopLevel->sizeX - tmpPosX) <
          1) {
        if (tmpSizeX - deltaSizeX < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }
    }

    // Resize other surfaces
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      // Variables
      int tmpPosX = tmpCoRSurface->posX;
      float tmpSizeX = tmpCoRSurface->sizeX;

      int deltaSizeX = newSizeX - currentSizeX;

      // Snap other surfaces on same column
      if (fabsf(tmpPosX + tmpSizeX - currentPosX - currentSizeX) < 1) {
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX + deltaSizeX,
                       tmpCoRSurface->sizeY);
      }

      // Resize surface side by side and at right with resizingTopLevel
      if (fabsf(resizingTopLevel->posX + resizingTopLevel->sizeX -
                tmpCoRSurface->posX) < 1) {
        surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX + deltaSizeX,
                      tmpCoRSurface->posY);
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX - deltaSizeX,
                       tmpCoRSurface->sizeY);
      }
    }
  }

  surfaceSetPos(resizingTopLevel, newPosX, resizingTopLevel->posY);
  surfaceSetSize(resizingTopLevel, newSizeX, resizingTopLevel->sizeY);
}

/*
  Copy of resizeTopLevelX with AI for ajust x axis to y axis
*/
void resizeTopLevelY(struct coR_surface *resizingTopLevel,
                     struct coR_state *coRState, int startCursorPosX,
                     int startCursorPosY, int startSizeX, int startSizeY,
                     int startPosX, int startPosY) {
  printf("-> resizeTopLevel\n");
  // Verif args
  if (resizingTopLevel == NULL || coRState == NULL)
    return;

  struct coR_workspace *workspace =
      coRState->workspaces + coRState->focusedWorkspaceNum;
  struct wl_list *xdgTopLevelsList = &workspace->xdgTopLevels;

  // More than 1 surface
  if (wl_list_length(xdgTopLevelsList) <= 1)
    return;

  // Var
  struct wlr_output *output = coRState->focusedOutput;

  // Get usable area of the output
  struct wlr_box box;
  getUsableArea(coRState, output, &box);

  // -- resize in axis Y --
  // Variables
  int deltaY = (int)(coRState->cursor->y) - startCursorPosY;
  float currentSizeY = resizingTopLevel->sizeY;
  int currentPosY = resizingTopLevel->posY;
  float newSizeY = currentSizeY;
  int newPosY = startPosY;

  // get the side to resize
  float threshold = startPosY + startSizeY / 2.;
  int side = startCursorPosY -
                 coRState->workspaces[resizingTopLevel->onWorkspaceNum].posY <
             threshold;
  int possibleSides = 2;

  // No resize sides glued to output's border
  if (startPosY <= box.y + EPSILON) {
    side = 0;
    possibleSides--;
  }

  if (startPosY + startSizeY + EPSILON >= box.height + box.y) {
    side = 1;
    possibleSides--;
  }

  // Change new value
  if (possibleSides == 0) {
    return;
  }

  // Resize with the perfect side
  if (side) {
    // Resize top side
    newPosY = startPosY + deltaY;
    newSizeY = startSizeY - deltaY;
  } else {
    // Resize bottom side
    newSizeY = startSizeY + deltaY;
  }

  // Verif minimal size
  if (newSizeY <= SURFACE_MIN_SIZE)
    return;

  // No resize sides glued to output's border
  if (newPosY < box.y - EPSILON / 2.) {
    return;
  }
  if (newPosY + newSizeY > box.height + box.y + EPSILON / 2.) {
    return;
  }

  // Resize with the perfect side
  if (side) {
    // Top side
    struct coR_surface *tmpCoRSurface;
    // Verif before resizing
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      // Snap other surfaces on same row
      if (tmpCoRSurface->posY == currentPosY) {
        if (tmpCoRSurface->sizeY - (newPosY - currentPosY) < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }

      // Resize surface side by side and at top with resizingTopLevel
      if (fabsf(tmpCoRSurface->posY + tmpCoRSurface->sizeY - currentPosY) < 1) {
        if (tmpCoRSurface->sizeY + (newPosY - currentPosY) < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }
    }

    // Resize other surfaces
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      int deltaPosY = newPosY - currentPosY;

      // Snap other surfaces on same row
      if (tmpCoRSurface->posY == currentPosY) {
        surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX,
                      tmpCoRSurface->posY + deltaPosY);
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                       tmpCoRSurface->sizeY - deltaPosY);
      }

      // Resize surface side by side and at top with resizingTopLevel
      if (fabsf(tmpCoRSurface->posY + tmpCoRSurface->sizeY - currentPosY) < 1) {
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                       tmpCoRSurface->sizeY + deltaPosY);
      }
    }

  } else {
    // Bottom side
    struct coR_surface *tmpCoRSurface;
    // Verif before resizing
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      // Variables
      int tmpPosY = tmpCoRSurface->posY;
      float tmpSizeY = tmpCoRSurface->sizeY;

      int deltaSizeY = newSizeY - currentSizeY;

      // Snap other surfaces on same row
      if (fabsf(tmpPosY + tmpSizeY - currentPosY - currentSizeY) < 1) {
        if (tmpCoRSurface->sizeY + deltaSizeY < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }

      // Resize surface side by side and at bottom with resizingTopLevel
      if (fabsf(resizingTopLevel->posY + resizingTopLevel->sizeY - tmpPosY) <
          1) {
        if (tmpSizeY - deltaSizeY < SURFACE_MIN_SIZE) {
          printf("STOP resizing\n");
          return;
        }
      }
    }

    // Resize other surfaces
    wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
      // Skip himself
      if (tmpCoRSurface == resizingTopLevel)
        continue;

      // Variables
      int tmpPosY = tmpCoRSurface->posY;
      float tmpSizeY = tmpCoRSurface->sizeY;

      int deltaSizeY = newSizeY - currentSizeY;

      // Snap other surfaces on same row
      if (fabsf(tmpPosY + tmpSizeY - currentPosY - currentSizeY) < 1) {
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                       tmpCoRSurface->sizeY + deltaSizeY);
      }

      // Resize surface side by side and at bottom with resizingTopLevel
      if (fabsf(resizingTopLevel->posY + resizingTopLevel->sizeY -
                tmpCoRSurface->posY) < 1) {
        surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX,
                      tmpCoRSurface->posY + deltaSizeY);
        surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                       tmpCoRSurface->sizeY - deltaSizeY);
      }
    }
  }

  surfaceSetPos(resizingTopLevel, resizingTopLevel->posX, newPosY);
  surfaceSetSize(resizingTopLevel, resizingTopLevel->sizeX, newSizeY);
}

void resizeTopLevel(struct coR_surface *resizingTopLevel,
                    struct coR_state *coRState, int startCursorPosX,
                    int startCursorPosY, int startSizeX, int startSizeY,
                    int startPosX, int startPosY) {
  resizeTopLevelX(resizingTopLevel, coRState, startCursorPosX, startCursorPosY,
                  startSizeX, startSizeY, startPosX, startPosY);
  resizeTopLevelY(resizingTopLevel, coRState, startCursorPosX, startCursorPosY,
                  startSizeX, startSizeY, startPosX, startPosY);
}

/*
  Resize all the surface in xdgTopLevelsList (on X axis) for take an free area
  determined by start....X/Y
  -> Return 0 for no changement
  -> Return 1 for minimum one surface have size changed
*/
int resizeXOnEmptyArea(int startPosX, int startPosY, float startSizeX,
                       float startSizeY, struct wl_list *xdgTopLevelsList) {
  int sizeChanged = 0;
  struct coR_surface *tmpCoRSurface;

  // ---- Left surface resize to right

  // Verif loop
  float totalResize = 0;
  wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpCoRSurface->posX;
    int tmpPosY = tmpCoRSurface->posY;
    float tmpSizeX = tmpCoRSurface->sizeX;
    float tmpSizeY = tmpCoRSurface->sizeY;

    // Is resizable on the empty area
    if (tmpPosY < startPosY - EPSILON ||
        tmpPosY + tmpSizeY > startPosY + startSizeY + EPSILON)
      continue;

    // Is side by side and at left of the destroyed one
    if (fabsf(tmpPosX + tmpSizeX - startPosX) > EPSILON) {
      continue;
    }

    totalResize += tmpCoRSurface->sizeY;
  }

  if (fabsf(totalResize - startSizeY) < EPSILON) {
    wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
      // Variables
      int tmpPosX = tmpCoRSurface->posX;
      int tmpPosY = tmpCoRSurface->posY;
      float tmpSizeX = tmpCoRSurface->sizeX;
      float tmpSizeY = tmpCoRSurface->sizeY;

      // Is resizable on the empty area
      if (tmpPosY < startPosY - EPSILON ||
          tmpPosY + tmpSizeY > startPosY + startSizeY + EPSILON)
        continue;

      // Is side by side and at left of the destroyed one
      if (fabsf(tmpPosX + tmpSizeX - startPosX) > EPSILON) {
        continue;
      }

      // If at right move it (not in this part
      // surfaceSetPos(tmpCoRSurface, startPosX, tmpCoRSurface->posY);
      surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX + startSizeX,
                     tmpCoRSurface->sizeY);
      sizeChanged = 1;
    }

    if (sizeChanged == 1)
      return sizeChanged;
  }

  // ---- Right surface resize to left

  // Verif loop
  totalResize = 0;
  wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpCoRSurface->posX;
    int tmpPosY = tmpCoRSurface->posY;
    float tmpSizeY = tmpCoRSurface->sizeY;

    // Is resizable on the empty area
    if (tmpPosY < startPosY - EPSILON ||
        tmpPosY + tmpSizeY > startPosY + startSizeY + EPSILON)
      continue;

    // Is side by side and at right of the destroyed one
    if (fabsf(startPosX + startSizeX - tmpPosX) > EPSILON) {
      continue;
    }

    totalResize += tmpCoRSurface->sizeY;
  }

  if (fabsf(totalResize - startSizeY) < EPSILON) {
    wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
      // Variables
      int tmpPosX = tmpCoRSurface->posX;
      int tmpPosY = tmpCoRSurface->posY;
      float tmpSizeY = tmpCoRSurface->sizeY;

      // Is resizable on the empty area
      if (tmpPosY < startPosY - EPSILON ||
          tmpPosY + tmpSizeY > startPosY + startSizeY + EPSILON)
        continue;

      // Is side by side and at right of the destroyed one
      if (fabsf(startPosX + startSizeX - tmpPosX) > EPSILON) {
        continue;
      }

      surfaceSetPos(tmpCoRSurface, startPosX, tmpCoRSurface->posY);
      surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX + startSizeX,
                     tmpCoRSurface->sizeY);
      sizeChanged = 1;
    }
    return sizeChanged;
  }

  return 0;
}

/*
  Resize all the surface in xdgTopLevelsList (on Y axis) for take an free area
  determined by start....X/Y
  -> Return 0 for no changement
  -> Return 1 for minimum one surface have size changed
*/

int resizeYOnEmptyArea(int startPosX, int startPosY, float startSizeX,
                       float startSizeY, struct wl_list *xdgTopLevelsList) {
  int sizeChanged = 0;
  struct coR_surface *tmpCoRSurface;

  // ---- Top surface resize to bottom

  // Verif loop
  float totalResize = 0;
  wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpCoRSurface->posX;
    int tmpPosY = tmpCoRSurface->posY;
    float tmpSizeX = tmpCoRSurface->sizeX;
    float tmpSizeY = tmpCoRSurface->sizeY;

    // Is resizable on the empty area
    if (tmpPosX < startPosX - EPSILON ||
        tmpPosX + tmpSizeX > startPosX + startSizeX + EPSILON)
      continue;

    // Is side by side and at top of the destroyed one
    if (fabsf(tmpPosY + tmpSizeY - startPosY) > EPSILON) {
      continue;
    }

    totalResize += tmpCoRSurface->sizeX;
  }

  if (fabsf(totalResize - startSizeX) < 1) {
    wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
      // Variables
      int tmpPosX = tmpCoRSurface->posX;
      int tmpPosY = tmpCoRSurface->posY;
      float tmpSizeX = tmpCoRSurface->sizeX;
      float tmpSizeY = tmpCoRSurface->sizeY;

      // Is resizable on the empty area
      if (tmpPosX < startPosX - EPSILON ||
          tmpPosX + tmpSizeX > startPosX + startSizeX + EPSILON)
        continue;

      // Is side by side and at top of the destroyed one
      if (fabsf(tmpPosY + tmpSizeY - startPosY) > EPSILON) {
        continue;
      }

      // If at bottom move it (not in this part)
      // surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX, startPosY);
      surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                     tmpCoRSurface->sizeY + startSizeY);
      sizeChanged = 1;
    }

    if (sizeChanged == 1)
      return sizeChanged;
  }

  // ---- Bottom surface resize to top

  // Verif loop
  totalResize = 0;
  wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpCoRSurface->posX;
    int tmpPosY = tmpCoRSurface->posY;
    float tmpSizeX = tmpCoRSurface->sizeX;

    // Is resizable on the empty area
    if (tmpPosX < startPosX - EPSILON ||
        tmpPosX + tmpSizeX > startPosX + startSizeX + EPSILON)
      continue;

    // Is side by side and at bottom of the destroyed one
    if (fabsf(startPosY + startSizeY - tmpPosY) > EPSILON) {
      continue;
    }

    totalResize += tmpCoRSurface->sizeX;
  }

  if (fabsf(totalResize - startSizeX) < EPSILON) {
    wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
      // Variables
      int tmpPosX = tmpCoRSurface->posX;
      int tmpPosY = tmpCoRSurface->posY;
      float tmpSizeX = tmpCoRSurface->sizeX;

      // Is resizable on the empty area
      if (tmpPosX < startPosX - EPSILON ||
          tmpPosX + tmpSizeX > startPosX + startSizeX + EPSILON)
        continue;

      // Is side by side and at bottom of the destroyed one
      if (fabsf(startPosY + startSizeY - tmpPosY) > EPSILON) {
        continue;
      }

      surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX, startPosY);
      surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                     tmpCoRSurface->sizeY + startSizeY);
      sizeChanged = 1;
    }
    return sizeChanged;
  }

  return 0;
}

void newDecorationHandler(struct wl_listener *listener, void *data) {
  struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
  struct coR_xdg_toplevel *coRXdgTopLevel =
      decoration->toplevel->base->surface->data;
  coRXdgTopLevel->decoration = decoration;
}
