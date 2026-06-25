#include "coRInputs.h"

// ---- ## Keyboard ## ---- //
void destroyInputHandler(struct wl_listener *listener, void *data) {
  printf("-> destroy Input\n");
  /*
    1. Listeners
  */

  // Variables
  struct coR_keyboard_input *coRKeyboardI =
      wl_container_of(listener, coRKeyboardI, destroyListener);

  // 1.
  if (coRKeyboardI->inputDevice->type == WLR_INPUT_DEVICE_KEYBOARD) {
    wl_list_remove(&coRKeyboardI->keyListener.link); // Crash si mis
  }
  wl_list_remove(&coRKeyboardI->destroyListener.link);

  free(coRKeyboardI);
  printf("<- destroy Input\n");
}

int superPressed = false;
void keyKeyboardHandler(struct wl_listener *listener, void *data) {
  // Variables
  struct coR_keyboard_input *coRKeyboardI =
      wl_container_of(listener, coRKeyboardI, keyListener);
  struct wlr_keyboard_key_event *event = data;
  printf("Touche: %d\n", event->keycode);

  // Raccourci spéciaux
  if (superPressed == true && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    if (event->keycode == 39) { // touche M
      exit(1);
      return;
    }

    if (event->keycode == 30) { // touche Q
      if (fork() == 0) {
        execlp("weston-terminal", "weston-terminal", NULL);
      }
      return;
    }
  }

  if (event->keycode == 125) // touche "super"
    superPressed = !superPressed;

  // Vérifie si une surface à le focus
  if (!coRKeyboardI->coRState->focusedSurface) {
    printf("Pas de surface ayant le focus\n");
    return;
  }

  // Send event to focused surface
  if (coRKeyboardI->coRState->focusedSurface) {
    wlr_seat_keyboard_notify_key(coRKeyboardI->coRState->seat, event->time_msec,
                                 event->keycode, event->state);
  }
}

void modifierKeyboardHandler(struct wl_listener *listener, void *data) {
  // printf("-> modifier keyboard\n");

  // Variables
  struct coR_keyboard_input *coRKeyboardI =
      wl_container_of(listener, coRKeyboardI, modifierListener);
  struct wlr_keyboard *keyboard =
      wlr_keyboard_from_input_device(coRKeyboardI->inputDevice);

  // Send new modifier to the seat
  wlr_seat_keyboard_notify_modifiers(coRKeyboardI->coRState->seat,
                                     &keyboard->modifiers);
}

// ---- ## Curseur ## ---- //
void cursorButtonHandler(struct wl_listener *listener, void *data) {
  // printf("-> buttonCursor\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorButtonListener);
  struct wlr_pointer_button_event *event = data;

  // Envoie au client le clique
  wlr_seat_pointer_notify_button(coRState->seat, event->time_msec,
                                 event->button, event->state);
  wlr_seat_pointer_notify_frame(coRState->seat);
}

void cursorMotionHandler(struct wl_listener *listener, void *data) {
  // printf("-> motionCursor\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorMotionListener);
  struct wlr_pointer_motion_event *event = data;
  double posX = coRState->cursor->x;
  double posY = coRState->cursor->y;

  // Move Cursor
  wlr_cursor_move(coRState->cursor, &event->pointer->base, event->delta_x,
                  event->delta_y);
  wlr_scene_node_set_position(&coRState->cursorScene->node, posX, posY);

  // Detecte la surface en dessous du curseur
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, false);
  double sX, sY;
  struct wlr_scene_node *node =
      wlr_scene_node_at(&coRState->scene->tree.node, posX, posY, &sX, &sY);
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, true);

  // Si aucune surface sous le curseur
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
    return;

  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);

  // Si pas réussi à l'obtenir
  if (!scene_surface)
    return;

  struct wlr_surface *surface = scene_surface->surface;
  if (surface) {
    // Si le focus n'est pas dessus
    if (coRState->focusedSurface != surface)
      inputsChangeSurfaceToFocus(coRState, surface, sX, sY);

    // Bouge le pointeur physique dedans
    wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX, sY);
  }

  // Retire le focus si il n'y à pas de surface à focus
  if (!coRState->focusedSurface) {
    wlr_seat_pointer_clear_focus(coRState->seat);
  }

  // Envoie au client
  wlr_seat_pointer_notify_frame(coRState->seat);
}

