#ifndef coRInputs_H
#define coRInputs_H
#pragma once

// Wlroot
#include <wlr/types/wlr_cursor.h>
#include "wlr/backend/libinput.h"

// My Lib
#include "../coRState.h"
#include "coRCursor.h"

// Lib c
#include <unistd.h>

// Functions
void inputsChangeSurfaceToFocus(struct coR_state *coRState,
                                struct wlr_surface *surface, int cursorX, int cursorY);
void newInputHandler(struct wl_listener *listener, void *data);
#endif
