#include "coRXSurface.h"

#include "../inputs/coRCursor.h"
#include "../inputs/coRInputs.h"
#include "coRSurface.h"
#include "coRXdgTopLevel.h"
#include "src/coRState.h"
#include <wayland-util.h>

void xwaylandCommitHandler(struct wl_listener *listener, void *data) {
  // printf("-> xwaylandCommitHandler\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, commitListener);
}

void xwaylandMapHandler(struct wl_listener *listener, void *data) {
  printf("-> xwayland Map\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, mapListener);
}

void xwaylandUnMapHandler(struct wl_listener *listener, void *data) {
  printf("-> xwaylandUnMapHandler\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, unMapListener);

  // Remove the focus if own it
  if (coRXSurface->coRSurface.coRState->focusedSurface ==
      surfaceGetSurface(&coRXSurface->coRSurface)) {
    coRXSurface->coRSurface.coRState->focusedSurface = NULL;
    coRXSurface->coRSurface.coRState->focusedCoRSurface = NULL;

    wlr_seat_keyboard_clear_focus(coRXSurface->coRSurface.coRState->seat);
    wlr_seat_pointer_clear_focus(coRXSurface->coRSurface.coRState->seat);
  }
}

// ----------------

void xwaylandConfigureHandler(struct wl_listener *listener, void *data) {
  printf("xwayland Configure\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, configureListener);

  // Temp values
  short sizeX = 800;
  short sizeY = 700;

  // Set configuration
  wlr_xwayland_surface_configure(coRXSurface->coRSurface.surfaceAbs, 0, 0,
                                 sizeX, sizeY);
}

bool isFirst = true;
void xwaylandAssociateHandler(struct wl_listener *listener, void *data) {
  printf("-> xwaylandAssociateHandler\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, associateListener);
  struct coR_surface *coRSurface = (struct coR_surface *)coRXSurface;
  struct wlr_xwayland_surface *xsurface = coRSurface->surfaceAbs;
  struct coR_state *coRState = coRSurface->coRState;

  if (isFirst == true) {
    isFirst = false;
    return;
  }

  if (!xsurface->surface)
    return;

  coRXSurface->commitListener.notify = xwaylandCommitHandler;
  wl_signal_add(&xsurface->surface->events.commit,
                &coRXSurface->commitListener);

  coRXSurface->unMapListener.notify = xwaylandUnMapHandler;
  wl_signal_add(&xsurface->surface->events.unmap, &coRXSurface->unMapListener);

  coRXSurface->mapListener.notify = xwaylandMapHandler;
  wl_signal_add(&xsurface->surface->events.map, &coRXSurface->mapListener);

  printf("<-xwaylandAssociateHandler\n");

  printf("xwayland first configure\n");
  // Add to the scene tree
  struct wlr_scene_surface *sceneSurface =
      wlr_scene_surface_create(&coRState->scene->tree, xsurface->surface);
  xsurface->data = sceneSurface;
  wlr_scene_node_reparent(&sceneSurface->buffer->node,
                          coRState->workspaces->rootNode);

  // add the structure to the surface data
  xsurface->surface->data = coRXSurface;

  struct wlr_scene_tree *sceneTree = coRState->workspaces->rootNode;

  // Decoration
  float color[4] = {0.8, 0.8, 0.8, 1};
  // Top border
  coRSurface->decoration[0] =
      wlr_scene_rect_create(sceneTree, coRSurface->sizeX, 2, color);
  wlr_scene_node_set_position(&coRSurface->decoration[0]->node, 0, 0);

  // Left border
  coRSurface->decoration[1] =
      wlr_scene_rect_create(sceneTree, 2, coRSurface->sizeY, color);
  wlr_scene_node_set_position(&coRSurface->decoration[1]->node, 0, 0);

  // Down border
  coRSurface->decoration[2] =
      wlr_scene_rect_create(sceneTree, coRSurface->sizeX, 2, color);
  wlr_scene_node_set_position(&coRSurface->decoration[2]->node, 0,
                              coRSurface->sizeY - 2);

  // Right border
  coRSurface->decoration[3] =
      wlr_scene_rect_create(sceneTree, 2, coRSurface->sizeY, color);
  wlr_scene_node_set_position(&coRSurface->decoration[3]->node,
                              coRSurface->sizeX - 2, 0);

  // Variables
  struct coR_surface *focusedCoRSurface = coRState->focusedCoRSurface;
  struct coR_workspace *workspace =
      coRState->workspaces + coRState->focusedWorkspaceNum;

  // Change workspace
  printf("start P0\n");
  coRSurface->onWorkspaceNum = coRState->focusedWorkspaceNum;
  wlr_scene_node_reparent(surfaceGetNode(coRSurface), workspace->rootNode);

  printf("start P1\n");
  // Change the focus on it
  coRState->focusedSurface = xsurface->surface;
  coRState->focusedCoRSurface = coRSurface;
  inputsChangeSurfaceToFocus(coRState, xsurface->surface, 0, 0);

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
  surfaceSetPos(coRSurface, 0, 0);
  surfaceSetSize(coRSurface, workspace->currentOutput->width,
                 workspace->currentOutput->height);
  wl_list_insert(&workspace->xdgTopLevels, &coRSurface->link);
}

void xwaylandDissociateHandler(struct wl_listener *listener, void *data) {
  printf("-> xwaylandDissociateHandler\n");
  /*
    0. Resize, Reposition les surfaces
    1. Retirer de la liste dans struct coR_state
    2. Retire les listeners de leur listes
    3. Remove the focus if own it
  */

  // Variables
  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, dissociateListener);
  struct coR_surface *coRSurface = (struct coR_surface *)coRXSurface;
  struct coR_state *coRState = coRSurface->coRState;

  // 0. Resize all surface to take the place left
  printf("0.\n");
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
      goto endResizeInDissociateFunc;
    }
  }

  // Resize on Y axis
  if (resizeYOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                         lastXdgTopLevelsList)) {
    goto endResizeInDissociateFunc;
  }

  // Resize on X axis
  resizeXOnEmptyArea(startPosX, startPosY, startSizeX, startSizeY,
                     lastXdgTopLevelsList);

endResizeInDissociateFunc:

  // 1.
  printf("1.\n");
  wl_list_remove(&coRSurface->link);

  // 2.
  printf("2.\n");
  wl_list_remove(&coRXSurface->commitListener.link);
  wl_list_remove(&coRXSurface->mapListener.link);
  wl_list_remove(&coRXSurface->unMapListener.link);

  // 3. Remove the focus if own it
  printf("3.\n");
  if (coRXSurface->coRSurface.coRState->focusedSurface ==
      surfaceGetSurface(&coRXSurface->coRSurface)) {
    coRXSurface->coRSurface.coRState->focusedSurface = NULL;
    coRXSurface->coRSurface.coRState->focusedCoRSurface = NULL;

    wlr_seat_keyboard_clear_focus(coRXSurface->coRSurface.coRState->seat);
    wlr_seat_pointer_clear_focus(coRXSurface->coRSurface.coRState->seat);
  }

  // 4. Decoration remove
  for (int i = 0; i < 4; i++) {
    if (coRSurface->decoration[i]) {
      wlr_scene_node_destroy(&coRSurface->decoration[i]->node);
      coRSurface->decoration[i] = NULL;
    }
  }
}

