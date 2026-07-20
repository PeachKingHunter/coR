#include "coRXdgTopLevel.h"
#include "coRState.h"
#include "inputs/coRCursor.h"
#include "inputs/coRInputs.h"
#include <stddef.h>
#include <stdio.h>
#include <wayland-util.h>

static void commitXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  // printf("-> commitXdgTopLevelHandler\n");
  // Variables
  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, commitListener);
  struct wlr_xdg_surface *xdgSurface = coRXdgTopLevel->xdgTopLevel->base;

  // if (coRXdgTopLevel->xdgTopLevel->base->initial_commit) {
  if (!xdgSurface->initialized || xdgSurface->configured) {
    return;
  }

  printf("-> first commit XdgSurface\n");
  wlr_xdg_surface_schedule_configure(xdgSurface);

  // Variables
  struct coR_state *coRState = coRXdgTopLevel->coRState;
  struct wlr_surface *focusedSurface = coRXdgTopLevel->coRState->focusedSurface;
  struct coR_workspace *workspace =
      coRState->workspaces + coRState->focusedWorkspaceNum;

  printf("start P1\n");
  // Change the focus on it
  coRXdgTopLevel->coRState->focusedSurface =
      coRXdgTopLevel->xdgTopLevel->base->surface;

  printf("start P2\n");
  // If Focused surface is on the focused workspace (by cursor)
  // then split the focused surface in two
  if (focusedSurface != NULL) {
    struct coR_xdg_toplevel *focusedCoRXdgTopLevel = focusedSurface->data;
    if (focusedCoRXdgTopLevel->onWorkspaceNum ==
        coRState->focusedWorkspaceNum) {
      splitXdgTopLevel(focusedCoRXdgTopLevel, coRXdgTopLevel);
      wl_list_insert(&workspace->xdgTopLevels, &coRXdgTopLevel->link);
      return;
    }
  }

  printf("start P3\n");
  // Get another surface because of focused one not on focused workspace
  // And split it
  struct wl_list *xdgTopLevelsList = &workspace->xdgTopLevels;
  if (!wl_list_empty(xdgTopLevelsList)) {
    struct coR_xdg_toplevel *xdgTopLevel =
        wl_container_of(xdgTopLevelsList->next, xdgTopLevel, link);
    focusedSurface = xdgTopLevel->xdgTopLevel->base->surface;
    splitXdgTopLevel(xdgTopLevel, coRXdgTopLevel);
    wl_list_insert(&workspace->xdgTopLevels, &coRXdgTopLevel->link);
    return;
  }

  printf("start P4\n");
  wl_list_insert(&workspace->xdgTopLevels, &coRXdgTopLevel->link);
  setXdgTopLevelPos(coRXdgTopLevel, 0, 0);
  setXdgTopLevelSize(coRXdgTopLevel, workspace->currentOutput->width,
                     workspace->currentOutput->height);
}

static void mapXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> map XdgTopLevel\n");

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, mapListener);
  struct coR_state *coRState = coRXdgTopLevel->coRState;

  inputsChangeSurfaceToFocus(coRState,
                             coRXdgTopLevel->xdgTopLevel->base->surface, 0, 0);
}

static void unmapXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> unmap XdgTopLevel\n");

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, unMapListener);
  struct coR_state *coRState = coRXdgTopLevel->coRState;

  if (coRState->focusedSurface == coRXdgTopLevel->xdgTopLevel->base->surface) {
    coRState->focusedSurface = NULL;
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
  // struct wlr_xdg_surface *xdgSurface = data;

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, destroyListener);
  struct coR_state *coRState = coRXdgTopLevel->coRState;

  // 0.
  // TODO VERIF: resize all surface to take the place left
  // Variables
  struct coR_xdg_toplevel *movingTopLevel = coRXdgTopLevel;
  int startPosX = movingTopLevel->posX;
  int startPosY = movingTopLevel->posY;
  int startSizeX = movingTopLevel->sizeX;
  int startSizeY = movingTopLevel->sizeY;

  struct coR_workspace *lastWorkspace =
      coRState->workspaces + movingTopLevel->onWorkspaceNum;
  struct wl_list *lastXdgTopLevelsList = &lastWorkspace->xdgTopLevels;

  // Resize all surface to take the place left | TODO -> VERIF:
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
  wl_list_remove(&coRXdgTopLevel->link);

  // 2.
  wl_list_remove(&coRXdgTopLevel->mapListener.link);
  wl_list_remove(&coRXdgTopLevel->unMapListener.link);
  wl_list_remove(&coRXdgTopLevel->destroyListener.link);
  wl_list_remove(&coRXdgTopLevel->commitListener.link);

  // 3.
  free(coRXdgTopLevel);
  printf("<- destroy XdgTopLevel\n");
}

void newXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> obtain new xdg TopLevel\n");
  struct wlr_xdg_surface *surface = data;
  printf("new xdg surface role=%d\n", surface->role);

  if (surface->role == WLR_XDG_SURFACE_ROLE_POPUP)
    return;
  /*
    1.Structure de donnée
    2.Stockage (liste)
    3.Scene for position management
    4.Listeners
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
  coRXdgTopLevel->xdgTopLevel = xdgTopLevel;
  coRXdgTopLevel->coRState = coRState;
  coRXdgTopLevel->posX = 0;
  coRXdgTopLevel->posY = 0;

  // 2.
  // wl_list_insert(&coRState->xdgTopLevels, &coRXdgTopLevel->link);

  // 3.
  struct wlr_scene_tree *topLevelSceneTree = wlr_scene_xdg_surface_create(
      coRState->workspaces[coRState->focusedWorkspaceNum].rootNode,
      xdgTopLevel->base);
  coRXdgTopLevel->onWorkspaceNum = coRState->focusedWorkspaceNum;

  topLevelSceneTree->node.data = coRXdgTopLevel;
  xdgTopLevel->base->data = topLevelSceneTree;
  xdgTopLevel->base->surface->data = coRXdgTopLevel;
  printf("-> TopLevel saved\n");

  // 4.
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
}

int setXdgTopLevelSize(struct coR_xdg_toplevel *xdgTopLevel, float newSizeX,
                       float newSizeY) {
  printf("-> setXdgTopLevelSize\n");
  // Verif entry
  if (xdgTopLevel == NULL)
    return 0;

  if (newSizeX <= xdgTopLevel->xdgTopLevel->current.min_width)
    return 0;

  if (newSizeY <= xdgTopLevel->xdgTopLevel->current.min_height)
    return 0;

  // Change position
  xdgTopLevel->sizeX = newSizeX;
  xdgTopLevel->sizeY = newSizeY;
  return wlr_xdg_toplevel_set_size(xdgTopLevel->xdgTopLevel, xdgTopLevel->sizeX,
                                   xdgTopLevel->sizeY);
}

int setXdgTopLevelPos(struct coR_xdg_toplevel *xdgTopLevel, float newPosX,
                      float newPosY) {
  printf("-> setXdgTopLevelPos\n");
  // Verif entry
  if (xdgTopLevel == NULL)
    return 0;

  // Change position
  xdgTopLevel->posX = newPosX;
  xdgTopLevel->posY = newPosY;

  struct wlr_scene_tree *topLevelSceneTree =
      xdgTopLevel->xdgTopLevel->base->data;
  wlr_scene_node_set_position(&topLevelSceneTree->node, xdgTopLevel->posX,
                              xdgTopLevel->posY);
  return 1;
}

int splitXdgTopLevel(struct coR_xdg_toplevel *toSplit,
                     struct coR_xdg_toplevel *newXdgTopLevel) {
  printf("-> splitXdgTopLevel\n");
  if (toSplit == NULL || newXdgTopLevel == NULL)
    return 0;

  // Variables
  struct wlr_scene_tree *topLevelSceneTreeToSplit =
      toSplit->xdgTopLevel->base->data;
  struct wlr_scene_tree *topLevelSceneTreeNew =
      newXdgTopLevel->xdgTopLevel->base->data;

  if (topLevelSceneTreeToSplit == NULL || topLevelSceneTreeNew == NULL)
    return 0;

  int width = toSplit->xdgTopLevel->current.width;
  int height = toSplit->xdgTopLevel->current.height;
  printf("after variables\n");

  // Devient frère à celui découpé
  wlr_scene_node_reparent(&topLevelSceneTreeNew->node,
                          topLevelSceneTreeToSplit->node.parent);
  printf("after reparent\n");

  // Minimal size
  if (width <= 2 && height <= 2) {
    printf("Too small\n");
    return 0;
  }
  printf("after verif minimal size\n");

  if (width > height) {
    printf("cond width > height entered\n");

    // Ajout à droite ou gauche
    // Position & size de la nouvelle surface
    setXdgTopLevelPos(newXdgTopLevel, toSplit->posX + width / 2.,
                      toSplit->posY);
    setXdgTopLevelSize(newXdgTopLevel, width / 2., height);
    printf("width > height part 1 Ok\n");

    // Resize the parent surface
    setXdgTopLevelSize(toSplit, width / 2., height);

  } else {
    printf("cond width < height entered\n");
    // Ajout en bas ou en haut
    // Position & size de la nouvelle surface
    setXdgTopLevelPos(newXdgTopLevel, toSplit->posX,
                      toSplit->posY + height / 2.);
    setXdgTopLevelSize(newXdgTopLevel, width, height / 2.);
    printf("width < height part 1 Ok\n");

    // Resize the parent surface
    setXdgTopLevelSize(toSplit, width, height / 2.);
  }
  printf("<- splitXdgTopLevel\n");
  return 1;
}

/* Resize toplevel for cursor motion
startPosX & startPosY are surface pos
startCursorPosX & startCursorPosY are cursor pos
startSizeX and startSizeY are size of the resizingTopLevel at default
*/
int lastDeltaX = 0;
int lastDeltaY = 0;
void resizeTopLevel(struct coR_xdg_toplevel *resizingTopLevel,
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
  if (wl_list_length(xdgTopLevelsList) <=
      1) // TODO: change it because N is bigger than 2
    return;

  // Var
  struct wlr_output *output = coRState->focusedOutput;

  // -- resize in axis X --
  // Variables
  int deltaX = (int)(coRState->cursor->x) - startCursorPosX;
  int currentSizeX = resizingTopLevel->sizeX;
  int currentPosX = resizingTopLevel->posX;
  int newSizeX = currentSizeX;
  int newPosX = startPosX;

  // get the side to resize
  float threshold = startPosX + startSizeX / 2.;
  int side = startCursorPosX -
                 coRState->workspaces[resizingTopLevel->onWorkspaceNum].posX <
             threshold;
  int possibleSides = 2;

  // No resize sides glued to output's border
  if (startPosX == 0) {
    side = 0;
    possibleSides--;
  }

  if (startPosX + startSizeX == output->width) {
    side = 1;
    possibleSides--;
  }

  if (possibleSides > 0) {
    // Resize with the perfect side
    if (side) {
      // Resize left side
      newPosX = startPosX + deltaX;
      newSizeX = startSizeX - deltaX;

      // Resize other surfaces
      struct coR_xdg_toplevel *tmpXdgTopLevel;
      wl_list_for_each(tmpXdgTopLevel, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpXdgTopLevel == resizingTopLevel)
          continue;

        // Variables
        int tmpPosX = tmpXdgTopLevel->posX;
        int tmpSizeX = tmpXdgTopLevel->sizeX;
        int lastNewSizeX = startSizeX - lastDeltaX;
        int lastNewPosX = startPosX + lastDeltaX;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosX - currentPosX) < 20) {
          setXdgTopLevelPos(tmpXdgTopLevel,
                            tmpXdgTopLevel->posX + newPosX - currentPosX,
                            tmpXdgTopLevel->posY);
          setXdgTopLevelSize(tmpXdgTopLevel,
                             tmpXdgTopLevel->sizeX - (newPosX - currentPosX),
                             tmpXdgTopLevel->sizeY);
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX - 2 &&
             tmpPosX <= newPosX - 2 + newSizeX) ||
            (tmpPosX + tmpSizeX >= lastNewPosX - 2 &&
             tmpPosX <= lastNewPosX - 2 + lastNewSizeX)) {
          if (tmpPosX + tmpSizeX * 8 / 10 < newPosX + 5) {
            // Resize
            setXdgTopLevelSize(tmpXdgTopLevel, newPosX - tmpPosX,
                               tmpXdgTopLevel->sizeY);
          }
        }
      }
      lastDeltaX = deltaX;

    } else {
      // Resize right side
      newSizeX = startSizeX + deltaX;

      // Resize other surfaces
      struct coR_xdg_toplevel *tmpXdgTopLevel;
      wl_list_for_each(tmpXdgTopLevel, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpXdgTopLevel == resizingTopLevel)
          continue;

        // Variables
        int tmpPosX = tmpXdgTopLevel->posX;
        // int tmpSizeX = tmpXdgTopLevel->xdgTopLevel->current.width;
        int tmpSizeX = tmpXdgTopLevel->sizeX;
        int lastNewSizeX = startSizeX + lastDeltaX;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosX + tmpSizeX - currentPosX - currentSizeX) < 20) {
          setXdgTopLevelSize(tmpXdgTopLevel,
                             tmpXdgTopLevel->sizeX + newSizeX - currentSizeX,
                             tmpXdgTopLevel->sizeY);
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX && tmpPosX <= newPosX + newSizeX) ||
            (tmpPosX + tmpSizeX >= newPosX &&
             tmpPosX <= newPosX + lastNewSizeX)) {
          if (newPosX + newSizeX * 8 / 10 < tmpPosX + 5) {
            // Resize
            int tmpLastSizeX = tmpXdgTopLevel->sizeX;
            setXdgTopLevelSize(tmpXdgTopLevel,
                               tmpPosX + tmpSizeX - newPosX - newSizeX,
                               tmpXdgTopLevel->sizeY);
            // TODO: Move surface
            setXdgTopLevelPos(tmpXdgTopLevel,
                              tmpXdgTopLevel->posX + tmpLastSizeX -
                                  tmpXdgTopLevel->sizeX,
                              tmpXdgTopLevel->posY);
          }
        }
      }
      lastDeltaX = deltaX;
    }
  }
  // ----

  // -- resize in axis Y -- (Copy of axis X)
  // Variables
  int deltaY = (int)(coRState->cursor->y) - startCursorPosY;
  int currentSizeY = resizingTopLevel->sizeY;
  int currentPosY = resizingTopLevel->posY;
  int newSizeY = currentSizeY;
  int newPosY = startPosY;

  // get the side to resize
  threshold = startPosY + startSizeY / 2.;
  // side = startCursorPosY < threshold;
  side = startCursorPosY -
             coRState->workspaces[resizingTopLevel->onWorkspaceNum].posY <
         threshold;
  possibleSides = 2;

  // No resize sides glued to output's border
  if (startPosY == 0) {
    side = 0;
    possibleSides--;
  }

  if (startPosY + startSizeY == output->height) {
    side = 1;
    possibleSides--;
  }

  if (possibleSides > 0) {
    // Resize with the perfect side
    if (side) {
      // Resize top side
      newPosY = startPosY + deltaY;
      newSizeY = startSizeY - deltaY;

      // Resize other surfaces
      struct coR_xdg_toplevel *tmpXdgTopLevel;
      wl_list_for_each(tmpXdgTopLevel, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpXdgTopLevel == resizingTopLevel)
          continue;

        // Variables
        int tmpPosY = tmpXdgTopLevel->posY;
        int tmpSizeY = tmpXdgTopLevel->sizeY;
        int lastNewSizeY = startSizeY - lastDeltaY;
        int lastNewPosY = startPosY + lastDeltaY;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosY - currentPosY) < 20) {
          setXdgTopLevelPos(tmpXdgTopLevel, tmpXdgTopLevel->posX,
                            tmpXdgTopLevel->posY + newPosY - currentPosY);
          setXdgTopLevelSize(tmpXdgTopLevel, tmpXdgTopLevel->sizeX,
                             tmpXdgTopLevel->sizeY - (newPosY - currentPosY));
        }

        // Test collision
        if ((tmpPosY + tmpSizeY >= newPosY - 2 &&
             tmpPosY <= newPosY - 2 + newSizeY) ||
            (tmpPosY + tmpSizeY >= lastNewPosY - 2 &&
             tmpPosY <= lastNewPosY - 2 + lastNewSizeY)) {
          if (tmpPosY + tmpSizeY * 8 / 10 < newPosY + 5) {
            // Resize
            setXdgTopLevelSize(tmpXdgTopLevel, tmpXdgTopLevel->sizeX,
                               newPosY - tmpPosY);
          }
        }
      }
      lastDeltaY = deltaY;

    } else {
      // Resize bottom side
      newSizeY = startSizeY + deltaY;

      // Resize other surfaces
      struct coR_xdg_toplevel *tmpXdgTopLevel;
      wl_list_for_each(tmpXdgTopLevel, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpXdgTopLevel == resizingTopLevel)
          continue;

        // Variables
        int tmpPosY = tmpXdgTopLevel->posY;
        int tmpSizeY = tmpXdgTopLevel->sizeY;
        int lastNewSizeY = startSizeY + lastDeltaY;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosY + tmpSizeY - currentPosY - currentSizeY) < 20) {
          setXdgTopLevelSize(tmpXdgTopLevel, tmpXdgTopLevel->sizeX,
                             tmpXdgTopLevel->sizeY + newSizeY - currentSizeY);
        }

        // Test collision
        if ((tmpPosY + tmpSizeY >= newPosY && tmpPosY <= newPosY + newSizeY) ||
            (tmpPosY + tmpSizeY >= newPosY &&
             tmpPosY <= newPosY + lastNewSizeY)) {
          if (newPosY + newSizeY * 8 / 10 < tmpPosY + 5) {
            // Resize
            int tmpLastSizeY = tmpXdgTopLevel->sizeY;
            setXdgTopLevelSize(tmpXdgTopLevel, tmpXdgTopLevel->sizeX,
                               tmpPosY + tmpSizeY - newPosY - newSizeY);
            // Move surface
            setXdgTopLevelPos(tmpXdgTopLevel, tmpXdgTopLevel->posX,
                              tmpXdgTopLevel->posY + tmpLastSizeY -
                                  tmpXdgTopLevel->sizeY);
          }
        }
      }
      lastDeltaY = deltaY;
    }
  }
  // ----

  // Verif minimal size
  if (newSizeX <= resizingTopLevel->xdgTopLevel->current.min_width ||
      newSizeY <= resizingTopLevel->xdgTopLevel->current.min_height)
    return;

  setXdgTopLevelPos(resizingTopLevel, newPosX, newPosY);
  setXdgTopLevelSize(resizingTopLevel, newSizeX, newSizeY);
}

