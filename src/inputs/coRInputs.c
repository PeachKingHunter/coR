#include "coRInputs.h"
#include "coRKeyboard.h"
#include "src/coRState.h"
#include <stdbool.h>
#include <stdio.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

// Global variables
int superPressed = false;

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
    wl_list_remove(&coRKeyboardI->keyListener.link);
  }
  wl_list_remove(&coRKeyboardI->destroyListener.link);

  free(coRKeyboardI);
  printf("<- destroy Input\n");
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
  case WLR_INPUT_DEVICE_TOUCH:
    break;
  case WLR_INPUT_DEVICE_TABLET:
    break;
  case WLR_INPUT_DEVICE_TABLET_PAD:
    break;
  case WLR_INPUT_DEVICE_SWITCH:
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

  // Change focus in case of XdgTopLevel
  if (surface->data != NULL) {
    printf("xdgTopLevelDetected To Focus\n");
    coRState->focusedCoRXdgToplevel = surface->data;
  } else {
    coRState->focusedCoRXdgToplevel = NULL;
  }

  coRState->focusedSurface = surface;

  if (keyboard) {
    wlr_seat_keyboard_notify_enter(coRState->seat, surface, keyboard->keycodes,
                                   keyboard->num_keycodes,
                                   &keyboard->modifiers);
  }
  wlr_seat_pointer_notify_enter(coRState->seat, surface, 0, 0);

  // printf("Focus changed\n"); TODO: not change focus if layerShell already
  // focused
}
