#include "coRInputs.h"
#include "src/coRState.h"
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

void newInputHandler(struct wl_listener *listener, void *data) {
  printf("-> new Input\n");

  // Get variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newInputListener);
  struct wlr_input_device *inputDevice = data;

  // Structure coR_input_d
  struct coR_input_d *coRInputD = calloc(1, sizeof(struct coR_input_d));
  coRInputD->inputDevice = inputDevice;
  coRInputD->coRState = coRState;

  switch (inputDevice->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    printf("keyboard detected\n");
    struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(inputDevice);

    if (keyboard->keymap) {
      wlr_keyboard_set_keymap(keyboard, keyboard->keymap);

    } else {
      /* Extrait de tinyWL: We need to prepare an XKB keymap and assign it to the keyboard. This
       * assumes the defaults (e.g. layout = "us"). -> Changer en clavier fr avec xkb_rule_names*/
      struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
      struct xkb_rule_names ruleNames = {.layout = "fr"};
      struct xkb_keymap *keymap =
          xkb_keymap_new_from_names(context, &ruleNames, XKB_KEYMAP_COMPILE_NO_FLAGS);
      // --

      wlr_keyboard_set_keymap(keyboard, keymap);
      xkb_keymap_unref(keymap);
      xkb_context_unref(context);
      wlr_keyboard_set_repeat_info(keyboard, 25, 600);

    }

    // Set keyboard for a seat
    wlr_seat_set_keyboard(coRState->seat, keyboard);

    // Listeners
    coRInputD->keyListener.notify = grabKeyboardBeginHandler;
    wl_signal_add(&keyboard->events.key, &coRInputD->keyListener);

    // Temporaire, je changerais plus tard
    coRState->temp = coRInputD;
    break;
  }
}

void grabKeyboardBeginHandler(struct wl_listener *listener, void *data) {
  // printf("-> grabKeyboardBeginHandler\n");

  struct coR_input_d *coRInputD =
      wl_container_of(listener, coRInputD, keyListener);
  struct wlr_keyboard_key_event *event = data;
  // printf("%d\n", event->keycode);

  // Vérifie si une surface à le focus
  if (!coRInputD->coRState->focusedSurface) {
    printf("Pas de surface ayant le focus\n");
    return;
  }

  // Send event to focused surface
  if (coRInputD->coRState->focusedSurface) {
    wlr_seat_keyboard_notify_key(coRInputD->coRState->seat, event->time_msec,
                                 event->keycode, event->state);
  }
}
