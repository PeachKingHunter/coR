#include "coROutput.h"
#include "src/coRInputs.h"
#include "src/coRXdgSurface.h"
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_layout.h>

void outputFrameHandler(struct wl_listener *listener, void *data) {
  // printf("-> Render frame of an output\n");
  /* Frame Rendering
    1.Attach renderer to output
    2.Begin rendering
    3.Clear screen with background color
    4.Render content
    5.End rendering
    6.Commit output
  */

  // Gets needed variables
  struct coR_output *coROutput =
      wl_container_of(listener, coROutput, frameListener);
  struct coR_state *coRState = coROutput->coRState;
  struct wlr_output *output = coROutput->output;

  // 2.
  struct wlr_output_state state;
  wlr_output_state_init(&state);

  struct wlr_render_pass *renderPass =
      wlr_output_begin_render_pass(output, &state, NULL);
  if (!renderPass) {
    wlr_output_state_finish(&state);
    return;
  }

  // 3.
  struct wlr_render_rect_options renderRect = {
      .box = {.width = output->width, .height = output->height},
      .color = {.r = 0, .g = 0, .b = 0, .a = 1}};
  wlr_render_pass_add_rect(renderPass, &renderRect);

  // 4.
  if (!wl_list_empty(
          &coRState->xdgSurfaces)) { // Pas sûre de l'utilité de se if
    struct coR_xdg_surface *s;
    wl_list_for_each(s, &coRState->xdgSurfaces, link) {
      if (s->xdgSurface->surface->mapped == false)
        continue;

      struct wlr_texture *texture =
          wlr_surface_get_texture(s->xdgSurface->surface);
      if (texture == NULL)
        continue;

      int posX = s->xdgSurface->surface->current.viewport.src.x;
      int posY = s->xdgSurface->surface->current.viewport.src.y;
      int sizeX = s->xdgSurface->surface->current.viewport.src.width;
      int sizeY = s->xdgSurface->surface->current.viewport.src.height;

      struct wlr_render_texture_options renderTextureOptions = {
          .dst_box = {.x = posX, .y = posY, .height = sizeY, .width = sizeX},
          .texture = texture,
          .transform = WL_OUTPUT_TRANSFORM_NORMAL,
      };
      wlr_render_pass_add_texture(renderPass, &renderTextureOptions);

      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);

      wlr_surface_send_frame_done(s->xdgSurface->surface, &now);
    }
  }

  // Cursor
  struct wlr_render_rect_options cursorRenderRect = {
      .box = {.width = 10,
              .height = 10,
              .x = coRState->cursor->x,
              .y = coRState->cursor->y},
      .color = {.r = 1, .g = 0, .b = 0, .a = 1}};
  wlr_render_pass_add_rect(renderPass, &cursorRenderRect);

  // 5.
  wlr_render_pass_submit(renderPass);

  // 6.
  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);
}

void outputDestroyHandler(struct wl_listener *listener, void *data) {
  printf("-> remove an output\n");
  /*
    1. Clear listeners
    2. Clear structure
  */

  // Variables
  struct coR_output *coROutput =
      wl_container_of(listener, coROutput, destroyListener);

  // 1.
  wl_list_remove(&coROutput->frameListener.link);
  wl_list_remove(&coROutput->destroyListener.link);

  // 2.
  free(coROutput);
  printf("<- remove an output\n");
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
  wl_signal_add(&output->events.destroy, &coROutput->destroyListener);

  coROutput->frameListener.notify = outputFrameHandler;
  wl_signal_add(&output->events.frame, &coROutput->frameListener);

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
  wlr_output_layout_add(coRState->outputLayout, output, 0, 0);

  printf("<- newOutput\n");
}
