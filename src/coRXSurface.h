#include <stdbool.h>
#include <stdio.h>
#include <wayland-server-core.h>

#define TYPE_XSURFACE 1

// Structure
struct coR_xsurface {
  char type;
  // Main component
  struct wlr_xwayland_surface *xsurface;
  struct coR_state *coRState;

  bool firstCommit;
  // Listeners
  struct wl_listener configureListener;
  struct wl_listener commitListener;
  struct wl_listener mapListener;
  struct wl_listener unMapListener;
  struct wl_listener associateListener;
  struct wl_listener dissociateListener;
  struct wl_listener destroyListener;
};

void xwaylandReadyHandler(struct wl_listener *listener, void *data);
void xwaylandNewSurfaceHandler(struct wl_listener *listener, void *data);
