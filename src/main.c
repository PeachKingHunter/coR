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
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_data_device.h>

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>

// LibC
#include <unistd.h> // Forks

#define COMPOSITOR_VERSION 6
#define WM_BASE_VERSION 7 // XdgShell
#define CURSOR_SHAPE_MANAGER_V1_VERSION 2
#define LAYER_SHELL_VERSION 5
#define FRACTIONAL_SCALE_VERSION 1
#define PRESENTATION_VERSION 2
#define LINUX_DMABUF_VERSION 5

int main() {
  // More logs
  wlr_log_init(WLR_DEBUG, NULL);

  /* Initialization Pattern
  0.Structure for server's components, listeners
  1.Create a Wayland display
  2.Create a wlroots backend (graphical)
  3.Inputs managing
    3.1.Cursor
  4.Render's main components
    4.1 Renderer
    4.2 Allocator
  5.surface managing
    5.1 Compositor & SubCompositor
    5.2 XdgTopLevel
    5.3 Scene root for surface position
      5.3.1 Cursor Rect in scene for render cursor
  6.Set up event listeners
  7.Other protocols
  8.Workspaces
  9.Start the backend
  10.Run the Wayland event loop
  11.Clear all
  */

  // 0. Main struture
  struct coR_state coRState = {0};

  // 1. Wayland display
  struct wl_display *display = wl_display_create();
  if (!display)
    exit(1);
  coRState.display = display;

  // 2. Wlroots backend
  struct wl_event_loop *eventLoop = wl_display_get_event_loop(display);

  struct wlr_backend *backend = wlr_backend_autocreate(eventLoop, NULL);
  if (!backend)
    exit(1);
  coRState.backend = backend;

  // 3. Inputs
  coRState.session = wlr_session_create(eventLoop);
  coRState.seat = wlr_seat_create(display, "cearT0");

  //  Set capabilities
  wlr_seat_set_capabilities(coRState.seat, WL_SEAT_CAPABILITY_KEYBOARD |
                                               WL_SEAT_CAPABILITY_POINTER);
  //  3.1 Curseur
  coRState.cursor = wlr_cursor_create();
  coRState.outputLayout = wlr_output_layout_create(display);
  wlr_cursor_attach_output_layout(coRState.cursor, coRState.outputLayout);
  // server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24); // To add

  // 4. Render components
  //  4.1 Renderer
  coRState.renderer = wlr_renderer_autocreate(backend);
  if (coRState.renderer == NULL) {
    wlr_log(WLR_ERROR, "Error: rendererCreation");
    exit(1);
  }
  wlr_renderer_init_wl_display(coRState.renderer, display);

  //  4.2 Allocator
  coRState.allocator = wlr_allocator_autocreate(backend, coRState.renderer);
  if (coRState.allocator == NULL) {
    wlr_log(WLR_ERROR, "Error: allocatorCreation");
    exit(1);
  }

  // 5. Compositor
  //  5.1 Compositor & SubCompositor
  struct wlr_compositor *compositor =
      wlr_compositor_create(display, COMPOSITOR_VERSION, coRState.renderer);
  if (compositor == NULL)
    exit(1);
  coRState.compositor = compositor;

  struct wlr_subcompositor *subCompositor = wlr_subcompositor_create(display);
  if (subCompositor == NULL)
    exit(1);

  //  5.2 XdgTopLevel & LayerSurface
  struct wlr_xdg_shell *xdgShell =
      wlr_xdg_shell_create(display, WM_BASE_VERSION);
  if (xdgShell == NULL)
    exit(1);

  struct wlr_layer_shell_v1 *layerShell =
      wlr_layer_shell_v1_create(display, LAYER_SHELL_VERSION);
  if (layerShell == NULL)
    exit(1);
  // Layer shell TODO: focused surface -> diffrenciate with
  // xdgTopLevel

  //  5.3 Scene root for surface position
  coRState.scene = wlr_scene_create();
  if (coRState.scene == NULL)
    exit(1);

  coRState.sceneLayout =
      wlr_scene_attach_output_layout(coRState.scene, coRState.outputLayout);
  if (coRState.sceneLayout == NULL)
    exit(1);

  //    5.3.1 Cursor Rect in scene for render cursor (temp I think)
  float color[4] = {1, 0, 0, 1};
  coRState.cursorScene =
      wlr_scene_rect_create(&coRState.scene->tree, 10, 10, color);

  // 6. Listeners
  coRState.newOutputListener.notify = newOutputHandler;
  wl_signal_add(&backend->events.new_output, &coRState.newOutputListener);

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

  // 7. Other protocols
  wlr_cursor_shape_manager_v1_create(display, CURSOR_SHAPE_MANAGER_V1_VERSION);
  wlr_viewporter_create(display);
  wlr_fractional_scale_manager_v1_create(display, FRACTIONAL_SCALE_VERSION);
  wlr_presentation_create(display, backend, PRESENTATION_VERSION);
  wlr_linux_dmabuf_v1_create_with_renderer(display, LINUX_DMABUF_VERSION,
                                           coRState.renderer);
  wlr_data_device_manager_create(coRState.display);

  // 8. Workspaces's structure initialisation
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

  // 9. Backend & Socket for get apps
  const char *socket = wl_display_add_socket_auto(coRState.display);
  if (!socket) {
    exit(1);
  }

  wlr_backend_start(backend);

  // Teste open app
  setenv("WAYLAND_DISPLAY", socket, true);
  if (fork() == 0)
    execlp("quickshell", "quickshell", NULL);
  if (fork() == 0)
    execlp("wpaperd", "wpaperd", NULL);
  if (fork() == 0)
    execlp("kitty", "kitty", NULL);
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

  // 10. Launch the compositor
  wl_display_run(display);

  // 11. Clear all
  wl_list_remove(&coRState.newOutputListener.link);
  wl_list_remove(&coRState.newXdgTopLevelListener.link);
  wl_list_remove(&coRState.newInputListener.link);
  wl_list_remove(&coRState.cursorButtonListener.link);
  wl_list_remove(&coRState.cursorMotionListener.link);
  wl_list_remove(&coRState.cursorMotionAbsoluteListener.link);
  wl_list_remove(&coRState.newLayerSurfaceListener.link);

  wlr_scene_node_destroy(&coRState.scene->tree.node);
  wlr_allocator_destroy(coRState.allocator);
  wlr_renderer_destroy(coRState.renderer);
  wlr_cursor_destroy(coRState.cursor);
  wlr_seat_destroy(coRState.seat);
  wlr_session_destroy(coRState.session);
  wlr_backend_destroy(backend);
  wl_display_destroy(display);
  exit(1);
}

/* TODO: Erreur à réglé:
- Ajouter le fullscreen
- Ajout du XWayland (Pour les appli X11 n'ayant pas de version wayland)
- Ajouter la possibilité de changer la config avec un fichier texte
- Être heureux (｡◕‿‿◕｡)
*/
