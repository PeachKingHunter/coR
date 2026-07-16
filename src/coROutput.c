#include "coROutput.h"
#include "coRState.h"

void outputFrameHandler(struct wl_listener *listener, void *data) {
  // printf("-> Render frame of an output\n");

  // Gets needed variables
  struct coR_output *coROutput =
      wl_container_of(listener, coROutput, frameListener);
  // struct coR_state *coRState = coROutput->coRState;
  struct wlr_output *output = coROutput->output;
  struct wlr_scene *scene = coROutput->sceneOutput->scene;

  // Render Surfaces of this output
  struct wlr_scene_output *scene_output =
      wlr_scene_get_scene_output(scene, output);
  wlr_scene_output_commit(scene_output, NULL);

  // Send frame with time
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
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
  // wlr_output_layout_add(coRState->outputLayout, output, 0, 0);
  struct wlr_output_layout_output *outputLayoutOutput =
      wlr_output_layout_add_auto(coRState->outputLayout, output);
  struct wlr_scene_output *sceneOutput =
      wlr_scene_output_create(coRState->scene, output);
  coROutput->sceneOutput = sceneOutput;

  wlr_scene_output_layout_add_output(coRState->sceneLayout, outputLayoutOutput,
                                     sceneOutput);

  // Give on start focus if no one as
  if (coRState->focusedOutput == NULL)
    coRState->focusedOutput = output;

  // Give a default workspace
  int currentWorkspace = 0;
  for (; currentWorkspace < NB_WORKSPACE; currentWorkspace++) {
    if (coRState->workspaces[currentWorkspace].currentOutput == NULL)
      break;
  }
  coRState->workspaces[currentWorkspace].currentOutput = coROutput->output;
  coRState->workspaces[currentWorkspace].posX = outputLayoutOutput->x;
  coRState->workspaces[currentWorkspace].posY = outputLayoutOutput->y;
  wlr_scene_node_set_position(
      &coRState->workspaces[currentWorkspace].rootNode->node,
      coRState->workspaces[currentWorkspace].posX,
      coRState->workspaces[currentWorkspace].posY);

  // Render and commit the output
  wlr_scene_output_commit(sceneOutput, NULL);

  printf("<- newOutput\n");
}
