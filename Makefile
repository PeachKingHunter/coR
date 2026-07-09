compile:
	meson compile

createBuildDir:
	meson setup build/

	cd build
	ninja



# Chercher le nom du package
# sudo ls /usr/lib/pkgconfig/ | grep wlroots
#
# Géneration manuelle du .h de wlr_layer_shell
# sudo wayland-scanner server-header /usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml /usr/include/wlr-layer-shell-unstable-v1-protocol.h
