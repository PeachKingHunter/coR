// Others
#include <wayland-server-protocol.h>
#include <wayland-util.h>

// Rendering
#include <wlr/render/allocator.h>
// #include <wlr/render/wlr_renderer.h>

// Compositor
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_shm.h>
#include <wlr/types/wlr_xdg_shell.h>

// LibC
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Forks

// wlroot for initialization Pattern
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/headless.h>
#include <wlr/util/log.h>

// Structures
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

struct coR_output {
  // Components
  struct wlr_output *output;
  struct coR_state *coRState;

  // Listeners
  struct wl_listener frameListener;
  struct wl_listener destroyListener;
};

struct coR_xdg_surface {
  struct wlr_xdg_surface *xdgSurface;

  // Listeners
  struct wl_listener mapListener;
  // struct wl_listener unMapListener;
  struct wl_listener askConfigureListener;

  // List
  struct wl_list link;
};

void mapXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> map XdgSurface\n");
}

void newSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> obtain new surface\n");

  struct wlr_surface *surface = data;
}

void newXdgSurfaceHandler(struct wl_listener *listener, void *data) {
  printf("-> obtain new xdg surface\n");
  /*
    1.Détection : Événement new_surface (XDG ou basique).
    2.Stockage : Ajouter la surface à une liste (ex: wl_list).
    3.Configuration (XDG uniquement) : Envoyer un configure au client pour qu’il
      définisse sa taille/position.
    4.Écoute des états : Gérer map/unmap/destroy
      (pour savoir quand afficher/masquer la surface).
    5.Rendering : Dans outputFrameHandler, parcourir les surfaces mappées.
      Récupérer leur texture (wlr_surface_get_texture).
      Les dessiner à leur position (ex: wlr_render_pass_add_texture).
    6.Validation : wlr_render_pass_submit + wlr_output_commit_state.
  */

  struct wlr_xdg_surface *xdgSurface = data;
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newXdgSurfaceListener);

  if (xdgSurface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
    return;

  // 2.
  struct coR_xdg_surface *coRXdgSurface =
      malloc(sizeof(struct coR_xdg_surface));
  if (coRXdgSurface == NULL) {
    printf("Failling to add struct to list (malloc precisly)\n");
    return;
  }
  coRXdgSurface->xdgSurface = xdgSurface;

  wl_list_insert(&coRState->xdgSurfaces, &coRXdgSurface->link);
  printf("-> Surface saved\n");

  // 3.
  wlr_xdg_surface_schedule_configure(xdgSurface);

  // 4.
  coRXdgSurface->mapListener.notify = mapXdgSurfaceHandler;
  wl_signal_add(&xdgSurface->surface->events.map, &coRXdgSurface->mapListener);
}

void outputFrameHandler(struct wl_listener *listener, void *data) {
  // printf("-> Render frame of an output\n");
  /* Frame Rendering
    1.Attach renderer to output
    2.Begin rendering
    3.Clear screen with background color
    4.Render content
    5.End rendering
    6.Commit output
  */

  // Gets needed variables
  struct coR_output *coROutput =
      wl_container_of(listener, coROutput, frameListener);
  struct coR_state *coRState = coROutput->coRState;
  struct wlr_output *output = coROutput->output;

  // 1.
  output->renderer = coRState->renderer;

  // 2.
  struct wlr_output_state state;
  wlr_output_state_init(&state);
  struct wlr_render_pass *renderPass =
      wlr_output_begin_render_pass(output, &state, NULL);
  if (!renderPass) {
    wlr_output_state_finish(&state);
    return;
  }

  // 3.
  struct wlr_render_rect_options renderRect = {
      .box = {.width = output->width, .height = output->height},
      .color = {.r = 0, .g = 0, .b = 0, .a = 1}};
  wlr_render_pass_add_rect(renderPass, &renderRect);

  // 4.
  // TODO: render surfaces
  //   Récupérer leur texture (wlr_surface_get_texture).
  //   Les dessiner à leur position (ex: wlr_render_pass_add_texture).
  struct coR_xdg_surface *s;
  wl_list_for_each(s, &coRState->xdgSurfaces, link) {
    if (s->xdgSurface->surface->mapped == false)
      continue;

    struct wlr_texture *texture =
        wlr_surface_get_texture(s->xdgSurface->surface);
    if (texture == NULL)
      continue;

    struct wlr_render_texture_options renderTextureOptions = {
        .dst_box = {.x = 0, .y = 0, .height = 200, .width = 200},
        .texture = texture,
        .transform = WL_OUTPUT_TRANSFORM_NORMAL,
    };
    wlr_render_pass_add_texture(renderPass, &renderTextureOptions);
    wlr_surface_send_frame_done(s->xdgSurface->surface, NULL);
  }

  // 5.
  wlr_render_pass_submit(renderPass);

  // 6.
  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);
}

