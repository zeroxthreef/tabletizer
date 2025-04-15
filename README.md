# Tabletizer
This is a small daemon that waits for wacom tablet discovery and refreshes both xinput properties and display transform mapping. I don't use a desktop environment so this had to be done with ``xsetwacom``, which doesnt have udev notification functionality. This works well for tablets that disconnect from usb when they are powered off like mine.

## Config
Configuration is done through a json file. The 'example_config.json' file should suffice. Multiple keys can be pressed at once. They just need space between each keysym name. The keysym names are just the standard X11 names.

* The button id is honestly found with trial and error. On my tablet, the buttons are mapped in reverse of how they physically appear.
* The 'display' config name can be found through running ``xrandr``.
* The 'devs' array names can be found though running ``xinput`` and adding all devices from your tablet, though the 'button' boolean should only be applied to the xinput device that has the property ``Wacom Button Actions``.

## Dependencies
* libudev
* Xlib
* libXi
* libXrandr

## Build & Installation
tabletizer.c _is_ the build system, and is used like so:
```
git clone --recursive https://github.com/zeroxthreef/tabletizer
cd tabletizer
chmod +x tabletizer.c
./tabletizer.c
chmod +x install.sh
./install.sh
# you'll want to edit your /usr/local/share/tabletizer/config.json file now
systemctl --user restart tabletizer
```

## TODO
* pen buttons
* dial(wheel) profile
* strip control
* touch control (my tablet does not have this feature)
* more complex actions per button
* maybe pressure curves, but this is usually better configured per application