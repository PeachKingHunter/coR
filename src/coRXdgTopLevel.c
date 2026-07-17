#include "coRXdgTopLevel.h"
#include "coRState.h"
#include "inputs/coRCursor.h"
#include "inputs/coRInputs.h"
#include <stddef.h>
#include <stdio.h>
#include <wayland-util.h>

static void commitXdgTopLevelHandler(struct wl_listener *listener, void *data) {
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
        wl_container_of(xdgTopLevelsList, xdgTopLevel, link);
    focusedSurface = xdgTopLevel->xdgTopLevel->base->surface;
    splitXdgTopLevel(xdgTopLevel, coRXdgTopLevel);
    wl_list_insert(&workspace->xdgTopLevels, &coRXdgTopLevel->link);
    return;
  }

  printf("start P4\n");
  wl_list_insert(&workspace->xdgTopLevels, &coRXdgTopLevel->link);
  struct wlr_scene_tree *topLevelSceneTreeNew =
      coRXdgTopLevel->xdgTopLevel->base->data;
  coRXdgTopLevel->posX = 0;
  coRXdgTopLevel->posY = 0;
  coRXdgTopLevel->sizeX = workspace->currentOutput->width;
  coRXdgTopLevel->sizeY = workspace->currentOutput->height;
  wlr_scene_node_set_position(&topLevelSceneTreeNew->node, coRXdgTopLevel->posX,
                              coRXdgTopLevel->posY);
  wlr_xdg_toplevel_set_size(coRXdgTopLevel->xdgTopLevel, coRXdgTopLevel->sizeX,
                            coRXdgTopLevel->sizeY);
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

  // Resize on X axis
  struct coR_xdg_toplevel *tmpXdgTopLevel;
  if (startSizeX < startSizeY) {
    int side = 0;
    wl_list_for_each_reverse(tmpXdgTopLevel, lastXdgTopLevelsList, link) {
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
        struct wlr_scene_tree *sceneTree =
            tmpXdgTopLevel->xdgTopLevel->base->data;
        tmpXdgTopLevel->posX = startPosX;
        wlr_scene_node_set_position(&sceneTree->node, tmpXdgTopLevel->posX,
                                    tmpXdgTopLevel->posY);
      }

      tmpXdgTopLevel->sizeX += startSizeX;
      wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                tmpXdgTopLevel->sizeX, tmpXdgTopLevel->sizeY);

      movingTopLevel = NULL;
    }
    if (movingTopLevel == NULL)
      goto endResizeAllSurfaces;
  }

  // Resize on Y axis
  int side = 0;
  wl_list_for_each_reverse(tmpXdgTopLevel, lastXdgTopLevelsList, link) {
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
      struct wlr_scene_tree *sceneTree =
          tmpXdgTopLevel->xdgTopLevel->base->data;
      tmpXdgTopLevel->posY = startPosY;
      wlr_scene_node_set_position(&sceneTree->node, tmpXdgTopLevel->posX,
                                  tmpXdgTopLevel->posY);
    }

    tmpXdgTopLevel->sizeY += startSizeY;
    wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                              tmpXdgTopLevel->sizeX, tmpXdgTopLevel->sizeY);

    movingTopLevel = NULL;
  }

  if (movingTopLevel == NULL)
    goto endResizeAllSurfaces;

  // Resize on X axis
  side = 0;
  wl_list_for_each_reverse(tmpXdgTopLevel, lastXdgTopLevelsList, link) {
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
      struct wlr_scene_tree *sceneTree =
          tmpXdgTopLevel->xdgTopLevel->base->data;
      tmpXdgTopLevel->posX = startPosX;
      wlr_scene_node_set_position(&sceneTree->node, tmpXdgTopLevel->posX,
                                  tmpXdgTopLevel->posY);
    }

    tmpXdgTopLevel->sizeX += startSizeX;
    wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                              tmpXdgTopLevel->sizeX, tmpXdgTopLevel->sizeY);

    movingTopLevel = NULL;
  }
  if (movingTopLevel == NULL)
    goto endResizeAllSurfaces;