/*
  Resize all the surface in xdgTopLevelsList (on X axis) for take an free area
  determined by start....X/Y
  -> Return 0 for no changement
  -> Return 1 for minimum one surface have size changed
*/
int resizeXOnEmptyArea(int startPosX, int startPosY, int startSizeX,
                       int startSizeY, struct wl_list *xdgTopLevelsList) {
  // Resize on X axis
  int side = 0;
  int sizeChanged = 0;

  struct coR_xdg_toplevel *tmpXdgTopLevel;
  wl_list_for_each_reverse(tmpXdgTopLevel, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpXdgTopLevel->posX;
    int tmpPosY = tmpXdgTopLevel->posY;
    int tmpSizeX = tmpXdgTopLevel->sizeX;
    int tmpSizeY = tmpXdgTopLevel->sizeY;

    // Is resizable on the empty area
    if (!(tmpPosY >= startPosY) ||
        !(tmpPosY + tmpSizeY <= startPosY + startSizeY))
      continue;

    // Is side by side with it
    // Left side
    if (abs(tmpPosX + tmpSizeX - startPosX) < 5) {
      if (side == 2)
        continue;
      else if (side == 0)
        side = 1;
    }

    // Right side
    else if (abs(startPosX + startSizeX - tmpPosX) < 5) {
      if (side == 1)
        continue;
      else if (side == 0)
        side = 2;
    }

    // Not side by side
    else {
      continue;
    }

    // Resize and move
    if (startPosX < tmpPosX) {
      setXdgTopLevelPos(tmpXdgTopLevel, startPosX, tmpXdgTopLevel->posY);
    }

    setXdgTopLevelSize(tmpXdgTopLevel, tmpXdgTopLevel->sizeX + startSizeX,
                       tmpXdgTopLevel->sizeY);

    sizeChanged = 1;
  }
  return sizeChanged;
}

