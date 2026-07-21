#include "coRLayerSurface.h"
#include "coROutput.h"
#include "coRState.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr-layer-shell-unstable-v1-protocol.h>

void commitLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> commitLayerSurfaceHandler\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, commitListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;
  // struct coR_state *coRState = coRLayerSurface->coRState;
  struct coR_output *wantedCoROutput = layerSurface->output->data;

  if (!layerSurface->initialized || layerSurface->configured)
    return;

  printf("-> first commit layer surface\n");

  // Get size wanted
  int sizeX = layerSurface->current.desired_width;
  if (sizeX == 0)
    sizeX = layerSurface->output->width;

  int sizeY = layerSurface->current.desired_height;
  if (sizeY == 0)
    sizeY = layerSurface->output->height;

  // TODO:Get pos from anchor
  int posX = 0;
  int posY = 0;

  // TODO here
  //

  posX += wantedCoROutput->sceneOutput->x;
  posY += wantedCoROutput->sceneOutput->y;

  printf("wanted size: %d, %d\n", sizeX, sizeY);
  printf("wanted pos: %d, %d\n", posX, posY);

  wlr_layer_surface_v1_configure(layerSurface, sizeX, sizeY); // TODO: mettre
  // autre par en global pour toute les layerSurfaces

  // Get the workspace wanted by itself
  struct wlr_box full_area = {.x = posX,
                              .y = posY,
                              .width = layerSurface->output->width,
                              .height = layerSurface->output->height};
  struct wlr_box usable_area = {.x = posX,
                                .y = posY,
                                .width = layerSurface->output->width,
                                .height = layerSurface->output->height};
  wlr_scene_layer_surface_v1_configure(coRLayerSurface->sceneLayerSurface,
                                       &full_area, &usable_area);
  if (layerSurface->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
    wlr_scene_node_lower_to_bottom(
        &coRLayerSurface->sceneLayerSurface->tree->node);
  } else {
    wlr_scene_node_raise_to_top(
        &coRLayerSurface->sceneLayerSurface->tree->node);
  }
}

void mapLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> mapLayerSurfaceHandler\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, mapListener);
  // struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;

  // TODO: add get focus
}

void unmapLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> unmapLayerSurfaceHandler\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, unmapListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;
  struct coR_state *coRState = coRLayerSurface->coRState;

  // Remove focuse if have it
  if (coRState->focusedSurface == layerSurface->surface) {
    coRState->focusedSurface = NULL;
    coRState->focusedCoRXdgToplevel = NULL; // Useless line
    wlr_seat_keyboard_clear_focus(coRState->seat);
    wlr_seat_pointer_clear_focus(coRState->seat);
  }
}

void destroyLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> destroyLayerSurfaceHandler\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, destroyListener);
  // struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;

  // unlink listeners
  wl_list_remove(&coRLayerSurface->commitListener.link);
  wl_list_remove(&coRLayerSurface->mapListener.link);
  wl_list_remove(&coRLayerSurface->unmapListener.link);
  wl_list_remove(&coRLayerSurface->destroyListener.link);

  // Free memory
  free(coRLayerSurface);
}

void newLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> newLayerSurfaceHandler\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newLayerSurfaceListener);
  struct wlr_layer_surface_v1 *layerSurface = data;

  // Create the structure
  struct coR_layer_surface *coRLayerSurface =
      calloc(1, sizeof(struct coR_layer_surface));
  if (coRLayerSurface == NULL) {
    printf("Error creating structure for layer surface\n");
    return;
  }

  coRLayerSurface->layerSurface = layerSurface;
  coRLayerSurface->coRState = coRState;

  // Add it to the scene
  coRLayerSurface->sceneLayerSurface =
      wlr_scene_layer_surface_v1_create(&coRState->scene->tree, layerSurface);

  // Listeners
  coRLayerSurface->commitListener.notify = commitLayerSurfaceHandler;
  wl_signal_add(&layerSurface->surface->events.commit,
                &coRLayerSurface->commitListener);

  coRLayerSurface->mapListener.notify = mapLayerSurfaceHandler;
  wl_signal_add(&layerSurface->surface->events.map,
                &coRLayerSurface->mapListener);

  coRLayerSurface->unmapListener.notify = unmapLayerSurfaceHandler;
  wl_signal_add(&layerSurface->surface->events.unmap,
                &coRLayerSurface->unmapListener);

  coRLayerSurface->destroyListener.notify = destroyLayerSurfaceHandler;
  wl_signal_add(&layerSurface->surface->events.destroy,
                &coRLayerSurface->destroyListener);
}
