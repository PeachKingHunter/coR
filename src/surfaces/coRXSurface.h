#include <stdbool.h>
#include <wayland-server-core.h>
#include "../src/coRState.h"
#include "coRSurface.h"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/xwayland.h>
#include <wlr/backend.h>

#include "../coRState.h"
#include <stddef.h>
#include <wayland-util.h>

#define TYPE_XSURFACE 1

// Structure
struct coR_xsurface {
  struct coR_surface coRSurface;

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
