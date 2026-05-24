#include "coROutput.h"

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
  // TODO: render surfaces
  //   Récupérer leur texture (wlr_surface_get_texture).
  //   Les dessiner à leur position (ex: wlr_render_pass_add_texture).
  struct coR_xdg_surface *s;
  wl_list_for_each(s, &coRState->xdgSurfaces, link) {
    if (s->xdgSurface->surface->mapped == false)
      continue;

    struct wlr_texture *texture =
        wlr_surface_get_texture(s->xdgSurface->surface);
    if (texture == NULL)
      continue;

    struct wlr_render_texture_options renderTextureOptions = {
        .dst_box = {.x = 10, .y = 10, .height = 500, .width = 500},
        .texture = texture,
        .transform = WL_OUTPUT_TRANSFORM_NORMAL,
    };
    wlr_render_pass_add_texture(renderPass, &renderTextureOptions);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    wlr_surface_send_frame_done(s->xdgSurface->surface, &now);  
}

  // 5.
  wlr_render_pass_submit(renderPass);

  // 6.
  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);
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


