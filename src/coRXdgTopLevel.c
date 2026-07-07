#include "coRXdgTopLevel.h"
#include "coRState.h"
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
  struct wlr_scene_tree *topLevelSceneTree =
      coRXdgTopLevel->xdgTopLevel->base->data;
  struct wlr_surface *focusedSurface = coRXdgTopLevel->coRState->focusedSurface;

  // Si il y a des emplacement vide, en prendre un
  if (!wl_list_empty(&coRXdgTopLevel->coRState->freeAreas)) {
    // Recup le première emplacement libre
    struct wl_list *elem = coRXdgTopLevel->coRState->freeAreas.next;
    struct free_area *freeArea = wl_container_of(elem, freeArea, link);
    wl_list_remove(elem);

    printf("size X: %d\n", freeArea->sizeX);
    printf("size Y: %d\n", freeArea->sizeY);
    printf("pos X: %d\n", freeArea->posX);
    printf("pos Y: %d\n", freeArea->posY);

    int outputWidth = coRXdgTopLevel->coRState->focusedOutput->width;
    int outputHeight = coRXdgTopLevel->coRState->focusedOutput->height;
    int sizeX = (freeArea->sizeX == 0) ? outputWidth : freeArea->sizeX;
    int sizeY = (freeArea->sizeY == 0) ? outputHeight : freeArea->sizeY;
    wlr_xdg_toplevel_set_size(coRXdgTopLevel->xdgTopLevel, sizeX, sizeY);

    wlr_scene_node_set_position(&topLevelSceneTree->node, freeArea->posX,
                                freeArea->posY);
    coRXdgTopLevel->posX = freeArea->posX;
    coRXdgTopLevel->posY = freeArea->posY;
    return;
  }

  // Pas d'emplacement vide, découpe la surface focused ou une au hazzard si
  // aucune focus
  if (focusedSurface == NULL) {
    struct coR_xdg_toplevel *xdgTopLevel = wl_container_of(
        &coRXdgTopLevel->coRState->xdgTopLevels, xdgTopLevel, link);
    focusedSurface = xdgTopLevel->xdgTopLevel->base->surface;
  }

  // Split the focused surface and add our one
  struct coR_xdg_toplevel *coRXdgTopLevelAutre =
      focusedSurface->data;
  splitXdgTopLevel(coRXdgTopLevelAutre, coRXdgTopLevel);
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
  struct wlr_xdg_surface *xdgSurface = data;

  struct coR_xdg_toplevel *coRXdgTopLevel =
      wl_container_of(listener, coRXdgTopLevel, destroyListener);
  struct coR_state *coRState = coRXdgTopLevel->coRState;

  // 0.

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
  coRXdgTopLevel->shrunkedTopLevel = NULL;
  coRXdgTopLevel->shrunkerTopLevel = NULL;
  coRXdgTopLevel->posX = 0;
  coRXdgTopLevel->posY = 0;

  // 2.
  wl_list_insert(&coRState->xdgTopLevels, &coRXdgTopLevel->link);

  // 3.
  struct wlr_scene_tree *topLevelSceneTree =
      wlr_scene_xdg_surface_create(&coRState->scene->tree, xdgTopLevel->base);

  // topLevelSceneTree->node.data = xdgTopLevel;
  xdgTopLevel->base->data = topLevelSceneTree;
  xdgTopLevel->base->surface->data = coRXdgTopLevel;

  wlr_scene_node_place_below(&topLevelSceneTree->node,
                             &coRState->cursorScene->node);
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

void splitXdgTopLevel(struct coR_xdg_toplevel *toSplit,
                      struct coR_xdg_toplevel *newXdgTopLevel) {
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
  newXdgTopLevel->shrunkedTopLevel = toSplit;
  toSplit->shrunkerTopLevel = newXdgTopLevel;
  wlr_scene_node_reparent(&topLevelSceneTreeNew->node,
                          topLevelSceneTreeToSplit->node.parent);

  if (width > height) {
    // Ajout à droite ou gauche
    // Position & size de la nouvelle surface
    wlr_scene_node_set_position(&topLevelSceneTreeNew->node, posX + width / 2,
                                posY);
    newXdgTopLevel->posX = toSplit->posX + width / 2;
    newXdgTopLevel->posY = toSplit->posY;
    wlr_xdg_toplevel_set_size(newXdgTopLevel->xdgTopLevel, width / 2, height);

    // Resize the parent surface
    wlr_xdg_toplevel_set_size(toSplit->xdgTopLevel, width / 2, height);

  } else {
    // Ajout en bas ou en haut
    // Position & size de la nouvelle surface
    wlr_scene_node_set_position(&topLevelSceneTreeNew->node, posX,
                                posY + height / 2);
    newXdgTopLevel->posX = toSplit->posX;
    newXdgTopLevel->posY = toSplit->posY + height / 2;
    wlr_xdg_toplevel_set_size(newXdgTopLevel->xdgTopLevel, width, height / 2);

    // Resize the parent surface
    wlr_xdg_toplevel_set_size(toSplit->xdgTopLevel, width, height / 2);
  }
}
