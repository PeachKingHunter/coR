compile:
	meson compile

createBuildDir:
	meson setup build/

	cd build
	ninja



# Chercher le nom du package
# sudo ls /usr/lib/pkgconfig/ | grep wlroots
