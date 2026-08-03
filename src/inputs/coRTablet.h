#ifndef CoRTablet_H
#define CoRTablet_H

// Wlroot
#include "src/inputs/coRInputs.h"
#include "wlr/backend/libinput.h"

// My Lib
#include "../coRState.h"
#include "coRCursor.h"

// Lib c
#include <unistd.h>

// Structure
struct coR_tablet_input {
  struct wlr_tablet_v2_tablet *tablet;
  struct wlr_tablet_v2_tablet_tool *tabletToolV2;
  struct wlr_input_device *inputDevice;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener axisListener;
  struct wl_listener proximityListener;
  struct wl_listener buttonListener;
  struct wl_listener tipListener;
};

// Functions
void axisTabletHandler(struct wl_listener *listener, void *data);
void proximityTabletHandler(struct wl_listener *listener, void *data);
void buttonTabletHandler(struct wl_listener *listener, void *data);
void tipTabletHandler(struct wl_listener *listener, void *data);

#endif
