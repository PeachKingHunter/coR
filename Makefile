run: compile
	cd forInstall; ./coR

compile:
	cd build; meson compile
	mv build/executable.out forInstall/coR

createBuildDir:
	meson setup build/

	cd build
	ninja

install: compile
	sudo cp forInstall/coR.desktop /usr/share/wayland-sessions;
	sudo cp forInstall/coR /usr/bin/


# Chercher le nom du package
# sudo ls /usr/lib/pkgconfig/ | grep wlroots
#
# Géneration manuelle du .h de wlr_layer_shell
# sudo wayland-scanner server-header /usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml /usr/include/wlr-layer-shell-unstable-v1-protocol.h
