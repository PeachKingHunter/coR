#include "coRXdgSurface.h"


static void commitXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  // printf("-> commit XdgSurface\n");

  struct coR_xdg_surface *coRXdgSurface =
      wl_container_of(listener, coRXdgSurface, commitListener);

  if (coRXdgSurface->xdgSurface->initialized) {
    if (!coRXdgSurface->xdgSurface->configured) {
      printf("-> first commit XdgSurface\n");
      wlr_xdg_surface_schedule_configure(coRXdgSurface->xdgSurface);
    }
  }
}

static void mapXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> map XdgSurface\n");

  struct coR_xdg_surface *coRXdgSurface =
      wl_container_of(listener, coRXdgSurface, mapListener);
  struct coR_state *coRState = coRXdgSurface->coRState;

  inputsChangeSurfaceToFocus(coRState, coRXdgSurface->xdgSurface->surface, 0,
                             0);
}

static void unmapXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> unmap XdgSurface\n");

  struct coR_xdg_surface *coRXdgSurface =
      wl_container_of(listener, coRXdgSurface, mapListener);
  struct coR_state *coRState = coRXdgSurface->coRState;

  if (coRState->focusedSurface == coRXdgSurface->xdgSurface->surface) {
    coRState->focusedSurface = NULL;
    wlr_seat_keyboard_clear_focus(coRState->seat);
    wlr_seat_pointer_clear_focus(coRState->seat);
  }
}

static void destroyXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> destroy XdgSurface\n");
  /*
    1. Retirer de la liste dans struct coR_state
    2. Retire les listeners de leur listes
    3. Clear la mémoire utilisé
  */

  // Variables
  struct wlr_xdg_surface *xdgSurface = data;

  struct coR_xdg_surface *coRXdgSurface =
      wl_container_of(listener, coRXdgSurface, destroyListener);
  struct coR_state *coRState = coRXdgSurface->coRState;

  // 1.
  wl_list_remove(&coRXdgSurface->link);

  // 2.
  wl_list_remove(&coRXdgSurface->mapListener.link);
  wl_list_remove(&coRXdgSurface->unMapListener.link);
  wl_list_remove(&coRXdgSurface->destroyListener.link);
  wl_list_remove(&coRXdgSurface->commitListener.link);

  // 3.
  free(coRXdgSurface);
  printf("<- destroy XdgSurface\n");
}

void newXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> obtain new xdg surface\n");
  /*
    1.Détection : Événement new_surface (XDG ou basique).
    2.Stockage : Ajouter la surface à une liste (ex: wl_list).
    3.Configuration (XDG uniquement) : Envoyer un configure au client pour qu’il
      définisse sa taille/position.
    4.Écoute des états : Gérer map/unmap/destroy
    5.Rendering : Dans outputFrameHandler, parcourir les surfaces mappées.
      Récupérer leur texture (wlr_surface_get_texture).
      Les dessiner à leur position (ex: wlr_render_pass_add_texture).
    6.Validation : wlr_render_pass_submit + wlr_output_commit_state.
  */

  struct wlr_xdg_surface *xdgSurface = data;
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newXdgSurfaceListener);

  // 2.
  struct coR_xdg_surface *coRXdgSurface =
      calloc(1, sizeof(struct coR_xdg_surface));
  if (coRXdgSurface == NULL) {
    printf("Failling to add struct to list (malloc precisly)\n");
    return;
  }
  coRXdgSurface->xdgSurface = xdgSurface;
  coRXdgSurface->coRState = coRState;

  // 3.
  wl_list_insert(&coRState->xdgSurfaces, &coRXdgSurface->link);
  struct wlr_scene_tree *sceneTree =
      wlr_scene_xdg_surface_create(&coRState->scene->tree, xdgSurface);
  sceneTree->node.data = xdgSurface;
  wlr_scene_node_place_below(&sceneTree->node, &coRState->cursorScene->node);
  printf("-> Surface saved\n");

  wlr_scene_node_set_position(&sceneTree->node, 50, 10);
  // 4.
  coRXdgSurface->mapListener.notify = mapXdgSurfaceHandler;
  wl_signal_add(&xdgSurface->surface->events.map, &coRXdgSurface->mapListener);

  coRXdgSurface->unMapListener.notify = unmapXdgSurfaceHandler;
  wl_signal_add(&xdgSurface->surface->events.unmap,
                &coRXdgSurface->unMapListener);

  coRXdgSurface->destroyListener.notify = destroyXdgSurfaceHandler;
  wl_signal_add(&xdgSurface->events.destroy, &coRXdgSurface->destroyListener);

  coRXdgSurface->commitListener.notify = commitXdgSurfaceHandler;
  wl_signal_add(&xdgSurface->surface->events.commit,
                &coRXdgSurface->commitListener);

  printf("give focus\n");
}

/* The lifecycle of an XDG surface follows these main states:
  1.Created - Surface is created but not yet configured
  2.Configured - Compositor has sent a configure event which the client
  acknowledged 3.Mapped - Surface has a buffer attached and is visible
  4.Unmapped - Surface no longer has a buffer and should not be rendered
  5.Destroyed - Surface is being freed
*/
