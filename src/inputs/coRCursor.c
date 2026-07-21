#include "./coRCursor.h"
#include "../coRXdgTopLevel.h"
#include "coRInputs.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>

// Global variables
struct coR_xdg_toplevel *resizingTopLevel = NULL;
int startResizingCursorPosX, startResizingCursorPosY;
int startResizingPosX, startResizingPosY;
int startResizingWidth, startResizingHeight;
extern int superPressed;
extern int lastDeltaX;

struct coR_xdg_toplevel *movingTopLevel = NULL;
int startMovingPosX, startMovingPosY;

extern int lastDeltaY;

// ---- ## Curseur ## ---- //
void cursorButtonHandler(struct wl_listener *listener, void *data) {
  // printf("-> cursorButtonHandler\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorButtonListener);
  struct wlr_pointer_button_event *event = data;

  // -> Active/Désactive -> le resize d'un toplevel avec click droit + SUPER
  if (event->button == 273) {
    if (event->state == WL_POINTER_BUTTON_STATE_PRESSED && superPressed &&
        coRState->focusedCoRXdgToplevel) {
      resizingTopLevel = coRState->focusedCoRXdgToplevel;
      startResizingCursorPosX = coRState->cursor->x;
      startResizingCursorPosY = coRState->cursor->y;
      startResizingPosX = resizingTopLevel->posX;
      startResizingPosY = resizingTopLevel->posY;
      startResizingWidth = resizingTopLevel->xdgTopLevel->current.width;
      startResizingHeight = resizingTopLevel->xdgTopLevel->current.height;

      lastDeltaX = 0;
      lastDeltaY = 0;

    } else if (event->state == WL_POINTER_BUTTON_STATE_RELEASED &&
               resizingTopLevel != NULL) {
      resizingTopLevel = NULL;
    }
  }

  // -> Active/Désactive -> le déplacement d'un toplevel avec click left + SUPER
  else if (event->button == 272) {
    if (event->state == WL_POINTER_BUTTON_STATE_PRESSED && superPressed &&
        coRState->focusedCoRXdgToplevel) {
      movingTopLevel = coRState->focusedCoRXdgToplevel;
      startMovingPosX = coRState->cursor->x;
      startMovingPosY = coRState->cursor->y;
      return;

    } else if (event->state == WL_POINTER_BUTTON_STATE_RELEASED &&
               movingTopLevel != NULL) {
      // Variables
      int startPosX = movingTopLevel->posX;
      int startPosY = movingTopLevel->posY;
      int startSizeX = movingTopLevel->sizeX;
      int startSizeY = movingTopLevel->sizeY;

      // Change toplevel posX & posY
      int deltaX = coRState->cursor->x - startMovingPosX;
      int deltaY = coRState->cursor->y - startMovingPosY;
      movingTopLevel->posX += deltaX;
      movingTopLevel->posY += deltaY;

      struct coR_workspace *workspace =
          coRState->workspaces + coRState->focusedWorkspaceNum;
      struct wl_list *xdgTopLevelsList = &workspace->xdgTopLevels;

      struct coR_workspace *lastWorkspace =
          coRState->workspaces + movingTopLevel->onWorkspaceNum;
      struct wl_list *lastXdgTopLevelsList = &lastWorkspace->xdgTopLevels;

      // Search an surface below the cursor
      struct coR_xdg_toplevel *tmpXdgTopLevel;
      wl_list_for_each(tmpXdgTopLevel, xdgTopLevelsList, link) {
        // Skip himself
        if (tmpXdgTopLevel == movingTopLevel)
          continue;

        // If is bellow the cursor
        int posX = tmpXdgTopLevel->posX;
        int sizeX = tmpXdgTopLevel->xdgTopLevel->current.width;

        int posY = tmpXdgTopLevel->posY;
        int sizeY = tmpXdgTopLevel->xdgTopLevel->current.height;

        double cursorPosXInWorkspace =
            coRState->cursor->x -
            coRState->workspaces[coRState->focusedWorkspaceNum].posX;
        double cursorPosYInWorkspace =
            coRState->cursor->y -
            coRState->workspaces[coRState->focusedWorkspaceNum].posY;

        // Test colisions curseur dans surface
        if (cursorPosXInWorkspace < posX ||
            cursorPosXInWorkspace > posX + sizeX ||
            cursorPosYInWorkspace < posY ||
            cursorPosYInWorkspace > posY + sizeY)
          continue;

        // Split the surface in two
        splitXdgTopLevel(tmpXdgTopLevel, movingTopLevel);
        // Place on same workspace
        if (tmpXdgTopLevel->onWorkspaceNum != movingTopLevel->onWorkspaceNum) {
          movingTopLevel->onWorkspaceNum = tmpXdgTopLevel->onWorkspaceNum;
          wl_list_remove(&movingTopLevel->link);
          wl_list_insert(&workspace->xdgTopLevels, &movingTopLevel->link);
        }

        // Resize all surface to take the place left | TODO -> VERIF:
        if (startSizeX < startSizeY) {
          // Resize on X axis
          if (resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                                 lastXdgTopLevelsList)) {
            movingTopLevel = NULL;
            break;
          }
        }

        // Resize on Y axis
        if (resizeYOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                               lastXdgTopLevelsList)) {
          movingTopLevel = NULL;
          break;
        }

        // Resize on X axis
        resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                           lastXdgTopLevelsList);
        movingTopLevel = NULL;
        break;
      }

      // Workspace vide -> on y met notre surface
      xdgTopLevelsList = &workspace->xdgTopLevels;
      if (wl_list_empty(xdgTopLevelsList)) {
        wl_list_remove(&movingTopLevel->link);
        wl_list_insert(&workspace->xdgTopLevels, &movingTopLevel->link);
        setXdgTopLevelPos(movingTopLevel, 0, 0);
        setXdgTopLevelSize(movingTopLevel, workspace->currentOutput->width,
                           workspace->currentOutput->height);
        struct wlr_scene_tree *sceneTree =
            movingTopLevel->xdgTopLevel->base->data;
        wlr_scene_node_reparent(&sceneTree->node, workspace->rootNode);
        movingTopLevel->onWorkspaceNum = coRState->focusedWorkspaceNum;

        // Resize all surface to take the place left | TODO -> VERIF:
        if (startSizeX < startSizeY) {
          // Resize on X axis
          if (resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                                 lastXdgTopLevelsList)) {
            movingTopLevel = NULL;
          }
        }

        // Resize on Y axis
        if (movingTopLevel != NULL) {
          if (resizeYOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                                 lastXdgTopLevelsList)) {
            movingTopLevel = NULL;
          }
        }

        // Resize on X axis
        if (movingTopLevel != NULL) {
          resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                             lastXdgTopLevelsList);
        }
        movingTopLevel = NULL;

      }

      // Pas de surface trouvé -> On la remet à sa position initial
      else if (movingTopLevel != NULL) {
        struct wlr_scene_tree *sceneTree =
            movingTopLevel->xdgTopLevel->base->data;
        int deltaX = coRState->cursor->x - startMovingPosX;
        int deltaY = coRState->cursor->y - startMovingPosY;
        movingTopLevel->posX -= deltaX;
        movingTopLevel->posY -= deltaY;
        wlr_scene_node_set_position(&sceneTree->node, movingTopLevel->posX,
                                    movingTopLevel->posY);
        movingTopLevel = NULL;
      }
    }
  }

  // Envoie au client le clique
  wlr_seat_pointer_notify_button(coRState->seat, event->time_msec,
                                 event->button, event->state);
  wlr_seat_pointer_notify_frame(coRState->seat);
}