void cursorMotionAbsoluteHandler(struct wl_listener *listener, void *data) {
  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorMotionAbsoluteListener);
  struct wlr_pointer_motion_absolute_event *event = data;
  double posX = coRState->cursor->x;
  double posY = coRState->cursor->y;

  // Move Cursor
  wlr_cursor_warp_absolute(coRState->cursor, &event->pointer->base, event->x,
                           event->y);
  wlr_scene_node_set_position(&coRState->cursorScene->node, posX, posY);

  // Detecte la surface en dessous du curseur
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, false);
  double sX, sY;
  struct wlr_scene_node *node =
      wlr_scene_node_at(&coRState->scene->tree.node, posX, posY, &sX, &sY);
  wlr_scene_node_set_enabled(&coRState->cursorScene->node, true);

  // Si aucune surface sous le curseur
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
    return;

  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);

  // Si pas réussi à l'obtenir
  if (!scene_surface)
    return;

  struct wlr_surface *surface = scene_surface->surface;
  if (surface) {
    // Si le focus n'est pas dessus
    if (coRState->focusedSurface != surface)
      inputsChangeSurfaceToFocus(coRState, surface, sX, sY);

    // Bouge le pointeur physique dedans
    wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX, sY);
  }

  // Retire le focus si il n'y à pas de surface à focus
  if (!coRState->focusedSurface) {
    wlr_seat_pointer_clear_focus(coRState->seat);
  }

  // Envoie au client
  wlr_seat_pointer_notify_frame(coRState->seat);
}

void cursorAxisHandler(struct wl_listener *listener, void *data) {
  printf("-> axisCursor\n");

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, cursorAxisListener);
  struct wlr_pointer_axis_event *event = data;

  // Envoie au client
  wlr_seat_pointer_notify_axis(
      coRState->seat, event->time_msec, event->orientation, event->delta,
      event->delta_discrete, event->source, event->relative_direction);
  wlr_seat_pointer_notify_frame(coRState->seat);
}

// ---- ## coRState's Listener for new inputs ## ---- //
void newInputHandler(struct wl_listener *listener, void *data) {
  printf("-> new Input\n");

  // Get variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newInputListener);
  struct wlr_input_device *inputDevice = data;

  switch (inputDevice->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    printf("keyboard detected\n");

    // Structure coR_keyboard_input
    struct coR_keyboard_input *coRKeyboardI =
        calloc(1, sizeof(struct coR_keyboard_input));
    coRKeyboardI->inputDevice = inputDevice;
    coRKeyboardI->coRState = coRState;

    struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(inputDevice);

    if (keyboard->keymap) {
      wlr_keyboard_set_keymap(keyboard, keyboard->keymap);

    } else {
      /* Extrait de tinyWL: We need to prepare an XKB keymap and assign it to
       * the keyboard. This assumes the defaults (e.g. layout = "us"). ->
       * Changer en clavier fr avec xkb_rule_names*/
      struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
      struct xkb_rule_names ruleNames = {.layout = "fr"};
      struct xkb_keymap *keymap = xkb_keymap_new_from_names(
          context, &ruleNames, XKB_KEYMAP_COMPILE_NO_FLAGS);
      // --

      wlr_keyboard_set_keymap(keyboard, keymap);
      xkb_keymap_unref(keymap);
      xkb_context_unref(context);
      wlr_keyboard_set_repeat_info(keyboard, 25, 600);
    }

    // Set keyboard for a seat
    wlr_seat_set_keyboard(coRState->seat, keyboard);

    // Listeners
    coRKeyboardI->keyListener.notify = keyKeyboardHandler;
    wl_signal_add(&keyboard->events.key, &coRKeyboardI->keyListener);

    coRKeyboardI->modifierListener.notify = modifierKeyboardHandler;
    wl_signal_add(&keyboard->events.modifiers, &coRKeyboardI->modifierListener);

    coRKeyboardI->destroyListener.notify = destroyInputHandler;
    wl_signal_add(&coRKeyboardI->inputDevice->events.destroy,
                  &coRKeyboardI->destroyListener);
    break;

  case WLR_INPUT_DEVICE_POINTER:
    printf("pointer detected\n");

    wlr_cursor_attach_input_device(coRState->cursor, inputDevice);
    break;
  }
}

// Change the surface that is focused by input-devices
void inputsChangeSurfaceToFocus(struct coR_state *coRState,
                                struct wlr_surface *surface, int cursorX,
                                int cursorY) {
  // Obtien le clavier
  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(coRState->seat);

  // Suprime l'ancien focus
  if (coRState->focusedSurface) {
    wlr_seat_keyboard_clear_focus(coRState->seat);
    wlr_seat_pointer_clear_focus(coRState->seat);
  }

  // Change focus
  coRState->focusedSurface = surface;

  if (keyboard) {
    wlr_seat_keyboard_notify_enter(coRState->seat, coRState->focusedSurface,
                                   keyboard->keycodes, keyboard->num_keycodes,
                                   &keyboard->modifiers);
  }
  wlr_seat_pointer_notify_enter(coRState->seat, coRState->focusedSurface, 0, 0);

  printf("Focus changed\n");
}
