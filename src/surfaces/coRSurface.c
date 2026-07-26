#include "coRSurface.h"

#include "coRXSurface.h"
#include "coRXdgTopLevel.h"
#include "../inputs/coRCursor.h"
#include "../coRState.h"

int surfaceIsFullScreen(struct coR_surface *coRSurface) {
  // Verif entry
  if (coRSurface == NULL)
    return -1;

  // Get the type
  int type = coRSurface->type;

  if (type == TYPE_XDG_TOPLEVEL)
    return ((struct wlr_xdg_toplevel *)coRSurface->surfaceAbs)
        ->current.fullscreen;

  if (type == TYPE_XSURFACE)
    return ((struct wlr_xwayland_surface *)coRSurface->surfaceAbs)->fullscreen;

  return -1;
}

int surfaceSplit(struct coR_surface *toSplit,
                 struct coR_surface *newCoRSurface) {
  printf("-> splitXdgTopLevel\n");
  if (toSplit == NULL || newCoRSurface == NULL)
    return 0;

  // Variables
  struct wlr_scene_node *toSplitNode = surfaceGetNode(toSplit);
  struct wlr_scene_node *newNode = surfaceGetNode(newCoRSurface);

  if (toSplitNode == NULL || newNode == NULL)
    return 0;

  int posX = toSplit->posX;
  int posY = toSplit->posY;

  int width = toSplit->sizeX;
  int height = toSplit->sizeY;
  printf("after variables\n");

  // Devient frère à celui découpé
  wlr_scene_node_reparent(newNode, toSplitNode->parent);
  printf("after reparent\n");

  // Minimal size
  if (width <= 2 && height <= 2) {
    printf("Too small\n");
    return 0;
  }
  printf("after verif minimal size\n");

  if (width > height) {
    printf("cond width > height entered\n");

    // Ajout à droite ou gauche
    // Position & size de la nouvelle surface
    surfaceSetPos(newCoRSurface, posX + width / 2., posY);
    surfaceSetSize(newCoRSurface, width / 2., height);
    printf("width > height part 1 Ok\n");

    // Resize the parent surface
    surfaceSetSize(toSplit, width / 2., height);

  } else {
    printf("cond width < height entered\n");
    // Ajout en bas ou en haut
    // Position & size de la nouvelle surface
    surfaceSetPos(newCoRSurface, posX, posY + height / 2.);
    surfaceSetSize(newCoRSurface, width, height / 2.);
    printf("width < height part 1 Ok\n");

    // Resize the parent surface
    surfaceSetSize(toSplit, width, height / 2.);
  }
  printf("<- splitXdgTopLevel\n");
  return 1;
}

struct wlr_scene_node *surfaceGetNode(struct coR_surface *coRSurface) {
  // Verif entry
  if (coRSurface == NULL)
    return NULL;

  // Get the type
  int type = coRSurface->type;

  if (type == TYPE_XDG_TOPLEVEL) {
    struct wlr_scene_tree *sceneTree =
        ((struct wlr_xdg_toplevel *)coRSurface->surfaceAbs)->base->data;
    return &sceneTree->node;
  }

  if (type == TYPE_XSURFACE) {
    struct wlr_scene_surface *sceneSurface =
        ((struct wlr_xwayland_surface *)coRSurface->surfaceAbs)->data;
    return &sceneSurface->buffer->node;
  }

  return NULL;
}

/*
  Just change the size/position of a surface with a coR_surface
*/
int surfaceSetSize(struct coR_surface *coRSurface, float newSizeX,
                   float newSizeY) {
  // Verif entry
  if (coRSurface == NULL)
    return -1;

  if (newSizeX <= 22 || newSizeY <= 22)
    return -1;

  // Change the size in the structure
  coRSurface->sizeX = newSizeX;
  coRSurface->sizeY = newSizeY;

  // Get the type
  int type = coRSurface->type;

  if (type == TYPE_XDG_TOPLEVEL) {
    return wlr_xdg_toplevel_set_size(coRSurface->surfaceAbs, coRSurface->sizeX,
                                     coRSurface->sizeY);
  }

  if (type == TYPE_XSURFACE) {
    wlr_xwayland_surface_configure(coRSurface->surfaceAbs, 0, 0,
                                   coRSurface->sizeX, coRSurface->sizeY);
    return 1;
    ;
  }

  return -1;
}

int surfaceSetPos(struct coR_surface *coRSurface, float newPosX,
                  float newPosY) {
  // Verif entry
  if (coRSurface == NULL)
    return -1;

  // Change the position in the structure
  coRSurface->posX = newPosX;
  coRSurface->posY = newPosY;

  // Change position
  struct wlr_scene_node *node = surfaceGetNode(coRSurface);
  wlr_scene_node_set_position(node, coRSurface->posX, coRSurface->posY);

  return -1;
}

/*
  Just change the size/position of a surface with a coR_surface
  But not permanently (don't change the value in the coR_xdg_toplevel structure)
*/
int surfaceSetSizeTemp(struct coR_surface *coRSurface, float newSizeX,
                       float newSizeY) {
  // Verif entry
  if (coRSurface == NULL)
    return -1;

  if (newSizeX <= 22 || newSizeY <= 22)
    return -1;

  // Get the type
  int type = coRSurface->type;

  if (type == TYPE_XDG_TOPLEVEL) {
    return wlr_xdg_toplevel_set_size(coRSurface->surfaceAbs, newSizeX,
                                     newSizeY);
  }

  if (type == TYPE_XSURFACE) {
    wlr_xwayland_surface_configure(coRSurface->surfaceAbs, 0, 0, newSizeX,
                                   newSizeY);
    return 1;
    ;
  }

  return -1;
}

int surfaceSetPosTemp(struct coR_surface *coRSurface, float newPosX,
                      float newPosY) {
  // Verif entry
  if (coRSurface == NULL)
    return -1;

  // Change position
  struct wlr_scene_node *node = surfaceGetNode(coRSurface);
  wlr_scene_node_set_position(node, newPosX, newPosY);

  return -1;
}

int surfaceSetFullscreen(struct coR_surface *coRSurface, bool mode) {
  // Verif entry
  if (coRSurface == NULL)
    return -1;

  // Get the type
  int type = coRSurface->type;

  if (type == TYPE_XDG_TOPLEVEL) {
    return wlr_xdg_toplevel_set_fullscreen(coRSurface->surfaceAbs, mode);
  }

  if (type == TYPE_XSURFACE) {
    wlr_xwayland_surface_set_fullscreen(coRSurface->surfaceAbs, mode);
    return 1;
  }

  return -1;
}
