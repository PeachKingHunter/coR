#ifndef CoRSurface_H
#define CoRSurface_H

#include "../coROutput.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr-layer-shell-unstable-v1-protocol.h>

// Wlroots
#include "../coRState.h"
#include <wayland-server-core.h>
#include <wlr/types/wlr_layer_shell_v1.h>

// My lib
#include <wlr/backend.h>

// Lib C
#include <stdio.h>

// Structure
struct coR_layer_surface {
  // Should not be move (same place for each surface type struct
  // char type;
  // struct wl_list link;

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
