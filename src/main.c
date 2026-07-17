// My lib
#include "coRLayerSurface.h"
#include "coROutput.h"
#include "coRState.h"
#include "coRXdgTopLevel.h"

#include "inputs/coRInputs.h"

// Input managing
#include "wlr/backend/session.h"
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>

// Rendering
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/render/allocator.h>

// Compositor
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_shm.h>
#include <wlr/types/wlr_subcompositor.h>

// Layer shell
#include <wlr/types/wlr_layer_shell_v1.h>

// Other protocoles
#include <wlr/types/wlr_cursor_shape_v1.h> // for hyprpaper cursor_shape_manager
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_viewporter.h>

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>

// LibC
#include <unistd.h> // Forks

#define CURSOR_SHAPE_MANAGER_V1_VERSION 2


int main() {
  // More logs
  wlr_log_init(WLR_DEBUG, NULL);

  /* Initialization Pattern
  0.Structure for server's components, listeners
  1.Create a Wayland display
  2.Create a wlroots backend (graphical)
  2.1 Inputs managing
  2.5.Render's main components
  2.7 Compositor for surface managing
  3.Set up event listeners
  4.Start the backend
  5.Run the Wayland event loop
  */

  // 0.
  struct coR_state coRState = {0};

  // 1.
  struct wl_display *display = wl_display_create();
  if (!display)
    exit(1);
  coRState.display = display;

  // 2.
  struct wl_event_loop *eventLoop = wl_display_get_event_loop(display);

  struct wlr_backend *backend = wlr_backend_autocreate(eventLoop, NULL);
  if (!backend)
    exit(1);
  coRState.backend = backend;

  // 2.1
  coRState.session = wlr_session_create(eventLoop);
  coRState.seat = wlr_seat_create(display, "cearT0");

  // Set capabilities
  wlr_seat_set_capabilities(coRState.seat, WL_SEAT_CAPABILITY_KEYBOARD |
                                               WL_SEAT_CAPABILITY_POINTER);
  // Curseur
  coRState.cursor = wlr_cursor_create();
  coRState.outputLayout = wlr_output_layout_create(display);
  wlr_cursor_attach_output_layout(coRState.cursor, coRState.outputLayout);
  // server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24); // To add

  // 2.5.
  // Renderer
  coRState.renderer = wlr_renderer_autocreate(backend);
  if (coRState.renderer == NULL) {
    wlr_log(WLR_ERROR, "Error: rendererCreation");
    exit(1);
  }
  wlr_renderer_init_wl_display(coRState.renderer, display);

  // Allocator
  coRState.allocator = wlr_allocator_autocreate(backend, coRState.renderer);
  if (coRState.allocator == NULL) {
    wlr_log(WLR_ERROR, "Error: allocatorCreation");
    exit(1);
  }

  // 2.7 - Compositor
  struct wlr_compositor *compositor =
      wlr_compositor_create(display, 6, coRState.renderer);
  if (compositor == NULL)
    exit(1);
  coRState.compositor = compositor;

  wlr_subcompositor_create(display);
  // wlr_data_device_manager_create(server.wl_display); // TO add late
  // (clipboard)

  // struct wlr_shm *shm =
  //     wlr_shm_create_with_renderer(display, 2, coRState.renderer);
  // if (shm == NULL)
  //   exit(1);

  // wlr_renderer_init_wl_shm(coRState.renderer, display);

  struct wlr_xdg_shell *xdgShell = wlr_xdg_shell_create(display, 6);
  if (xdgShell == NULL)
    exit(1);

  wl_list_init(&coRState.xdgTopLevels); // Liste of surfaces

  // List of free area for new surfaces
  // A default surface on the whole screen
  wl_list_init(&coRState.freeAreas);
  struct free_area *freeArea = malloc(sizeof(struct free_area));
  if (freeArea == NULL)
    exit(1);
  freeArea->posX = 0;
  freeArea->posY = 0;
  freeArea->sizeX = 0; // Auto Screen's size
  freeArea->sizeY = 0; // Auto Screen's size
  wl_list_insert(&coRState.freeAreas, &freeArea->link);

  // Test of predef position for surfaces
  // struct free_area *freeArea = malloc(sizeof(struct free_area));
  // if (freeArea == NULL)
  //   exit(1);
  // freeArea->posX = 300;
  // freeArea->posY = 40;
  // freeArea->sizeX = 300; // Auto Screen's size
  // freeArea->sizeY = 200; // Auto Screen's size
  // wl_list_insert(&coRState.freeAreas, &freeArea->link);
  //
  // freeArea = malloc(sizeof(struct free_area));
  // if (freeArea == NULL)
  //   exit(1);
  // freeArea->posX = 52;
  // freeArea->posY = 10;
  // freeArea->sizeX = 200; // Auto Screen's size
  // freeArea->sizeY = 500; // Auto Screen's size
  // wl_list_insert(&coRState.freeAreas, &freeArea->link);

  // Scene root for surface position
  coRState.scene = wlr_scene_create();
  coRState.sceneLayout =
      wlr_scene_attach_output_layout(coRState.scene, coRState.outputLayout);

  // Layer shell TODO: decomment & focused surface -> diffrenciate with
  // xdgTopLevel
  struct wlr_layer_shell_v1 *layerShell = wlr_layer_shell_v1_create(display, 5);

  // 3.
  coRState.newOutputListener.notify = newOutputHandler;
  wl_signal_add(&backend->events.new_output, &coRState.newOutputListener);

  // coRState.newSurfaceListener.notify = newSurfaceHandler;
  // wl_signal_add(&compositor->events.new_surface,
  // &coRState.newSurfaceListener);

  coRState.newXdgTopLevelListener.notify = newXdgTopLevelHandler;
  wl_signal_add(&xdgShell->events.new_toplevel,
                &coRState.newXdgTopLevelListener);

  coRState.newInputListener.notify = newInputHandler;
  wl_signal_add(&backend->events.new_input, &coRState.newInputListener);

  coRState.cursorButtonListener.notify = cursorButtonHandler;
  wl_signal_add(&coRState.cursor->events.button,
                &coRState.cursorButtonListener);

  coRState.cursorMotionListener.notify = cursorMotionHandler;
  wl_signal_add(&coRState.cursor->events.motion,
                &coRState.cursorMotionListener);

  coRState.cursorMotionAbsoluteListener.notify = cursorMotionAbsoluteHandler;
  wl_signal_add(&coRState.cursor->events.motion_absolute,
                &coRState.cursorMotionAbsoluteListener);
  coRState.cursorAxisListener.notify = cursorAxisHandler;
  wl_signal_add(&coRState.cursor->events.axis, &coRState.cursorAxisListener);

  coRState.newLayerSurfaceListener.notify = newLayerSurfaceHandler;
  wl_signal_add(&layerShell->events.new_surface,
                &coRState.newLayerSurfaceListener);

  // Other protocols
  wlr_cursor_shape_manager_v1_create(display, CURSOR_SHAPE_MANAGER_V1_VERSION);
  wlr_viewporter_create(display);
  wlr_fractional_scale_manager_v1_create(display, 1);

  // Cursor Rect for render
  float color[4] = {1, 0, 0, 1};
  coRState.cursorScene =
      wlr_scene_rect_create(&coRState.scene->tree, 10, 10, color);

  // Workspaces
  coRState.focusedWorkspaceNum = 0;
  for (int i = 0; i < NB_WORKSPACE; i++) {
    struct coR_workspace *workspace = coRState.workspaces + i;

    wl_list_init(&workspace->xdgTopLevels);
    workspace->currentOutput = NULL;
    // workspace->posX = 2000 * i;
    workspace->posX = 19204564; // Temp
    workspace->posY = 0;
    workspace->rootNode = wlr_scene_tree_create(&coRState.scene->tree);
    if (workspace->rootNode == NULL)
      exit(1);
    wlr_scene_node_place_below(&workspace->rootNode->node,
                               &coRState.cursorScene->node);
    wlr_scene_node_set_position(&workspace->rootNode->node, workspace->posX,
                                workspace->posY);
  }

  // Socket for get apps
  const char *socket = wl_display_add_socket_auto(coRState.display);
  if (!socket) {
    exit(1);
  }

  // 4.
  wlr_backend_start(backend);

  // Teste open app
  setenv("WAYLAND_DISPLAY", socket, true);
  // if (fork() == 0)
  //   execlp("weston-terminal", "weston-terminal", NULL);
  if (fork() == 0)
    execlp("quickshell", "quickshell", NULL);
  if (fork() == 0)
    execlp("wpaperd", "wpaperd", NULL);
  // if (fork() == 0)
  //   execlp("kitty", "kitty", NULL);
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

  // 5.
  wl_display_run(display);

  // Clear
  wl_list_remove(&coRState.newOutputListener.link);
  wl_list_remove(&coRState.newLayerSurfaceListener.link);
  wl_list_remove(&coRState.newXdgTopLevelListener.link);
  wl_list_remove(&coRState.newInputListener.link);

  wlr_scene_node_destroy(&coRState.scene->tree.node);
  wlr_allocator_destroy(coRState.allocator);
  wlr_renderer_destroy(coRState.renderer);
  wlr_seat_destroy(coRState.seat);
  wlr_session_destroy(coRState.session);
  wlr_backend_destroy(backend);
  wl_display_destroy(display);
}

/* TODO: Erreur à réglé:
- Peut pas décaler un surface sur un workspace vide
- Peut pas enlever la derniere surface d'un workspace sinon boucle infini / crash
*/