void cursorMotionHandler(struct wl_listener *listener, void *data) {
  // printf("-> cursorMotionHandler\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorMotionListener);
  struct wlr_pointer_motion_event *event = data;
  double posX = coRState->cursor->x;
  double posY = coRState->cursor->y;

  // Move Cursor
  wlr_cursor_move(coRState->cursor, &event->pointer->base, event->delta_x,
                  event->delta_y);
  wlr_scene_node_set_position(&coRState->cursorScene->node, posX, posY);

  // -> Get the focused workspace
  for (int i = 0; i < NB_WORKSPACE; i++) {
    struct coR_workspace *currentWorkspace = coRState->workspaces + i;
    if (currentWorkspace->currentOutput != NULL) {
      if (currentWorkspace->posX <= posX &&
          posX <=
              currentWorkspace->posX + currentWorkspace->currentOutput->width &&
          currentWorkspace->posY <= posY &&
          posY <= currentWorkspace->posY +
                      currentWorkspace->currentOutput->height) {
        coRState->focusedWorkspaceNum = i;
        break;
      }
    }
  }

  // -> Change le focus si souris sur surface
  double sX, sY;
  struct wlr_surface *surface = getSurfaceBelowCursor(coRState, &sX, &sY);
  if (surface) {
    // Si le focus n'est pas dessus
    if (coRState->focusedSurface != surface)
      inputsChangeSurfaceToFocus(coRState, surface, sX, sY);

    // Bouge le pointeur physique dedans
    wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX, sY);
  }

  // -> le resize d'un toplevel avec click droit + SUPER
  if (resizingTopLevel != NULL) {
    resizeTopLevel(resizingTopLevel, coRState, startResizingCursorPosX,
                   startResizingCursorPosY, startResizingWidth,
                   startResizingHeight, startResizingPosX, startResizingPosY);
  }

  // -> le déplacement d'un toplevel avec click left + SUPER
  if (movingTopLevel != NULL) {
    int deltaX = coRState->cursor->x - startMovingPosX;
    int deltaY = coRState->cursor->y - startMovingPosY;

    struct wlr_scene_tree *sceneTree = movingTopLevel->xdgTopLevel->base->data;
    wlr_scene_node_set_position(&sceneTree->node, movingTopLevel->posX + deltaX,
                                movingTopLevel->posY + deltaY);
  }

  // Envoie au client
  wlr_seat_pointer_notify_frame(coRState->seat);
}

