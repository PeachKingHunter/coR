#ifndef CoRState_H
#define CoRState_H

// wlroot for initialization Pattern
#include "coRXdgTopLevel.h"
#include "wlr/types/wlr_seat.h"
#include <wayland-server-core.h>

// Surface moving & resize (scene)
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>

#define NB_WORKSPACE 10 // 0 to 9

// Structures

struct coR_workspace {
  struct wlr_output *currentOutput;
  struct wlr_scene_tree *rootNode;
  struct wl_list xdgTopLevels;

  int posX;
  int posY;
};

struct coR_state {
  // Main components
  struct wl_display *display;
  // struct wl_event_loop *eventLoop;
  struct wlr_backend *backend;

  // For surfaces
  struct wlr_compositor *compositor;
  struct wlr_scene *scene;
  struct wlr_scene_output_layout *sceneLayout;

  // Workspaces
  struct coR_workspace workspaces[NB_WORKSPACE];

  // Focus
  struct wlr_surface *focusedSurface;
  struct coR_xdg_toplevel *focusedCoRXdgToplevel;
  // TODO: separate focusedTopLevel &
  // focusedSurface due to subsurfaces & layerSurface
  struct wlr_output *focusedOutput;
  int focusedWorkspaceNum;

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
  struct wl_listener newXdgTopLevelListener;
  struct wl_listener newLayerSurfaceListener; // For layerShell

  struct wl_listener newInputListener;
  struct wl_listener cursorButtonListener;
  struct wl_listener cursorMotionListener;
  struct wl_listener cursorMotionAbsoluteListener;
  struct wl_listener cursorAxisListener;
};

#endif