static void xwaylandFullscreenHandler(struct wl_listener *listener,
                                      void *data) {
  printf("->fullscren xwayland\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, fullscreenListener);
  struct coR_state *coRState = coRXSurface->coRSurface.coRState;

  surfaceChangeFullscreen(coRState, (struct coR_surface *)coRXSurface);
}

void xwaylandDestroyHandler(struct wl_listener *listener, void *data) {
  printf("-> xwayland Destroy\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, destroyListener);

  wl_list_remove(&coRXSurface->configureListener.link);
  wl_list_remove(&coRXSurface->associateListener.link);
  wl_list_remove(&coRXSurface->dissociateListener.link);
  wl_list_remove(&coRXSurface->destroyListener.link);
  wl_list_remove(&coRXSurface->fullscreenListener.link);

  free(coRXSurface);
}

// ----------------

void xwaylandNewSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("xwayland new surface\n");

  // Variables
  struct wlr_xwayland_surface *xsurface = data;
  struct coR_state *coRState =
      wl_container_of(listener, coRState, xwaylandNewSurfaceListener);

  // Structure
  struct coR_xsurface *coRXSurface = calloc(1, sizeof(struct coR_xsurface));
  coRXSurface->coRSurface.coRState = coRState;
  coRXSurface->coRSurface.surfaceAbs = xsurface;
  coRXSurface->coRSurface.type = TYPE_XSURFACE;

  // 4. Decoration remove
  for (int i = 0; i < 4; i++) {
      coRXSurface->coRSurface.decoration[i] = NULL;
  }

  // Listeners
  coRXSurface->configureListener.notify = xwaylandConfigureHandler;
  wl_signal_add(&xsurface->events.request_configure,
                &coRXSurface->configureListener);

  coRXSurface->dissociateListener.notify = xwaylandDissociateHandler;
  wl_signal_add(&xsurface->events.dissociate, &coRXSurface->dissociateListener);

  coRXSurface->associateListener.notify = xwaylandAssociateHandler;
  wl_signal_add(&xsurface->events.associate, &coRXSurface->associateListener);

  coRXSurface->destroyListener.notify = xwaylandDestroyHandler;
  wl_signal_add(&xsurface->events.destroy, &coRXSurface->destroyListener);

  coRXSurface->fullscreenListener.notify = xwaylandFullscreenHandler;
  wl_signal_add(&xsurface->events.request_fullscreen,
                &coRXSurface->fullscreenListener);
}

void xwaylandReadyHandler(struct wl_listener *listener, void *data) {
  printf("xwayland ready\n");

  printf("DISPLAY=%s\n", getenv("DISPLAY"));
  if (getenv("DISPLAY") == NULL) {
    printf("WARNING: DISPLAY is not set!\n");
    // setenv("DISPLAY", ":0", 1);
  }
}
