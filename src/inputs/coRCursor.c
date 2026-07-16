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
  // printf("-> buttonCursor\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorButtonListener);
  struct wlr_pointer_button_event *event = data;

  // -> Active/Désactive -> le resize d'un toplevel avec click droit + SUPER
  if (event->button == 273) {
    if (event->state == WL_POINTER_BUTTON_STATE_PRESSED && superPressed &&
        coRState->focusedSurface) {
      resizingTopLevel = coRState->focusedSurface->data;
      startResizingCursorPosX = coRState->cursor->x;
      startResizingCursorPosY = coRState->cursor->y;
      startResizingPosX = resizingTopLevel->posX;
      startResizingPosY = resizingTopLevel->posY;
      struct coR_xdg_toplevel *focusedTopLevel = coRState->focusedSurface->data;
      startResizingWidth = focusedTopLevel->xdgTopLevel->current.width;
      startResizingHeight = focusedTopLevel->xdgTopLevel->current.height;

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
        coRState->focusedSurface) {
      movingTopLevel = coRState->focusedSurface->data;
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

        // TODO VERIF: resize all surface to take the place left
        // Resize on X axis
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
              wlr_scene_node_set_position(
                  &sceneTree->node, tmpXdgTopLevel->posX, tmpXdgTopLevel->posY);
            }

            tmpXdgTopLevel->sizeX += startSizeX;
            wlr_xdg_toplevel_set_size(tmpXdgTopLevel->xdgTopLevel,
                                      tmpXdgTopLevel->sizeX,
                                      tmpXdgTopLevel->sizeY);

            movingTopLevel = NULL;
          }
          if (movingTopLevel == NULL)
            break;
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
                                    tmpXdgTopLevel->sizeX,
                                    tmpXdgTopLevel->sizeY);

          movingTopLevel = NULL;
        }

        if (movingTopLevel == NULL)
          break;

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
                                    tmpXdgTopLevel->sizeX,
                                    tmpXdgTopLevel->sizeY);

          movingTopLevel = NULL;
        }
        if (movingTopLevel == NULL)
          break;
      }

      // Pas de surface trouvé -> On la remet à sa position initial
      if (movingTopLevel != NULL) {
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
  // printf("-> motionCursor\n");

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
        printf("%d\n", i);
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
  resizeTopLevel(resizingTopLevel, coRState, startResizingCursorPosX,
                 startResizingCursorPosY, startResizingWidth,
                 startResizingHeight, startResizingPosX, startResizingPosY);

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
  printf("-> axisCursor\n");

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
