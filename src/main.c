// My lib
#include "coRState.h"
#include "coROutput.h"
#include "coRXdgSurface.h"
#include "coRSurface.h"

// Rendering
#include <wlr/render/allocator.h>

// Compositor
#include <wlr/types/wlr_shm.h>

// LibC
#include <unistd.h> // Forks

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>

int main() {
  // More logs
  wlr_log_init(WLR_DEBUG, NULL);

  /* Initialization Pattern
    0.Structure for server's components, listeners
    1.Create a Wayland display
    2.Create a wlroots backend
    2.5.Render's main components
    2.7 Compositor for surface managing
    3.Set up event listeners
    4.Start the backend
    5.Run the Wayland event loop
  */

  // 0.
  struct coR_state coRState = {0};

  // 1.
  struct wl_display *display = wl_display_create();
  if (!display)
    exit(1);
  coRState.display = display;

  // 2.
  struct wl_event_loop *eventLoop = wl_display_get_event_loop(display);
  struct wlr_backend *backend = wlr_backend_autocreate(eventLoop, NULL);
  if (!backend)
    exit(1);
  // coRState.eventLoop = eventLoop;
  coRState.backend = backend;

  // 2.5.
  coRState.renderer = wlr_renderer_autocreate(backend);
  coRState.allocator = wlr_allocator_autocreate(backend, coRState.renderer);

  // 2.7 - Compositor
  struct wlr_compositor *compositor =
      wlr_compositor_create(display, 5, coRState.renderer);
  if (compositor == NULL)
    exit(1);
  coRState.compositor = compositor;
  struct wlr_shm *shm =
      wlr_shm_create_with_renderer(display, 1, coRState.renderer);

  struct wlr_xdg_shell *xdgShell = wlr_xdg_shell_create(display, 6);
  if (xdgShell == NULL || shm == NULL)
    exit(1);

  wl_list_init(&coRState.xdgSurfaces); // Liste of surfaces

  // 3.
  coRState.newOutputListener.notify = newOutputHandler;
  wl_signal_add(&backend->events.new_output, &coRState.newOutputListener);

  coRState.newSurfaceListener.notify = newSurfaceHandler;
  wl_signal_add(&compositor->events.new_surface, &coRState.newSurfaceListener);

  coRState.newXdgSurfaceListener.notify = newXdgSurfaceHandler;
  wl_signal_add(&xdgShell->events.new_surface, &coRState.newXdgSurfaceListener);

  // Socket for get apps
  const char *socket = wl_display_add_socket_auto(coRState.display);
  if (!socket) {
    exit(1);
  }

  // 4.
  wlr_backend_start(backend);

  // Teste open app
  setenv("WAYLAND_DISPLAY", socket, true);
  if (fork() == 0) {
    // execlp("sh", "sh", "-c", "kitty", (char *)NULL);
    // execlp("kitty", "kitty", (char *)NULL);
    // execlp("dolphin", "dolphin", (char *)NULL);
    execlp("weston-simple-shm", "weston-simple-shm", NULL);
    // execlp("weston-terminal", "weston-terminal", NULL);
  }
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

  // 5.
  wl_display_run(display);
}