void outputDestroyHandler(struct wl_listener *listener, void *data) {
  printf("-> remove an output\n");
}

void newOutputHandler(struct wl_listener *listener, void *data) {
  printf("-> newOutput\n");
  /* New Output Handling
    0.Init rendering
    1.Create a coR_output structure
    2.Set up frame and destroy listeners
    3.Configure the output (mode, transform, etc.)
    4.Add the output to layouts if applicable
  */

  // Gets needed variables
  struct wlr_output *output = data;
  struct coR_state *coRState =
      wl_container_of(listener, coRState, newOutputListener);

  // 0.
  wlr_output_init_render(output, coRState->allocator, coRState->renderer);

  // 1.
  struct coR_output *coROutput = malloc(sizeof(struct coR_output));
  if (!coROutput)
    return;

  coROutput->output = output;
  coROutput->coRState = coRState;

  // 2.
  coROutput->destroyListener.notify = outputDestroyHandler;
  coROutput->frameListener.notify = outputFrameHandler;

  wl_signal_add(&output->events.frame, &coROutput->frameListener);
  wl_signal_add(&output->events.destroy, &coROutput->destroyListener);

  // 3.
  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
  if (mode)
    wlr_output_state_set_mode(&state, mode);

  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);

  // 4. Layout for multiscreen, scale and other (todo later)

  printf("<- newOutput\n");
}

int main() {
  // More logs
  wlr_log_init(WLR_DEBUG, NULL);

  /* Initialization Pattern
    0.Structure for server's components, listeners
    1.Create a Wayland display
    2.Create a wlroots backend
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
  // struct wlr_backend *backend = wlr_headless_backend_create(eventLoop);
  if (!backend)
    exit(1);
  // coRState.eventLoop = eventLoop;
  coRState.backend = backend;

  // 2.5.
  coRState.renderer = wlr_renderer_autocreate(backend);
  coRState.allocator = wlr_allocator_autocreate(backend, coRState.renderer);

  // 2.7 - Compositor
  struct wlr_compositor *compositor =
      wlr_compositor_create(display, 5, coRState.renderer);
  if (compositor == NULL)
    exit(1);
  coRState.compositor = compositor;
  struct wlr_shm *shm =
      wlr_shm_create_with_renderer(display, 1, coRState.renderer);

  struct wlr_xdg_shell *xdgShell = wlr_xdg_shell_create(display, 6);
  if (xdgShell == NULL || shm == NULL)
    exit(1);

  wl_list_init(&coRState.xdgSurfaces); // Liste of surfaces

  // 3.
  coRState.newOutputListener.notify = newOutputHandler;
  wl_signal_add(&backend->events.new_output, &coRState.newOutputListener);

  coRState.newSurfaceListener.notify = newSurfaceHandler;
  wl_signal_add(&compositor->events.new_surface, &coRState.newSurfaceListener);

  coRState.newXdgSurfaceListener.notify = newXdgSurfaceHandler;
  wl_signal_add(&xdgShell->events.new_surface, &coRState.newXdgSurfaceListener);

  // // Virtual Output -> Compositor have an surface
  // wlr_headless_add_output(backend, 600, 600);

  // Socket for get apps
  const char *socket = wl_display_add_socket_auto(coRState.display);
  if (!socket) {
    exit(1);
  }

  // 4.
  wlr_backend_start(backend);

  // Teste open app
  setenv("WAYLAND_DISPLAY", socket, true);
  if (fork() == 0) {
    // execlp("sh", "sh", "-c", "kitty", (char *)NULL);
    // execlp("kitty", "kitty", (char *)NULL);
    // execlp("dolphin", "dolphin", (char *)NULL);
    execlp("weston", "foot", (char *)NULL);
  }
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

  // 5.
  wl_display_run(display);
}

/* The lifecycle of an XDG surface follows these main states:
  1.Created - Surface is created but not yet configured
  2.Configured - Compositor has sent a configure event which the client
  acknowledged 3.Mapped - Surface has a buffer attached and is visible
  4.Unmapped - Surface no longer has a buffer and should not be rendered
  5.Destroyed - Surface is being freed
*/
