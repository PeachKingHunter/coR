project('coR', 'c')
add_project_arguments('-DWLR_USE_UNSTABLE', language: 'c')

wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots-0.20')
xkbcommon = dependency('xkbcommon')

executable(
    'executable.out',
    'src/main.c',
    'src/coRState.c',
    'src/coROutput.c',
    'src/coRXdgTopLevel.c',
    'src/inputs/coRInputs.c',
    'src/inputs/coRKeyboard.c',
    'src/inputs/coRCursor.c',
    'src/coRLayerSurface.c',
    'src/coRXSurface.c',
    dependencies: [wayland_server, wlroots, xkbcommon]
)
