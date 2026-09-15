#include "coRState.h"
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_primary_selection.h>

// void newDataControlManagerHandler(struct wl_listener *listener, void *data) {
//   // struct wlr_data_control_device_v1 *dataControlDevice = data;
//   printf("newDataControlManagerHandler\n");
// }

// With keyboard buttons (copy & paste)
void setSelectionHandler(struct wl_listener *listener, void *data) {
  printf("setSelectionHandler\n");

  // Verif entry
  if (data == NULL)
    return;

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, setSelectionListener);
  struct wlr_seat_request_set_selection_event *event = data;

  // Copy
  wlr_seat_set_selection(coRState->seat, event->source, event->serial);
}

// With mouse button (copy & paste)
void setPrimarySelectionHandler(struct wl_listener *listener, void *data) {
  printf("setPrimarySelectionHandler\n");

  // Verif entry
  if (data == NULL)
    return;

  // Variables
  struct coR_state *coRState =
      wl_container_of(listener, coRState, setPrimarySelectionListener);
  struct wlr_seat_request_set_primary_selection_event *event = data;

  // Copy
  wlr_seat_set_primary_selection(coRState->seat, event->source, event->serial);
}
