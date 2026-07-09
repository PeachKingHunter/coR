#ifndef CoRState_H
#define CoRState_H

// wlroot for initialization Pattern
#include "wlr/types/wlr_seat.h"
#include <wayland-server-core.h>

// Surface moving & resize (scene)
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>

// Structure
struct coR_state {
  // Main components
  struct wl_display *display;
  // struct wl_event_loop *eventLoop;
  struct wlr_backend *backend;

  // For surfaces
  struct wlr_compositor *compositor;
  struct wl_list xdgTopLevels;
  struct wlr_scene *scene;
  struct wlr_scene_output_layout *sceneLayout;

  // Placement in output
  struct wl_list freeAreas;

  // Focus
  struct wlr_surface *focusedSurface;
  struct wlr_output *focusedOutput;

  // Components for render outputs
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  // For lib input of wlr
  struct wlr_session *session;
  struct wlr_seat *seat; // For peripherics

  struct wlr_cursor *cursor;
  struct wlr_scene_rect *cursorScene;
  struct wlr_output_layout *outputLayout;

  // Listeners
  struct wl_listener newOutputListener;
  // struct wl_listener newSurfaceListener; // For normal surface (wayland but
  // not used because of xdgTopLevel)
  struct wl_listener newXdgTopLevelListener;

  struct wl_listener newInputListener;

  struct wl_listener cursorButtonListener;
  struct wl_listener cursorMotionListener;
  struct wl_listener cursorMotionAbsoluteListener;
  struct wl_listener cursorAxisListener;

  struct wl_listener newLayerSurfaceListener; // For layerShell
};

struct free_area {
  int posX;
  int posY;
  int sizeX;
  int sizeY;

  struct wl_list link;
};

#endif
