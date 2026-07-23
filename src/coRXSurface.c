#include "coRXSurface.h"
#include "src/coRState.h"
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/xwayland.h>

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
}

// ----------------

void xwaylandConfigureHandler(struct wl_listener *listener, void *data) {
  printf("xwayland Configure\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, configureListener);

  if (coRXSurface->firstCommit == true)
    return;

  // Temp values
  short sizeX = 800;
  short sizeY = 700;

  // Set configuration
  wlr_xwayland_surface_configure(coRXSurface->xsurface, 0, 0, sizeX, sizeY);
}

void xwaylandAssociateHandler(struct wl_listener *listener, void *data) {
  printf("-> xwaylandAssociateHandler\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, associateListener);
  struct coR_state *coRState = coRXSurface->coRState;

  if (!coRXSurface->xsurface->surface)
    return;

  coRXSurface->commitListener.notify = xwaylandCommitHandler;
  wl_signal_add(&coRXSurface->xsurface->surface->events.commit,
                &coRXSurface->commitListener);

  coRXSurface->unMapListener.notify = xwaylandUnMapHandler;
  wl_signal_add(&coRXSurface->xsurface->surface->events.unmap,
                &coRXSurface->unMapListener);

  coRXSurface->mapListener.notify = xwaylandMapHandler;
  wl_signal_add(&coRXSurface->xsurface->surface->events.map,
                &coRXSurface->mapListener);

  printf("<-xwaylandAssociateHandler\n");

  if (!coRXSurface->xsurface->surface)
    return;

  printf("xwayland first configure\n");

  // Add to the scene tree
  struct wlr_scene_surface *sceneSurface = wlr_scene_surface_create(
      &coRState->scene->tree, coRXSurface->xsurface->surface);
  coRXSurface->xsurface->data = sceneSurface;
  wlr_scene_node_reparent(&sceneSurface->buffer->node,
                          coRState->workspaces->rootNode);

  // add the structure to the surface data
  // coRXSurface->xsurface->surface->data = coRXSurface; // Not yet because will
  // cause error with xdgtoplevel for the moment

  // For no other (Useless)
  coRXSurface->firstCommit = true;
}

void xwaylandDissociateHandler(struct wl_listener *listener, void *data) {
  printf("-> xwaylandDissociateHandler\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, dissociateListener);

  wl_list_remove(&coRXSurface->commitListener.link);
  wl_list_remove(&coRXSurface->mapListener.link);
  wl_list_remove(&coRXSurface->unMapListener.link);
}

void xwaylandDestroyHandler(struct wl_listener *listener, void *data) {
  printf("-> xwayland Destroy\n");

  struct coR_xsurface *coRXSurface =
      wl_container_of(listener, coRXSurface, destroyListener);

  wl_list_remove(&coRXSurface->configureListener.link);
  wl_list_remove(&coRXSurface->associateListener.link);
  wl_list_remove(&coRXSurface->dissociateListener.link);
  wl_list_remove(&coRXSurface->destroyListener.link);

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
  coRXSurface->coRState = coRState;
  coRXSurface->xsurface = xsurface;
  coRXSurface->firstCommit = false;
  coRXSurface->type = TYPE_XSURFACE;

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
}

void xwaylandReadyHandler(struct wl_listener *listener, void *data) {
  printf("xwayland ready\n");

  printf("DISPLAY=%s\n", getenv("DISPLAY"));
  if (getenv("DISPLAY") == NULL) {
    printf("WARNING: DISPLAY is not set!\n");
    // setenv("DISPLAY", ":0", 1);
  }
}
