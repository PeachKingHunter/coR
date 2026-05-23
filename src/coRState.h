#ifndef CoRState_H
#define CoRState_H

// wlroot for initialization Pattern
#include <wayland-server-core.h>

// Structure
struct coR_state {
  // Main components
  struct wl_display *display;
  // struct wl_event_loop *eventLoop;
  struct wlr_backend *backend;

  // For surfaces
  struct wlr_compositor *compositor;
  struct wl_list xdgSurfaces;

  // Components for render outputs
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  // Listeners
  struct wl_listener newOutputListener;
  struct wl_listener newSurfaceListener;
  struct wl_listener newXdgSurfaceListener;
};

#endif
