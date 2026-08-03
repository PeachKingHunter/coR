#ifndef CoROuput_H
#define CoROuput_H

// My lib
#include "coRState.h"
#include "inputs/coRInputs.h"

// wlroot
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_cursor.h>

// Struture
struct coR_output {
  struct wl_list link;

  // Components
  struct wlr_output *output;
  struct coR_state *coRState;
  struct wlr_scene_output *sceneOutput;

  // Proprieties
  char *name;
  int posX;
  int posY;

  // Listeners
  struct wl_listener frameListener;
  struct wl_listener destroyListener;
};

// Methods
void outputFrameHandler(struct wl_listener *listener, void *data);
void outputDestroyHandler(struct wl_listener *listener, void *data);
void newOutputHandler(struct wl_listener *listener, void *data);

#endif
