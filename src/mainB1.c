// Others


// LibC
#include <stdio.h>
#include <stdlib.h>

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>

void newOutputHandler(struct wl_listener *listener, void *data) {
  printf("-> newOutput\n");

}

int main() {
  // More logs
  wlr_log_init(WLR_DEBUG, NULL);

  /* Initialization Pattern
    1.Create a Wayland display
    2.Create a wlroots backend
    3.Set up event listeners
    4.Start the backend
    5.Run the Wayland event loop
  */

  // 1.
  struct wl_display *display = wl_display_create();
  if (!display)
    exit(1);

  // 2.
  struct wl_event_loop *eventLoop = wl_display_get_event_loop(display);
  struct wlr_backend *backend = wlr_backend_autocreate(eventLoop, NULL);
  if (!backend)
    exit(1);

  // 3.
  struct wl_listener newOutputListener;
  newOutputListener.notify = newOutputHandler;
  wl_signal_add(&backend->events.new_output, &newOutputListener);

  // 4-5.
  wlr_backend_start(backend);
  wl_display_run(display);
}
