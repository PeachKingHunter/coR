#ifndef CoRState_H
#define CoRState_H

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include "wlr/types/wlr_seat.h"

// Structure
struct coR_state {
  // Main components
  struct wl_display *display;
  // struct wl_event_loop *eventLoop;
  struct wlr_backend *backend;

  // For surfaces
  struct wlr_compositor *compositor;
  struct wl_list xdgSurfaces;
  struct wlr_surface *focusedSurface;

  // Components for render outputs
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  // For lib input of wlr
  struct wlr_session *session;
  struct wlr_seat *seat; // For peripherics

  struct wlr_cursor *cursor;
  struct wlr_output_layout *outputLayout;

  // Listeners
  struct wl_listener newOutputListener;
  struct wl_listener newSurfaceListener;
  struct wl_listener newXdgSurfaceListener;

  struct wl_listener newInputListener;
  
  struct wl_listener cursorButtonListener;
  struct wl_listener cursorMotionListener;
  struct wl_listener cursorMotionAbsoluteListener;
  struct wl_listener cursorAxisListener;
};

#endif
