#include "coRXdgTopLevel.h"

#include "../coRState.h"
#include "../inputs/coRCursor.h"
#include "../inputs/coRInputs.h"
#include "coRSurface.h"
#include "src/surfaces/coRLayerSurface.h"
#include "wlr/util/box.h"
#include <stdint.h>
#include <stdio.h>
#include <wayland-util.h>

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

  // // Take the entire screen (exept docks
  // surfaceSetPos(coRSurface, 0, 0);
  // surfaceSetSize(coRSurface, workspace->currentOutput->width,
  //                workspace->currentOutput->height);
  // wl_list_insert(&workspace->xdgTopLevels, &coRSurface->link);
}

static void mapXdgTopLevelHandler(struct wl_listener *listener, void *data) {
  printf("-> map XdgTopLevel\n");

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, mapListener);
  struct coR_state *coRState = coRXdgTopLevel->coRSurface.coRState;

  struct wlr_xdg_toplevel *xdgTopLevel = coRXdgTopLevel->coRSurface.surfaceAbs;
  struct wlr_surface *surface = xdgTopLevel->base->surface;
  inputsChangeSurfaceToFocus(coRState, surface, 0, 0);
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
  int startSizeX = coRSurface->sizeX;
  int startSizeY = coRSurface->sizeY;

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

