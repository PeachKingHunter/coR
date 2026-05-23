// libc
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// others libraries
#include "wayland-server-core.h"
// #include "wayland-server.h"
#include "wayland-util.h"
#include "wlroots-0.20/wlr/backend.h"
#include "wlroots-0.20/wlr/render/wlr_renderer.h"

struct waylandServer {
  // Server & backend
  struct wl_display *display; // connection to the Wayland server.
  struct wl_event_loop *eventLoop;

  struct wlr_backend *backend;
  struct wlr_renderer *renderer;

  // Outputs
  struct wl_listener new_output; // Only the last
  struct wl_list outputs;        // mcw_output::link // List of outputs
};

struct mcw_output {
  struct wlr_output *wlr_output;
  struct waylandServer *server;
  struct timespec last_frame;

  struct wl_listener destroy;
  struct wl_listener frame;

  struct wl_list link;
};

static void outputFrameNotifyHandler(struct wl_listener *listener, void *data) {
  // Variables
  struct mcw_output *output = wl_container_of(listener, output, frame);
  struct wlr_output *wlr_output = data;
  struct wlr_renderer *renderer = output->server->renderer;
  printf("frame ready");

  // Test render

  // Guide
  // wlr_output_make_current(wlr_output, NULL);
  // wlr_renderer_begin(renderer, wlr_output);
  //
  // float color[4] = {1.0, 0, 0, 1.0};
  // wlr_renderer_clear(renderer, color);
  //
  // wlr_output_swap_buffers(wlr_output, NULL, NULL);
  // wlr_renderer_end(renderer);

  // Get the renderer and output buffer
  // struct wlr_output_state state = {0};
  // wlr_output_state_init(&state);
  //
  //
  // struct wlr_render_pass *pass = wlr_render_pass_create(renderer,
  // wlr_output); if (!pass) {
  //   wlr_output_state_finish(&state);
  //   return;
  // }
  //
  // // Clear to red
  // float color[4] = {1.0, 0, 0, 1.0};
  // wlr_render_pass_add_rect(pass, &(struct wlr_box){0}, color, NULL);
  //
  // // Submit the render pass
  // wlr_render_pass_submit(pass);
  // wlr_output_commit(wlr_output, &state);
}

static void outputDestroyNotifyHandler(struct wl_listener *listener,
                                       void *data) {
  printf("finished output\n");
  struct mcw_output *output = wl_container_of(listener, output, destroy);
  wl_list_remove(&output->link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->frame.link);
  free(output);
}

// Function for new output notify
static void newOutputNotifyHandler(struct wl_listener *listener, void *data) {
  printf("new output\n");
  // Get server & convert data
  struct waylandServer *server = wl_container_of(listener, server, new_output);
  struct wlr_output *wlrOutput = data;

  // setting the output if have it (ex: 1920x1080@60Hz)
  if (!wl_list_empty(&wlrOutput->modes)) {
    // Get the mode of the output & create, init a state
    struct wlr_output_mode *mode =
        wl_container_of(wlrOutput->modes.prev, mode,
                        link); // modes.prev -> generally the highest config,
                               // can be changed to prefered
    struct wlr_output_state outputState;
    wlr_output_state_init(&outputState);

    // Config the state and set output to it
    wlr_output_state_set_mode(&outputState, mode);
    wlr_output_commit_state(wlrOutput, &outputState);

    // Delete the temp outputState
    wlr_output_state_finish(&outputState);
  }

  // Add it to the list
  struct mcw_output *output = calloc(1, sizeof(struct mcw_output));
  clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
  output->server = server;
  output->wlr_output = wlrOutput;
  wl_list_insert(&server->outputs, &output->link);

  // on output disconnect
  output->destroy.notify = outputDestroyNotifyHandler;
  wl_signal_add(&server->backend->events.destroy, &output->destroy);

  // On output's frame ready to render
  output->frame.notify = outputFrameNotifyHandler;
  wl_signal_add(&output->wlr_output->events.frame, &output->frame);
}

int main(int argc, char **argv) {
  // ------ Init the server & backend ------
  struct waylandServer server;

  // Get a connection to the wayland server
  server.display = wl_display_create();
  assert(server.display);

  // Get the event loop of the display
  server.eventLoop = wl_display_get_event_loop(server.display);
  assert(server.eventLoop);

  // backend
  server.backend = wlr_backend_autocreate(server.eventLoop, NULL);
  assert(server.backend);

  // ------ Outputs ------
  wl_list_init(&server.outputs);

  // on output connect
  server.new_output.notify = newOutputNotifyHandler;
  wl_signal_add(&server.backend->events.new_output, &server.new_output);

  // -- Start backend --
  if (!wlr_backend_start(server.backend)) {
    fprintf(stderr, "backend didn't start\n");
    wl_display_destroy(server.display);
    return 1;
  }

  server.renderer = wlr_renderer_autocreate(server.backend);

  wl_display_run(server.display);
  wl_display_destroy(server.display);

  return 0;
}
