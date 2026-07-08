#include "./coRCursor.h"
#include <wlr/types/wlr_cursor.h>
#include "../coRXdgTopLevel.h"
#include "coRInputs.h"

// Global variables
struct coR_xdg_toplevel *resizingTopLevel = NULL;
int startResizingPosX, startResizingPosY;
int startResizingWidth, startResizingHeight;

struct coR_xdg_toplevel *movingTopLevel = NULL;
int startMovingPosX, startMovingPosY;

extern int superPressed;

// ---- ## Curseur ## ---- //
void cursorButtonHandler(struct wl_listener *listener, void *data) {
  // printf("-> buttonCursor\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorButtonListener);
  struct wlr_pointer_button_event *event = data;

  // Active/Désactive le resize d'un toplevel avec click droit + SUPER
  if (superPressed && event->button == 273) {
    if (event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
      resizingTopLevel = coRState->focusedSurface->data;
      startResizingPosX = coRState->cursor->x;
      startResizingPosY = coRState->cursor->y;
      struct coR_xdg_toplevel *focusedTopLevel = coRState->focusedSurface->data;
      startResizingWidth = focusedTopLevel->xdgTopLevel->current.width;
      startResizingHeight = focusedTopLevel->xdgTopLevel->current.height;

    } else if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
      // create new free Area (1 or 2 which the resize)
      // Variables
      int endResizingPosX = coRState->cursor->x;
      int endResizingPosY = coRState->cursor->y;

      int deltaX = startResizingPosX - endResizingPosX;
      int deltaY = startResizingPosY - endResizingPosY;

      int posX = resizingTopLevel->posX;
      int posY = resizingTopLevel->posY;
      int sizeX = resizingTopLevel->xdgTopLevel->current.width;
      int sizeY = resizingTopLevel->xdgTopLevel->current.height;

      // Create right Free Area
      if (deltaX > 0) {
        struct free_area *freeArea = malloc(sizeof(struct free_area));
        if (freeArea == NULL)
          exit(1);
        freeArea->posX = posX + sizeX;
        freeArea->posY = posY;
        freeArea->sizeX = deltaX;
        freeArea->sizeY = sizeY;
        if (freeArea->sizeX > 0 && freeArea->sizeY > 0) {
          wl_list_insert(&coRState->freeAreas, &freeArea->link);
        } else {
          free(freeArea);
        }
      }

      // Create down Free Area
      if (deltaY > 0) {
        struct free_area *freeArea = malloc(sizeof(struct free_area));
        if (freeArea == NULL)
          exit(1);
        freeArea->posX = posX;
        freeArea->posY = posY + sizeY;
        freeArea->sizeX = sizeX;
        freeArea->sizeY = deltaY;
        if (freeArea->sizeX > 0 && freeArea->sizeY > 0) {
          wl_list_insert(&coRState->freeAreas, &freeArea->link);
        } else {
          free(freeArea);
        }
      }

      // Create down right Free Area
      if (deltaY > 0) {
        struct free_area *freeArea = malloc(sizeof(struct free_area));
        if (freeArea == NULL)
          exit(1);
        freeArea->posX = posX + sizeX;
        freeArea->posY = posY + sizeY;
        freeArea->sizeX = deltaX;
        freeArea->sizeY = deltaY;
        if (freeArea->sizeX > 0 && freeArea->sizeY > 0) {
          wl_list_insert(&coRState->freeAreas, &freeArea->link);
        } else {
          free(freeArea);
        }
      }
      resizingTopLevel = NULL;
      return;
    }
  }

  // Active/Désactive le déplacement d'un toplevel avec click left + SUPER
  if (superPressed && event->button == 272) {
    if (event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
      movingTopLevel = coRState->focusedSurface->data;
      startMovingPosX = coRState->cursor->x;
      startMovingPosY = coRState->cursor->y;

    } else if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
      // TODO: Split existant surface
      // or take free Area depending on what is underneath

      double cursorPosX = coRState->cursor->x;
      double cursorPosY = coRState->cursor->y;

      // TODO: change toplevel posX & posY: Version temp
      int deltaX = coRState->cursor->x - startMovingPosX;
      int deltaY = coRState->cursor->y - startMovingPosY;
      movingTopLevel->posX += deltaX;
      movingTopLevel->posY += deltaY;

      struct coR_xdg_toplevel *tmpXdgTopLevel;
      wl_list_for_each(tmpXdgTopLevel, &coRState->xdgTopLevels, link) {
        // Skip himself
        if (tmpXdgTopLevel == movingTopLevel)
          continue;

        // If is bellow the cursor (TODO free_area / an other surface)
        int posX = tmpXdgTopLevel->posX;
        int sizeX = tmpXdgTopLevel->xdgTopLevel->current.width;

        int posY = tmpXdgTopLevel->posY;
        int sizeY = tmpXdgTopLevel->xdgTopLevel->current.height;

        // Test colisions curseur dans surface
        if (cursorPosX < posX || cursorPosX > posX + sizeX ||
            cursorPosY < posY || cursorPosY > posY + sizeY)
          continue;

        // Split the surface in two
        splitXdgTopLevel(tmpXdgTopLevel, movingTopLevel);

        // TODO: resize all surface to take the place
        // Add Free Area else

        movingTopLevel = NULL;
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

  resizeTopLevel(resizingTopLevel, coRState, startResizingPosX,
                 startResizingPosY, startResizingWidth, startResizingHeight);

  // move toplevel with cursor motion
  if (movingTopLevel != NULL) {
    int deltaX = coRState->cursor->x - startMovingPosX;
    int deltaY = coRState->cursor->y - startMovingPosY;

    struct wlr_scene_tree *sceneTree = movingTopLevel->xdgTopLevel->base->data;
    wlr_scene_node_set_position(&sceneTree->node, movingTopLevel->posX + deltaX,
                                movingTopLevel->posY + deltaY);
  }

  // Detecte la surface en dessous du curseur
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, false);
  double sX, sY;
  struct wlr_scene_node *node =
      wlr_scene_node_at(&coRState->scene->tree.node, posX, posY, &sX, &sY);
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, true);

  // Si aucune surface sous le curseur
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
    return;

  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);

  // Si pas réussi à l'obtenir
  if (!scene_surface)
    return;

  struct wlr_surface *surface = scene_surface->surface;
  if (surface) {
    // Si le focus n'est pas dessus
    if (coRState->focusedSurface != surface)
      inputsChangeSurfaceToFocus(coRState, surface, sX, sY);

    // Bouge le pointeur physique dedans
    wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX, sY);
  }

  // Retire le focus si il n'y à pas de surface à focus
  if (!coRState->focusedSurface) {
    wlr_seat_pointer_clear_focus(coRState->seat);
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

  // Detecte la surface en dessous du curseur
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, false);
  double sX, sY;
  struct wlr_scene_node *node =
      wlr_scene_node_at(&coRState->scene->tree.node, posX, posY, &sX, &sY);
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, true);

  // Si aucune surface sous le curseur
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
    return;

  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);

  // Si pas réussi à l'obtenir
  if (!scene_surface)
    return;

  struct wlr_surface *surface = scene_surface->surface;
  if (surface) {
    // Si le focus n'est pas dessus
    if (coRState->focusedSurface != surface)
      inputsChangeSurfaceToFocus(coRState, surface, sX, sY);

    // Bouge le pointeur physique dedans
    wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX, sY);
  }

  // Retire le focus si il n'y à pas de surface à focus
  if (!coRState->focusedSurface) {
    wlr_seat_pointer_clear_focus(coRState->seat);
  }

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