void cursorMotionAbsoluteHandler(struct wl_listener *listener, void *data) {
  // printf("-> cursorMotionAbsoluteHandler\n");
  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorMotionAbsoluteListener);
  struct wlr_pointer_motion_absolute_event *event = data;
  double posX = coRState->cursor->x;
  double posY = coRState->cursor->y;

  // Move Cursor
  wlr_cursor_warp_absolute(coRState->cursor, &event->pointer->base, event->x,
                           event->y);
  wlr_scene_node_set_position(&coRState->cursorScene->node, posX, posY);

  // TODO: comme l'autre fonction: cursorMotion

  // Envoie au client
  wlr_seat_pointer_notify_frame(coRState->seat);
}

void cursorAxisHandler(struct wl_listener *listener, void *data) {
  // printf("-> cursorAxisHandler\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorAxisListener);
  struct wlr_pointer_axis_event *event = data;

  // Envoie au client
  wlr_seat_pointer_notify_axis(
      coRState->seat, event->time_msec, event->orientation, event->delta,
      event->delta_discrete, event->source, event->relative_direction);
  wlr_seat_pointer_notify_frame(coRState->seat);
}

struct wlr_surface *getSurfaceBelowCursor(struct coR_state *coRState,
                                          double *sX, double *sY) {
  // printf("-> getSurfaceBelowCursor\n");
  // Variables
  double posX = coRState->cursor->x;
  double posY = coRState->cursor->y;

  // Get node
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, false);
  struct wlr_scene_node *node =
      wlr_scene_node_at(&coRState->scene->tree.node, posX, posY, sX, sY);
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, true);

  // Si aucune surface sous le curseur
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
    return NULL;

  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);

  // Si pas réussi à l'obtenir
  if (!scene_surface)
    return NULL;

  return scene_surface->surface;
}
