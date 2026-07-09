#ifndef CoRSurface_H
#define CoRSurface_H

// Wlroots
#include "coRState.h"
#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>

// My lib
#include <wlr/backend.h>

// Lib C
#include <stdio.h>

// Structure
struct coR_layer_surface {
  // Main component
  struct wlr_layer_surface_v1 *layerSurface;
  struct coR_state *coRState;
  struct wlr_scene_layer_surface_v1 *sceneLayerSurface;

  // Listeners
  struct wl_listener commitListener;
  struct wl_listener mapListener;
  struct wl_listener unmapListener;
  struct wl_listener destroyListener;
};

// Methods
void newLayerSurfaceHandler(struct wl_listener *listener, void *data);

#endif