endResizeAllSurfaces:
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

/* The lifecycle of an XDG surface follows these main states:
  1.Created - Surface is created but not yet configured
  2.Configured - Compositor has sent a configure event which the client
  acknowledged 3.Mapped - Surface has a buffer attached and is visible
  4.Unmapped - Surface no longer has a buffer and should not be rendered
  5.Destroyed - Surface is being freed
*/

int splitXdgTopLevel(struct coR_xdg_toplevel *toSplit,
                     struct coR_xdg_toplevel *newXdgTopLevel) {
  printf("Enter split func\n");
  if (toSplit == NULL || newXdgTopLevel == NULL)
    return 0;

  // Variables
  struct wlr_scene_tree *topLevelSceneTreeToSplit =
      toSplit->xdgTopLevel->base->data;
  struct wlr_scene_tree *topLevelSceneTreeNew =
      newXdgTopLevel->xdgTopLevel->base->data;

  int posX = topLevelSceneTreeToSplit->node.x;
  int posY = topLevelSceneTreeToSplit->node.y;

  int width = toSplit->xdgTopLevel->current.width;
  int height = toSplit->xdgTopLevel->current.height;

  // Devient frère à celui découpé
  wlr_scene_node_reparent(&topLevelSceneTreeNew->node,
                          topLevelSceneTreeToSplit->node.parent);

  // Minimal size
  if (width <= 2 && height <= 2) {
    printf("Too small\n");
    return 0;
  }

  if (width > height) {

    // Ajout à droite ou gauche
    // Position & size de la nouvelle surface
    wlr_scene_node_set_position(&topLevelSceneTreeNew->node, posX + width / 2.,
                                posY);
    newXdgTopLevel->posX = toSplit->posX + width / 2.;
    newXdgTopLevel->posY = toSplit->posY;
    newXdgTopLevel->sizeX = width / 2.;
    newXdgTopLevel->sizeY = height;
    wlr_xdg_toplevel_set_size(newXdgTopLevel->xdgTopLevel,
                              newXdgTopLevel->sizeX, newXdgTopLevel->sizeY);

    // Resize the parent surface
    toSplit->sizeX = width / 2.;
    toSplit->sizeY = height;
    wlr_xdg_toplevel_set_size(toSplit->xdgTopLevel, toSplit->sizeX,
                              toSplit->sizeY);

  } else {
    // Ajout en bas ou en haut
    // Position & size de la nouvelle surface
    wlr_scene_node_set_position(&topLevelSceneTreeNew->node, posX,
                                posY + height / 2.);
    newXdgTopLevel->posX = toSplit->posX;
    newXdgTopLevel->posY = toSplit->posY + height / 2.;
    newXdgTopLevel->sizeX = width;
    newXdgTopLevel->sizeY = height / 2.;
    wlr_xdg_toplevel_set_size(newXdgTopLevel->xdgTopLevel,
                              newXdgTopLevel->sizeX, newXdgTopLevel->sizeY);

    // Resize the parent surface
    toSplit->sizeX = width;
    toSplit->sizeY = height / 2.;
    wlr_xdg_toplevel_set_size(toSplit->xdgTopLevel, toSplit->sizeX,
                              toSplit->sizeY);
  }
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
          tmpXdgTopLevel->posX += newPosX - currentPosX;
          wlr_scene_node_set_position(tmpXdgTopLevel->xdgTopLevel->base->data,
                                      tmpXdgTopLevel->posX,
                                      tmpXdgTopLevel->posY);
          tmpXdgTopLevel->sizeX -= newPosX - currentPosX;
          wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                    tmpXdgTopLevel->sizeX,
                                    tmpXdgTopLevel->sizeY);
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX - 2 &&
             tmpPosX <= newPosX - 2 + newSizeX) ||
            (tmpPosX + tmpSizeX >= lastNewPosX - 2 &&
             tmpPosX <= lastNewPosX - 2 + lastNewSizeX)) {
          if (tmpPosX + tmpSizeX * 8 / 10 < newPosX + 5) {
            // Resize
            tmpXdgTopLevel->sizeX = newPosX - tmpPosX;
            wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                      tmpXdgTopLevel->sizeX,
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
          tmpXdgTopLevel->sizeX += newSizeX - currentSizeX;
          wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                    tmpXdgTopLevel->sizeX,
                                    tmpXdgTopLevel->sizeY);
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX && tmpPosX <= newPosX + newSizeX) ||
            (tmpPosX + tmpSizeX >= newPosX &&
             tmpPosX <= newPosX + lastNewSizeX)) {
          if (newPosX + newSizeX * 8 / 10 < tmpPosX + 5) {
            // Resize
            int tmpLastSizeX = tmpXdgTopLevel->sizeX;
            tmpXdgTopLevel->sizeX = tmpPosX + tmpSizeX - newPosX - newSizeX;
            wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                      tmpXdgTopLevel->sizeX,
                                      tmpXdgTopLevel->sizeY);
            // TODO: Move surface
            tmpXdgTopLevel->posX += tmpLastSizeX - tmpXdgTopLevel->sizeX;
            wlr_scene_node_set_position(tmpXdgTopLevel->xdgTopLevel->base->data,
                                        tmpXdgTopLevel->posX,
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
          tmpXdgTopLevel->posY += newPosY - currentPosY;
          wlr_scene_node_set_position(tmpXdgTopLevel->xdgTopLevel->base->data,
                                      tmpXdgTopLevel->posX,
                                      tmpXdgTopLevel->posY);
          tmpXdgTopLevel->sizeY -= newPosY - currentPosY;
          wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                    tmpXdgTopLevel->sizeX,
                                    tmpXdgTopLevel->sizeY);
        }

        // Test collision
        if ((tmpPosY + tmpSizeY >= newPosY - 2 &&
             tmpPosY <= newPosY - 2 + newSizeY) ||
            (tmpPosY + tmpSizeY >= lastNewPosY - 2 &&
             tmpPosY <= lastNewPosY - 2 + lastNewSizeY)) {
          if (tmpPosY + tmpSizeY * 8 / 10 < newPosY + 5) {
            // Resize
            tmpXdgTopLevel->sizeY = newPosY - tmpPosY;
            wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                      tmpXdgTopLevel->sizeX,
                                      tmpXdgTopLevel->sizeY);
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
          tmpXdgTopLevel->sizeY += newSizeY - currentSizeY;
          wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                    tmpXdgTopLevel->sizeX,
                                    tmpXdgTopLevel->sizeY);
        }

        // Test collision
        if ((tmpPosY + tmpSizeY >= newPosY && tmpPosY <= newPosY + newSizeY) ||
            (tmpPosY + tmpSizeY >= newPosY &&
             tmpPosY <= newPosY + lastNewSizeY)) {
          if (newPosY + newSizeY * 8 / 10 < tmpPosY + 5) {
            // Resize
            int tmpLastSizeY = tmpXdgTopLevel->sizeY;
            tmpXdgTopLevel->sizeY = tmpPosY + tmpSizeY - newPosY - newSizeY;
            wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                      tmpXdgTopLevel->sizeX,
                                      tmpXdgTopLevel->sizeY);
            // Move surface
            tmpXdgTopLevel->posY += tmpLastSizeY - tmpXdgTopLevel->sizeY;
            wlr_scene_node_set_position(tmpXdgTopLevel->xdgTopLevel->base->data,
                                        tmpXdgTopLevel->posX,
                                        tmpXdgTopLevel->posY);
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

  resizingTopLevel->posX = newPosX;
  resizingTopLevel->posY = newPosY;
  wlr_scene_node_set_position(resizingTopLevel->xdgTopLevel->base->data,
                              resizingTopLevel->posX, resizingTopLevel->posY);
  resizingTopLevel->sizeX = newSizeX;
  resizingTopLevel->sizeY = newSizeY;
  wlr_xdg_toplevel_set_size(resizingTopLevel->xdgTopLevel, newSizeX, newSizeY);
}