/* Resize toplevel for cursor motion
startPosX & startPosY are surface pos
startCursorPosX & startCursorPosY are cursor pos
startSizeX and startSizeY are size of the resizingTopLevel at default
*/
int lastDeltaX = 0;
int lastDeltaY = 0;
void resizeTopLevel(struct coR_surface *resizingTopLevel,
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
  int currentSizeX = resizingTopLevel->sizeX;
  int currentPosX = resizingTopLevel->posX;
  int newSizeX = currentSizeX;
  int newPosX = startPosX;

  // Variables for axis Y
  int deltaY = (int)(coRState->cursor->y) - startCursorPosY;
  int currentSizeY = resizingTopLevel->sizeY;
  int currentPosY = resizingTopLevel->posY;
  int newSizeY = currentSizeY;
  int newPosY = startPosY;

  // get the side to resize
  float threshold = startPosX + startSizeX / 2.;
  int side = startCursorPosX -
                 coRState->workspaces[resizingTopLevel->onWorkspaceNum].posX <
             threshold;
  int possibleSides = 2;

  // No resize sides glued to output's border
  if (startPosX <= box.x) {
    side = 0;
    possibleSides--;
  }

  if (startPosX + startSizeX + 1 >= box.width + box.x) {
    side = 1;
    possibleSides--;
  }

  if (possibleSides > 0) {
    // Resize with the perfect side
    if (side) {
      // Resize left side
      newPosX = startPosX + deltaX;
      newSizeX = startSizeX - deltaX;

      // Verif if can resize all other surfaces
      struct coR_surface *tmpCoRSurface;
      wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpCoRSurface == resizingTopLevel)
          continue;

        // Variables
        int tmpPosX = tmpCoRSurface->posX;
        int tmpSizeX = tmpCoRSurface->sizeX;
        int lastNewSizeX = startSizeX - lastDeltaX;
        int lastNewPosX = startPosX + lastDeltaX;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosX - currentPosX) < 20) {
          int newTmpSize = tmpCoRSurface->sizeX - (newPosX - currentPosX);
          if (newTmpSize < 50) {
            stopResizingSurface();
            return;
          }
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX - 2 &&
             tmpPosX <= newPosX - 2 + newSizeX) ||
            (tmpPosX + tmpSizeX >= lastNewPosX - 2 &&
             tmpPosX <= lastNewPosX - 2 + lastNewSizeX)) {
          if (tmpPosX + tmpSizeX * 8 / 10 <= newPosX + 5) {
            // Resize
            int newTmpSize = newPosX - tmpPosX;
            if (newTmpSize < 50) {
              stopResizingSurface();
              return;
            }
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
        int tmpSizeX = tmpCoRSurface->sizeX;
        int lastNewSizeX = startSizeX - lastDeltaX;
        int lastNewPosX = startPosX + lastDeltaX;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosX - currentPosX) < 20) {
          surfaceSetPos(tmpCoRSurface,
                        tmpCoRSurface->posX + newPosX - currentPosX,
                        tmpCoRSurface->posY);
          surfaceSetSize(tmpCoRSurface,
                         tmpCoRSurface->sizeX - (newPosX - currentPosX),
                         tmpCoRSurface->sizeY);
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX - 2 &&
             tmpPosX <= newPosX - 2 + newSizeX) ||
            (tmpPosX + tmpSizeX >= lastNewPosX - 2 &&
             tmpPosX <= lastNewPosX - 2 + lastNewSizeX)) {
          if (tmpPosX + tmpSizeX * 8 / 10 < newPosX + 5) {
            // Resize
            surfaceSetSize(tmpCoRSurface, newPosX - tmpPosX,
                           tmpCoRSurface->sizeY);
          }
        }
      }
      lastDeltaX = deltaX;

    } else {
      // Resize right side
      newSizeX = startSizeX + deltaX;

      // Verif if can resize all other surfaces
      // C'est horrible, je devrais vraiment changer comment je gère mes fenêtre
      // car j'en est mare. Je ne le ferait pas pour l'axe Y
      struct coR_surface *tmpCoRSurface;
      wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpCoRSurface == resizingTopLevel)
          continue;

        // Variables
        int tmpPosX = tmpCoRSurface->posX;
        int tmpSizeX = tmpCoRSurface->sizeX;
        int lastNewSizeX = startSizeX + lastDeltaX;

        // Special case: surface on border with it -> Resize
        if (abs(tmpPosX + tmpSizeX - currentPosX - currentSizeX) < 20) {
          if (tmpCoRSurface->sizeX + newSizeX - currentSizeX < 33) {
            stopResizingSurface();
            return;
          }
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX && tmpPosX <= newPosX + newSizeX) ||
            (tmpPosX + tmpSizeX >= newPosX &&
             tmpPosX <= newPosX + lastNewSizeX)) {
          if (newPosX + newSizeX * 8 / 10 < tmpPosX + 5) {
            // Resize
            if (tmpPosX + tmpSizeX - newPosX - newSizeX < 33) {
              stopResizingSurface();
              return;
            }
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
        // int tmpSizeX = tmpXdgTopLevel->xdgTopLevel->current.width;
        int tmpSizeX = tmpCoRSurface->sizeX;
        int lastNewSizeX = startSizeX + lastDeltaX;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosX + tmpSizeX - currentPosX - currentSizeX) < 20) {
          surfaceSetSize(tmpCoRSurface,
                         tmpCoRSurface->sizeX + newSizeX - currentSizeX,
                         tmpCoRSurface->sizeY);
        }

        // Test colision
        if ((tmpPosX + tmpSizeX >= newPosX && tmpPosX <= newPosX + newSizeX) ||
            (tmpPosX + tmpSizeX >= newPosX &&
             tmpPosX <= newPosX + lastNewSizeX)) {
          if (newPosX + newSizeX * 8 / 10 < tmpPosX + 5) {
            // Resize
            int tmpLastSizeX = tmpCoRSurface->sizeX;
            surfaceSetSize(tmpCoRSurface,
                           tmpPosX + tmpSizeX - newPosX - newSizeX,
                           tmpCoRSurface->sizeY);
            // Move surface
            surfaceSetPos(tmpCoRSurface,
                          tmpCoRSurface->posX + tmpLastSizeX -
                              tmpCoRSurface->sizeX,
                          tmpCoRSurface->posY);
          }
        }
      }
      lastDeltaX = deltaX;
    }
  }
  // ----

  // -- resize in axis Y -- (Copy of axis X)
  // get the side to resize
  threshold = startPosY + startSizeY / 2.;
  side = startCursorPosY -
             coRState->workspaces[resizingTopLevel->onWorkspaceNum].posY <
         threshold;
  possibleSides = 2;

  // No resize sides glued to output's border
  if (startPosY <= box.y) {
    side = 0;
    possibleSides--;
  }

  if (startPosY + startSizeY + 1 >= box.height + box.y) {
    side = 1;
    possibleSides--;
  }

  if (possibleSides > 0) {
    // Resize with the perfect side
    if (side) {
      // Resize top side
      newPosY = startPosY + deltaY;
      newSizeY = startSizeY - deltaY;

      // Verif if can resize all other surfaces
      struct coR_surface *tmpCoRSurface;
      wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpCoRSurface == resizingTopLevel)
          continue;

        // Variables
        int tmpPosY = tmpCoRSurface->posY;
        int tmpSizeY = tmpCoRSurface->sizeY;
        int lastNewSizeY = startSizeY - lastDeltaY;
        int lastNewPosY = startPosY + lastDeltaY;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosY - currentPosY) < 20) {
          int newTmpSize = tmpCoRSurface->sizeY - (newPosY - currentPosY);
          if (newTmpSize < 50) {
            stopResizingSurface();
            return;
          }
        }

        // Test colision
        if ((tmpPosY + tmpSizeY >= newPosY - 2 &&
             tmpPosY <= newPosY - 2 + newSizeY) ||
            (tmpPosY + tmpSizeY >= lastNewPosY - 2 &&
             tmpPosY <= lastNewPosY - 2 + lastNewSizeY)) {
          if (tmpPosY + tmpSizeY * 8 / 10 <= newPosY + 5) {
            // Resize
            int newTmpSize = newPosY - tmpPosY;
            if (newTmpSize < 50) {
              stopResizingSurface();
              return;
            }
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
        int tmpSizeY = tmpCoRSurface->sizeY;
        int lastNewSizeY = startSizeY - lastDeltaY;
        int lastNewPosY = startPosY + lastDeltaY;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosY - currentPosY) < 20) {
          surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX,
                        tmpCoRSurface->posY + newPosY - currentPosY);
          surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                         tmpCoRSurface->sizeY - (newPosY - currentPosY));
        }

        // Test collision
        if ((tmpPosY + tmpSizeY >= newPosY - 2 &&
             tmpPosY <= newPosY - 2 + newSizeY) ||
            (tmpPosY + tmpSizeY >= lastNewPosY - 2 &&
             tmpPosY <= lastNewPosY - 2 + lastNewSizeY)) {
          if (tmpPosY + tmpSizeY * 8 / 10 < newPosY + 5) {
            // Resize
            surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                           newPosY - tmpPosY);
          }
        }
      }
      lastDeltaY = deltaY;

    } else {
      // Resize bottom side
      newSizeY = startSizeY + deltaY;

      // Verif if can resize all other surfaces
      struct coR_surface *tmpCoRSurface;
      wl_list_for_each(tmpCoRSurface, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpCoRSurface == resizingTopLevel)
          continue;

        // Variables
        int tmpPosY = tmpCoRSurface->posY;
        int tmpSizeY = tmpCoRSurface->sizeY;
        int lastNewSizeY = startSizeY + lastDeltaY;

        // Special case: surface on border with it -> Resize
        if (abs(tmpPosY + tmpSizeY - currentPosY - currentSizeY) < 20) {
          if (tmpCoRSurface->sizeY + newSizeY - currentSizeY < 33) {
            stopResizingSurface();
            return;
          }
        }

        // Test colision
        if ((tmpPosY + tmpSizeY >= newPosY && tmpPosY <= newPosY + newSizeY) ||
            (tmpPosY + tmpSizeY >= newPosY &&
             tmpPosY <= newPosY + lastNewSizeY)) {
          if (newPosY + newSizeY * 8 / 10 < tmpPosY + 5) {
            // Resize
            if (tmpPosY + tmpSizeY - newPosY - newSizeY < 33) {
              stopResizingSurface();
              return;
            }
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
        int tmpSizeY = tmpCoRSurface->sizeY;
        int lastNewSizeY = startSizeY + lastDeltaY;

        // Special case: surface below resizingTopLevel
        if (abs(tmpPosY + tmpSizeY - currentPosY - currentSizeY) < 20) {
          surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                         tmpCoRSurface->sizeY + newSizeY - currentSizeY);
        }

        // Test collision
        if ((tmpPosY + tmpSizeY >= newPosY && tmpPosY <= newPosY + newSizeY) ||
            (tmpPosY + tmpSizeY >= newPosY &&
             tmpPosY <= newPosY + lastNewSizeY)) {
          if (newPosY + newSizeY * 8 / 10 < tmpPosY + 5) {
            // Resize
            int tmpLastSizeY = tmpCoRSurface->sizeY;
            surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                           tmpPosY + tmpSizeY - newPosY - newSizeY);
            // Move surface
            surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX,
                          tmpCoRSurface->posY + tmpLastSizeY -
                              tmpCoRSurface->sizeY);
          }
        }
      }
      lastDeltaY = deltaY;
    }
  }
  // ----

  // Verif minimal size
  if (newSizeX <= 22 || newSizeY <= 22)
    return;

  // No resize sides glued to output's border
  if (startPosX > box.x) {
    if (newPosX <= box.x) {
      return;
    }
  }

  if (startPosX + startSizeX < box.width + box.x) {
    if (newPosX + newSizeX >= box.width + box.x) {
      return;
    }
  }

  if (startPosY > box.y) {
    if (newPosY <= box.y) {
      return;
    }
  }

  if (startPosY + startSizeY < box.height + box.y) {
    if (newPosY + newSizeY >= box.height + box.y) {
      return;
    }
  }

  surfaceSetPos(resizingTopLevel, newPosX, newPosY);
  surfaceSetSize(resizingTopLevel, newSizeX, newSizeY);
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

  struct coR_surface *tmpCoRSurface;
  wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpCoRSurface->posX;
    int tmpPosY = tmpCoRSurface->posY;
    int tmpSizeX = tmpCoRSurface->sizeX;
    int tmpSizeY = tmpCoRSurface->sizeY;

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
      surfaceSetPos(tmpCoRSurface, startPosX, tmpCoRSurface->posY);
    }

    surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX + startSizeX,
                   tmpCoRSurface->sizeY);

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

  struct coR_surface *tmpCoRSurface;
  wl_list_for_each_reverse(tmpCoRSurface, xdgTopLevelsList, link) {
    // Variables
    int tmpPosX = tmpCoRSurface->posX;
    int tmpPosY = tmpCoRSurface->posY;
    int tmpSizeX = tmpCoRSurface->sizeX;
    int tmpSizeY = tmpCoRSurface->sizeY;

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
      surfaceSetPos(tmpCoRSurface, tmpCoRSurface->posX, startPosY);
    }

    ;
    surfaceSetSize(tmpCoRSurface, tmpCoRSurface->sizeX,
                   tmpCoRSurface->sizeY + startSizeY);

    sizeChanged = 1;
  }

  return sizeChanged;
}
