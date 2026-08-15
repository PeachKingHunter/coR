#include "coRTablet.h"
#include "coRInputs.h"
#include "src/inputs/coRInputs.h"
#include <stddef.h>
#include <stdio.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>

void axisTabletHandler(struct wl_listener *listener, void *data) {
  // printf("-> axisTabletHandler\n");

  // Variables
  struct coR_tablet_input *coRTabletI =
      wl_container_of(listener, coRTabletI, axisListener);
  struct wlr_tablet_tool_axis_event *event = data;
  struct coR_state *coRState = coRTabletI->coRState;

  // if (coRTabletI->tabletToolV2 == NULL)
  //   return;

  // printf("P1\n");
  // Move Cursor
  wlr_cursor_move(coRState->cursor, coRTabletI->inputDevice, event->dx,
                  event->dy);

  // printf("x:%fl\n", event->tablet->width_mm);
  // printf("y:%fl\n", event->tablet->height_mm);
  double lx, ly;
  wlr_cursor_absolute_to_layout_coords(
      coRState->cursor, coRTabletI->inputDevice, event->x, event->y, &lx, &ly);

  if (event->updated_axes & WLR_TABLET_TOOL_AXIS_X)
    wlr_cursor_warp(coRState->cursor, coRTabletI->inputDevice, lx,
                    coRState->cursor->y);
  if (event->updated_axes & WLR_TABLET_TOOL_AXIS_Y)
    wlr_cursor_warp(coRState->cursor, coRTabletI->inputDevice,
                    coRState->cursor->x, ly);

  double posX = coRState->cursor->x;
  double posY = coRState->cursor->y;
  // printf("%fd %fd\n", posX, posY);
  wlr_scene_node_set_position(&coRState->cursorScene->node, posX, posY);

  // -> Change le focus si souris sur surface
  double sX, sY;
  struct wlr_surface *surface = getSurfaceBelowCursor(coRState, &sX, &sY);
  if (surface) {
    // Si le focus n'est pas dessus
    if (coRState->focusedSurface != surface)
      inputsChangeSurfaceToFocus(coRState, surface, sX, sY);

    // // Bouge le pointeur physique dedans
    wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX, sY);
  }

  // printf("P2\n");
  // // Notify tablet
  // wlr_tablet_v2_tablet_tool_notify_motion(coRTabletI->tabletToolV2, event->x,
  //                                         event->y);
  // if (event->tool->pressure)
  //   wlr_tablet_v2_tablet_tool_notify_pressure(coRTabletI->tabletToolV2,
  //                                             event->pressure);
  // if (event->tool->distance)
  //   wlr_tablet_v2_tablet_tool_notify_distance(coRTabletI->tabletToolV2,
  //                                             event->distance);

  wlr_seat_pointer_notify_frame(coRState->seat);
}

void proximityTabletHandler(struct wl_listener *listener, void *data) {
  printf("-> proximityTabletHandler\n");
  // // Variables
  // struct coR_tablet_input *coRTabletI =
  //     wl_container_of(listener, coRTabletI, proximityListener);
  // struct wlr_tablet_tool_proximity_event *proximityEvent = data;
  // struct coR_state *coRState = coRTabletI->coRState;
  //
  // printf("P1\n");
  // if (coRTabletI->tabletToolV2 == NULL)
  //   coRTabletI->tabletToolV2 = wlr_tablet_tool_create(
  //       coRTabletI->coRState->tabletManager, coRTabletI->coRState->seat,
  //       proximityEvent->tool);
  //
  // // -> Change le focus si souris sur surface
  // double sX, sY;
  // struct wlr_surface *surface = getSurfaceBelowCursor(coRState, &sX, &sY);
  // if (surface) {
  //   // Si le focus n'est pas dessus
  //   if (coRState->focusedSurface != surface)
  //     inputsChangeSurfaceToFocus(coRState, surface, sX, sY);
  //
  //   // // Bouge le pointeur physique dedans (ENLEVER, C'EST POUR LE POINTER
  //   je
  //   // crois)
  //   // wlr_seat_pointer_notify_motion(coRState->seat, event->time_msec, sX,
  //   sY);
  // }
  //
  // printf("P2\n");
  // // Notify the client
  // if (proximityEvent->state == WLR_TABLET_TOOL_PROXIMITY_IN) {
  //   if (coRTabletI->coRState->focusedSurface)
  //     wlr_tablet_v2_tablet_tool_notify_proximity_in(
  //         coRTabletI->tabletToolV2, coRTabletI->tablet,
  //         coRTabletI->coRState->focusedSurface);
  // } else {
  //   wlr_tablet_v2_tablet_tool_notify_proximity_out(coRTabletI->tabletToolV2);
  // }
}

void buttonTabletHandler(struct wl_listener *listener, void *data) {
  printf("-> buttonTabletHandler\n");
  // // Variables
  // struct coR_tablet_input *coRTabletI =
  //     wl_container_of(listener, coRTabletI, buttonListener);
  // struct wlr_tablet_tool_button_event *event = data;
  //
  // if (coRTabletI->tabletToolV2 == NULL) {
  //   printf("fn quit (no tabletToolV2)\n");
  //   return;
  // }
  //
  // wlr_tablet_v2_tablet_tool_notify_button(coRTabletI->tabletToolV2,
  //                                         event->button, event->state);
}

void tipTabletHandler(struct wl_listener *listener, void *data) {
  printf("-> tipTabletHandler\n");
  // Variables
  struct coR_tablet_input *coRTabletI =
      wl_container_of(listener, coRTabletI, tipListener);
  struct wlr_tablet_tool_tip_event *event = data;
  struct coR_state *coRState = coRTabletI->coRState;

  // if (coRTabletI->tabletToolV2 == NULL) {
  //   printf("fn quit (no tabletToolV2)\n");
  //   return;
  // }

  if (event->state == WLR_TABLET_TOOL_TIP_DOWN) {
    // wlr_tablet_v2_tablet_tool_notify_down(coRTabletI->tabletToolV2);

    // Envoie au client le clique du pointeur
    printf("button pressed with graphical tab\n");
    wlr_seat_pointer_notify_button(coRState->seat, event->time_msec, 272,
                                   WL_POINTER_BUTTON_STATE_PRESSED);
    wlr_seat_pointer_notify_frame(coRState->seat);

  } else if (event->state == WLR_TABLET_TOOL_TIP_UP) {
    // wlr_tablet_v2_tablet_tool_notify_up(coRTabletI->tabletToolV2);

    // Envoie au client le clique du pointeur
    wlr_seat_pointer_notify_button(coRState->seat, event->time_msec, 272,
                                   WL_POINTER_BUTTON_STATE_RELEASED);
    wlr_seat_pointer_notify_frame(coRState->seat);
  }

  // if (event->tool->tilt)
  //   wlr_tablet_v2_tablet_tool_notify_tilt(coRTabletI->tabletToolV2, event->x,
  //                                         event->y);
}
