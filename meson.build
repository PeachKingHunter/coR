project('coR', 'c')
add_project_arguments('-DWLR_USE_UNSTABLE', language: 'c')

wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots-0.20')

executable(
    'executable.out',
    'src/main.c',
    'src/coRState.c',
    'src/coROutput.c',
    'src/coRSurface.c',
    'src/coRXdgSurface.c',
    dependencies: [wayland_server, wlroots]
)
