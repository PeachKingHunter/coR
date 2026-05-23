// Others
#include <wayland-server-protocol.h>
#include <wayland-util.h>

// Rendering
#include <wlr/render/allocator.h>
// #include <wlr/render/wlr_renderer.h>

// LibC
#include <stdio.h>
#include <stdlib.h>

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>

// Structures
struct coR_state {
  // Main components
  struct wl_display *display;
  // struct wl_event_loop *eventLoop;
  // struct wlr_backend *backend;

  // Components for render outputs
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  // Listeners
  struct wl_listener newOutputListener;
};

struct coR_output {
  // Components
  struct wlr_output *output;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener frameListener;
  struct wl_listener destroyListener;
};

void outputFrameHandler(struct wl_listener *listener, void *data) {
  printf("-> Render frame of an output\n");
}

void outputDestroyHandler(struct wl_listener *listener, void *data) {
  printf("-> remove an output\n");
}

void newOutputHandler(struct wl_listener *listener, void *data) {
  printf("-> newOutput\n");
  /* New Output Handling
    0.Init rendering
    1.Create a coR_output structure
    2.Set up frame and destroy listeners
    3.Configure the output (mode, transform, etc.)
    4.Add the output to layouts if applicable
  */

  // Gets needed variables
  struct wlr_output *output = data;
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newOutputListener);

  // 0.
  wlr_output_init_render(output, coRState->allocator, coRState->renderer);

  // 1.
  struct coR_output *coROutput = malloc(sizeof(struct coR_output));
  if (!coROutput)
    return;

  coROutput->output = output;
  coROutput->coRState = coRState;

  // 2.
  coROutput->destroyListener.notify = outputDestroyHandler;
  coROutput->frameListener.notify = outputFrameHandler;

  wl_signal_add(&output->events.frame, &coROutput->frameListener);
  wl_signal_add(&output->events.destroy, &coROutput->destroyListener);

  // 3.
  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
  if (mode)
    wlr_output_state_set_mode(&state, mode);

  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);

  // 4. Layout for multiscreen, scale and other (todo later)



  printf("<- newOutput\n");
}

int main() {
  // More logs
  wlr_log_init(WLR_DEBUG, NULL);

  /* Initialization Pattern
    0.Structure for server's components, listeners
    1.Create a Wayland display
    2.Create a wlroots backend
    3.Render main components
    3.Set up event listeners
    4.Start the backend
    5.Run the Wayland event loop
  */

  // 0.
  struct coR_state coRState;

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
  // coRState.backend = backend;

  // 2.5.
  coRState.renderer = wlr_renderer_autocreate(backend);  
  coRState.allocator = wlr_allocator_autocreate(backend, coRState.renderer);

  // 3.
  coRState.newOutputListener.notify = newOutputHandler;
  wl_signal_add(&backend->events.new_output, &coRState.newOutputListener);

  // 4-5.
  wlr_backend_start(backend);
  wl_display_run(display);
}