/*
  Resize all the surface in xdgTopLevelsList (on Y axis) for take an free area
  determined by start....X/Y
  -> Return 0 for no changement
  -> Return 1 for minimum one surface have size changed
*/
int resizeYOnEmptyArea(int startPosX, int startPosY, int startSizeX,
                       int startSizeY, struct wl_list *xdgTopLevelsList) {
  // Resize on Y axis
  int side = 0;
  int sizeChanged = 0;

  struct coR_xdg_toplevel *tmpXdgTopLevel;
  wl_list_for_each_reverse(tmpXdgTopLevel, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpXdgTopLevel->posX;
    int tmpPosY = tmpXdgTopLevel->posY;
    int tmpSizeX = tmpXdgTopLevel->sizeX;
    int tmpSizeY = tmpXdgTopLevel->sizeY;

    // Is resizable on the empty area
    if (!(tmpPosX >= startPosX) ||
        !(tmpPosX + tmpSizeX <= startPosX + startSizeX))
      continue;

    // Is side by side with it
    // Top side
    if (abs(tmpPosY + tmpSizeY - startPosY) < 5) {
      if (side == 2)
        continue;
      else if (side == 0)
        side = 1;
    }

    // Bottom side
    else if (abs(startPosY + startSizeY - tmpPosY) < 5) {
      if (side == 1)
        continue;
      else if (side == 0)
        side = 2;
    }

    // Not side by side
    else {
      continue;
    }

    // Resize and move
    if (startPosY < tmpPosY) {
      setXdgTopLevelPos(tmpXdgTopLevel, tmpXdgTopLevel->posX, startPosY);
    }

    ;
    setXdgTopLevelSize(tmpXdgTopLevel, tmpXdgTopLevel->sizeX,
                       tmpXdgTopLevel->sizeY + startSizeY);

    sizeChanged = 1;
  }

  return sizeChanged;
}
