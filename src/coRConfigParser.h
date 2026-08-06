#ifndef CoRConfigParser_H
#define CoRConfigParser_H

// wlroot
#include "src/coRState.h"
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <xkbcommon/xkbcommon.h>

// Should use: int execvp (const char *file, char *const argv[]);
//    file: points to the file name associated with the file being executed. -> Argv[0]
//    argv:  is a null terminated array of character pointers.

struct keyCommand {
  xkb_keysym_t *keys;
  char **command;
  
  int nbKeys;

  struct wl_list link;
};

// Methods
void runConfig(struct coR_state *coRState);

#endif
