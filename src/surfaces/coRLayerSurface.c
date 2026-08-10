#include "coRLayerSurface.h"
#include "../coROutput.h"
#include <stdint.h>
#include <stdio.h>
#include <wayland-util.h>

void commitLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  // printf("-> commitLayerSurfaceHandler\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, commitListener);
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;
  struct coR_state *coRState = coRLayerSurface->coRState;

  // Only the first commit
  if (!layerSurface->initialized || layerSurface->configured)
    return;

  printf("-> first commit layer surface\n");

  // Recup un écran sinon ne fait rien et attends un prochaine fois
  struct coR_output *wantedCoROutput = NULL;
  if (layerSurface->output != NULL) {
    wantedCoROutput = layerSurface->output->data;
    printf("Have wanted output\n");
  } else {
    if (coRState->focusedOutput) {
      wantedCoROutput = coRState->focusedOutput->data;
    } else {
      return;
    }
  }

  coRLayerSurface->coROutput = wantedCoROutput;
  int outputSizeX = wantedCoROutput->output->width;
  int outputSizeY = wantedCoROutput->output->height;

  // Get size wanted
  int sizeX = layerSurface->current.desired_width;
  printf("sizeX: %d\n", sizeX);
  if (sizeX == 0)
    sizeX = outputSizeX;

  int sizeY = layerSurface->current.desired_height;
  printf("sizeY: %d\n", sizeY);
  if (sizeY == 0)
    sizeY = outputSizeY;

  printf("wanted size: %d, %d\n", sizeX, sizeY);

  wlr_layer_surface_v1_configure(layerSurface, sizeX, sizeY); // TODO: mettre
  // autre par en global pour toute les layerSurfaces

  // Wanted pos
  int posXInWorkspace = 0;
  int posYInWorkspace = 0;

  // TODO:Get pos from anchor
  uint32_t anchor = layerSurface->current.anchor;
  printf("anchor: %d\n", anchor);
  if (anchor == 0 || anchor == 15) {
    // Pas d'anchor ou sur tous les coté => au centre
    posXInWorkspace = (outputSizeX - sizeX) / 2.;
    posYInWorkspace = (outputSizeY - sizeY) / 2.;

  } else {
    // Top Anchor 1
    // Bottom Anchor 2
    // Left Anchor 4
    // Right Anchor 8

    // Have 3 border -> dock
    if (anchor == 7 || anchor == 11 || anchor == 13 || anchor == 14)
      wl_list_insert(&coRState->docks, &coRLayerSurface->link);

    // Pas top mais bottom
    if (!(anchor & (1 << 0)) && (anchor & (1 << 1))) {
      posYInWorkspace = outputSizeY - sizeY;
    }
    // Pas gauche mais droite
    if (!(anchor & (1 << 2)) && (anchor & (1 << 3))) {
      posXInWorkspace = outputSizeX - sizeX;
    }
  }

  // TODO here
  int outputPosX = wantedCoROutput->sceneOutput->x;
  int outputPosY = wantedCoROutput->sceneOutput->y;

  int posX = posXInWorkspace + outputPosX;
  int posY = posYInWorkspace + outputPosY;

  printf("wanted pos: %d, %d\n", posX, posY);

  // Get the workspace wanted by itself
  struct wlr_box full_area = {.x = outputPosX,
                              .y = outputPosY,
                              .width = outputSizeX,
                              .height = outputSizeY};

  coRLayerSurface->posX = posX;
  coRLayerSurface->posY = posY;
  coRLayerSurface->sizeX = sizeX;
  coRLayerSurface->sizeY = sizeY;
  struct wlr_box usable_area = {
      .x = posX, .y = posY, .width = sizeX, .height = sizeY};
  wlr_scene_layer_surface_v1_configure(coRLayerSurface->sceneLayerSurface,
                                       &full_area, &usable_area);

  // Hierarchi selon le layer de la surface
  if (layerSurface->current.layer == ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND) {
    wlr_scene_node_lower_to_bottom(
        &coRLayerSurface->sceneLayerSurface->tree->node);
  } else {
    wlr_scene_node_place_below(&coRLayerSurface->sceneLayerSurface->tree->node,
                               &coRState->cursorScene->node);
  }
}

void mapLayerSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> mapLayerSurfaceHandler\n");

  // Variables
  struct coR_layer_surface *coRLayerSurface =
      wl_container_of(listener, coRLayerSurface, mapListener);
  struct coR_state *coRState = coRLayerSurface->coRState;
  struct wlr_layer_surface_v1 *layerSurface = coRLayerSurface->layerSurface;

  // get focus
  coRState->focusedSurface = coRLayerSurface->layerSurface->surface;
  inputsChangeSurfaceToFocus(coRState, layerSurface->surface, 0, 0);
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
    coRState->focusedCoRSurface = NULL; // Useless line
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

  uint32_t anchor = coRLayerSurface->layerSurface->current.anchor;
  if (anchor == 7 || anchor == 11 || anchor == 13 || anchor == 14)
    wl_list_remove(&coRLayerSurface->link);

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
  coRLayerSurface->coROutput = NULL;

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

void getUsableArea(struct coR_state *coRState, struct wlr_output *output,
                   struct wlr_box *usableArea) {
  int posX = 0;
  int posY = 0;
  int sizeX = output->width;
  int sizeY = output->height;

  struct coR_layer_surface *tmpCoRLS;
  wl_list_for_each(tmpCoRLS, &coRState->docks, link) {
    uint32_t anchor = tmpCoRLS->layerSurface->current.anchor;
    // Tmp disable (hypothesis: Same docks on all screen)
    // if (tmpCoRLS->coROutput->output != workspace->currentOutput)
    //   continue;

    // Dock petit X (à gauche)
    if (tmpCoRLS->posX <= posX && posX <= tmpCoRLS->posX + tmpCoRLS->sizeX &&
        !(anchor & (1 << 3))) {
      sizeX -= tmpCoRLS->posX + tmpCoRLS->sizeX - posX;
      posX = tmpCoRLS->posX + tmpCoRLS->sizeX;
    }

    // Dock grand X (à droite)
    if (tmpCoRLS->posX <= posX + sizeX &&
        posX + sizeX <= tmpCoRLS->posX + tmpCoRLS->sizeX &&
        !(anchor & (1 << 2))) {
      printf("enterXD\n");
      sizeX -= posX + sizeX - tmpCoRLS->posX;
    }

    // Dock grand Y (en bas)
    if (tmpCoRLS->posY <= posY + sizeY &&
        posY + sizeY <= tmpCoRLS->posY + tmpCoRLS->sizeY &&
        !(anchor & (1 << 0))) {
      printf("enterYB\n");
      sizeY -= posY + sizeY - tmpCoRLS->posY;
    }

    // Dock petit Y (en haut)
    if (tmpCoRLS->posY <= posY && posY <= tmpCoRLS->posY + tmpCoRLS->sizeY &&
        !(anchor & (1 << 1))) {
      sizeY -= tmpCoRLS->posY + tmpCoRLS->sizeY - posY;
      posY = tmpCoRLS->posY + tmpCoRLS->sizeY;
    }
  }
  usableArea->x = posX;
  usableArea->y = posY;
  usableArea->width = sizeX;
  usableArea->height = sizeY;
}
