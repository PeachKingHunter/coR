#include "coRLayerSurface.h"
#include "coRState.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

void commitLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  // printf("-> commit layer surface\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, commitListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;
  struct coR_state *coRState = coRLayerSurface->coRState;

  if (!layerSurface->initialized || layerSurface->configured)
    return;

  printf("-> first commit layer surface\n");
  int sizeX = layerSurface->current.desired_width;
  int sizeY = layerSurface->current.desired_height;

  wlr_layer_surface_v1_configure(layerSurface, sizeX, sizeY);

  struct wlr_box full_area = {.x = 0, .y = 0, .width = 0, .height = 0};
  struct wlr_box usable_area = {.x = 0,
                                .y = 0,
                                .width = coRState->focusedOutput->width,
                                .height = coRState->focusedOutput->height};
  wlr_scene_layer_surface_v1_configure(coRLayerSurface->sceneLayerSurface,
                                       &full_area, &usable_area);
}

void mapLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> map layer surface\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, mapListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;

  // TODO: add get focus
}

void unmapLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> unmap layer surface\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, unmapListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;
  struct coR_state *coRState = coRLayerSurface->coRState;

  // Remove focuse if have it
  if (coRState->focusedSurface == layerSurface->surface) {
    coRState->focusedSurface = NULL;
    wlr_seat_keyboard_clear_focus(coRState->seat);
    wlr_seat_pointer_clear_focus(coRState->seat);
  }
}

void destroyLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> destroy layer surface\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, destroyListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;

  // unlink listeners
  wl_list_remove(&coRLayerSurface->commitListener.link);
  wl_list_remove(&coRLayerSurface->mapListener.link);
  wl_list_remove(&coRLayerSurface->unmapListener.link);
  wl_list_remove(&coRLayerSurface->destroyListener.link);

  // Free memory
  free(coRLayerSurface);
}

void newLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> obtain new layer surface\n");

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
