#ifndef CoRState_H
#define CoRState_H

// wlroot for initialization Pattern
// #include "surfaces/coRXdgTopLevel.h"
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

  // Surfaces
  struct wlr_compositor *compositor;
  struct wlr_scene *scene;
  struct wlr_scene_output_layout *sceneLayout;
  struct wl_list docks; // All layerSurface docked on border (3 border)

  // Focus
  struct wlr_surface *focusedSurface;
  void *focusedCoRSurface;
  struct wlr_output *focusedOutput;
  int focusedWorkspaceNum;

  // Workspaces & Outputs
  struct coR_workspace workspaces[NB_WORKSPACE];
  struct wl_list outputs;

  // Settings
  struct wl_list commands;
  int scrollPower;

  // Inputs
  struct wlr_session *session;
  struct wlr_seat *seat; // For peripherics

  struct wlr_cursor *cursor;
  struct wlr_scene_rect *cursorScene;
  struct wlr_output_layout *outputLayout;

  struct wlr_tablet_manager_v2 *tabletManager;

  // Components for render
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  // Listeners
  struct wl_listener newOutputListener;
  struct wl_listener newXdgTopLevelListener;
  struct wl_listener newLayerSurfaceListener; // For layerShell

  struct wl_listener newInputListener;
  struct wl_listener cursorButtonListener;
  struct wl_listener cursorMotionListener;
  struct wl_listener cursorMotionAbsoluteListener;
  struct wl_listener cursorAxisListener;

  struct wl_listener xwaylandReadyListener;
  struct wl_listener xwaylandNewSurfaceListener;

  struct wl_listener newDecorationListener;
};

#endif
