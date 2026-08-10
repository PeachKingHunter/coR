#include "coRKeyboard.h"
#include "src/coRConfigParser.h"
#include "src/coRState.h"
#include "src/surfaces/coRSurface.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

extern int superPressed;

#include <wlr/types/wlr_xdg_shell.h>

// ---- ## Keyboard ## ---- //
void keyKeyboardHandler(struct wl_listener *listener, void *data) {
  // Variables
  struct coR_keyboard_input *coRKeyboardI =
      wl_container_of(listener, coRKeyboardI, keyListener);
  struct wlr_keyboard_key_event *event = data;
  struct coR_state *coRState = coRKeyboardI->coRState;

  // Convert key
  char keyStr[17];
  struct wlr_keyboard *keyboard =
      wlr_keyboard_from_input_device(coRKeyboardI->inputDevice);
  xkb_keysym_t keySym = xkb_state_key_get_one_sym(
      keyboard->xkb_state, event->keycode + 8);        // Get key sym
  xkb_keysym_get_name(keySym, keyStr, sizeof(keyStr)); // Key in full character

  // Print it for debug
  printf("Touche: %d -> %u, %s\n", event->keycode, keySym, keyStr);

  // key pressed in array
  // printf("key state: %d\n", isKeyPressed(coRKeyboardI, event->keycode));
  keySetPress(coRKeyboardI, keySym,
              event->state == WL_KEYBOARD_KEY_STATE_PRESSED);
  // printf("key state: %d\n", isKeyPressed(coRKeyboardI, event->keycode));

  // Try all commands
  struct keyCommand *command;
  wl_list_for_each(command, &coRState->commands, link) {
    uint8_t allKeysOk = 1;
    for (int i = 0; i < command->nbKeys; i++) {
      if (!isKeyPressed(coRKeyboardI, command->keys[i])) {
        allKeysOk = 0;
        // printf("NOP\n");
        break;
      }
    }

    if (allKeysOk == 0)
      continue;

    printf("command Ok\n");

    // Action from the compositor
    if (strcmp(command->command[0], "compositorAction") == 0) {
      printf("compositorAction\n");
      printf("%50s\n", command->command[0]);
      printf("%50s\n", command->command[1]);

      // Fullscreen
      if (strcmp(command->command[1], "fullscreen") == 0) {
        surfaceChangeFullscreen(coRState, coRState->focusedCoRSurface);
        return;
      }

      // Close compositor
      if (strcmp(command->command[1], "exit") == 0) {
        exit(1);
        return;
      }

      printf("Before\n");
      // Below are action not able in fullscreen
      if (surfaceIsFullScreen(coRState->focusedCoRSurface) == 1) {
        continue;
      }
      printf("After\n");

      // Close focused application
      if (strcmp(command->command[1], "killFocused") == 0) {
        printf("killFocused\n");
        if (coRState->focusedCoRSurface != NULL) {
          struct coR_surface *coRSurface =
              ((struct coR_surface *)coRState->focusedCoRSurface);
          if (coRSurface->type == TYPE_XDG_TOPLEVEL)
            wlr_xdg_toplevel_send_close(coRSurface->surfaceAbs);
          else if (coRSurface->type == TYPE_XSURFACE)
            wlr_xwayland_surface_close(coRSurface->surfaceAbs);
        }
        return;
      }

      continue;
    }

    // Application
    else if (fork() == 0) {
      printf("newApp\n");

      execvp(command->command[0], command->command);
      perror("execvp");
      exit(1);
    }
  }

  // Raccourci spéciaux
  if (superPressed == true && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {

    // Below are action not able in fullscreen
    if (surfaceIsFullScreen(coRState->focusedCoRSurface) == 1) {
      return;
    }

    // touche ², 1 à 4 -> Move Workspace 0 to 9
    if ((event->keycode >= 2 && event->keycode <= 10) || event->keycode == 41) {
      int otherWorkspaceNum = event->keycode - 1;
      if (otherWorkspaceNum > 9)
        otherWorkspaceNum = 0;

      // TODO: should not change the workspace if contain a surface in
      // fullscreen

      printf("%d\n", otherWorkspaceNum);
      if (coRState->focusedWorkspaceNum != otherWorkspaceNum) {

        // Get all other current workspace in variables
        struct coR_workspace *currentWorkspace =
            coRState->workspaces + coRState->focusedWorkspaceNum;
        struct wlr_output *currentOutput = currentWorkspace->currentOutput;
        int currentPosX = currentWorkspace->posX;
        int currentPosY = currentWorkspace->posY;

        // Exchange positions
        struct coR_workspace *otherWorkspace =
            coRState->workspaces + otherWorkspaceNum;

        currentWorkspace->posX = otherWorkspace->posX;
        currentWorkspace->posY = otherWorkspace->posY;
        currentWorkspace->currentOutput = otherWorkspace->currentOutput;
        wlr_scene_node_set_position(&currentWorkspace->rootNode->node,
                                    currentWorkspace->posX,
                                    currentWorkspace->posY);

        otherWorkspace->posX = currentPosX;
        otherWorkspace->posY = currentPosY;
        otherWorkspace->currentOutput = currentOutput;
        wlr_scene_node_set_position(&otherWorkspace->rootNode->node,
                                    otherWorkspace->posX, otherWorkspace->posY);

        coRState->focusedWorkspaceNum = otherWorkspaceNum;
      }
      return;
    }
  }

  if (event->keycode == 125) // touche "super"
    superPressed = !superPressed;

  // superPressed = true; // Temp for testing

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

uint8_t isKeyPressed(struct coR_keyboard_input *coRKeyboardI,
                     uint32_t keyCode) {
  if (keyCode >= MAX_KEYS_SYM)
    return 0;

  int arrayIndex = keyCode / 32;
  int bitInSlot = (keyCode / 32.f - arrayIndex) * 32;

  return (coRKeyboardI->pressedKeys[arrayIndex] & (1 << bitInSlot)) != 0;
}

void keySetPress(struct coR_keyboard_input *coRKeyboardI, uint32_t keyCode,
                 int isPressed) {
  if (keyCode >= MAX_KEYS_SYM)
    return;

  int arrayIndex = keyCode / 32;
  int bitInSlot = (keyCode / 32.f - arrayIndex) * 32;

  if (isPressed == 1) {
    coRKeyboardI->pressedKeys[arrayIndex] |= (1 << bitInSlot);
  } else {
    coRKeyboardI->pressedKeys[arrayIndex] &= ~(1 << bitInSlot);
  }
}
