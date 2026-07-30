#ifndef CoRSurface_H
#define CoRSurface_H

// Wlroots
#include "../coRState.h"
#include "src/coROutput.h"
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr-layer-shell-unstable-v1-protocol.h>
#include <wlr/types/wlr_layer_shell_v1.h>

// My lib
#include <wlr/backend.h>

// Lib C
#include <stddef.h>

// Structure
struct coR_layer_surface {
  // List
  struct wl_list link;

  // Placement
  int posX, posY;
  float sizeX, sizeY;

  // Main component
  struct wlr_layer_surface_v1 *layerSurface;
  struct coR_state *coRState;
  struct wlr_scene_layer_surface_v1 *sceneLayerSurface;
  struct coR_output *coROutput;

  // Listeners
  struct wl_listener commitListener;
  struct wl_listener mapListener;
  struct wl_listener unmapListener;
  struct wl_listener destroyListener;
};

// Methods
void newLayerSurfaceHandler(struct wl_listener *listener, void *data);

/*
 * Donne la zone de l'écran utilisable et donc ne chevauchant pas un dock
 * Ne retourne rien, change juste les valeurs dans la structure usableArea
 * donner en argument
 */
void getUsableArea(struct coR_state *coRState, struct wlr_output *output,
                   struct wlr_box *usableArea);

#endif
