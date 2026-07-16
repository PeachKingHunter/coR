#include "coRKeyboard.h"
#include <stdio.h>
#include <wayland-server-protocol.h>

extern int superPressed;

// ---- ## Keyboard ## ---- //
void keyKeyboardHandler(struct wl_listener *listener, void *data) {
  // Variables
  struct coR_keyboard_input *coRKeyboardI =
      wl_container_of(listener, coRKeyboardI, keyListener);
  struct wlr_keyboard_key_event *event = data;
  struct coR_state *coRState = coRKeyboardI->coRState;
  printf("Touche: %d\n", event->keycode);

  // Raccourci spéciaux
  if (superPressed == true && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    if (event->keycode == 39) { // touche M -> Close compositor
      exit(1);
      return;
    }

    if (event->keycode == 30) { // touche Q -> Open Terminal
      if (fork() == 0) {
        // execlp("weston-terminal", "weston-terminal", NULL);
        execlp("kitty", "kitty", NULL);
      }
      return;
    }

    if ((event->keycode >= 2 && event->keycode <= 10) ||
        event->keycode == 41) { // touche ², 1 à 4 -> Move Workspace 0 to 9
      int otherWorkspaceNum = event->keycode - 1;
      if (otherWorkspaceNum > 9)
        otherWorkspaceNum = 0;

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
