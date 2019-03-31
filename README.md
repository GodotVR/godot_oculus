# Oculus SDK GDNative plugin for Godot

**IMPORTANT** for this branch your need to copy `modules/gdnative/include/arvr/godot_arvr.h` into `godot_headers/arvr/`
If compiled correctly this module will work on an original Godot 3.1 stable build but the new logic will be unused.

This repository contains the source code for the Oculus plugin for Godot.
See demo project as an example for how to use the plugin.

This is a Windows only plugin!

The leading version of this repository now lives at:
https://github.com/GodotVR/godot_oculus

Versions
--------
Note that due to a breaking change in Godot 3.1 the current master of this repository creates a GDNative module that can only be used with Godot 3.1 or later.

If you want to build this module for Godot 3.0 please checkout the 3.0 branch.

License
-------
Please check the Oculus SDK for license information in relation to the Oculus SDK used in this project.
The rest of this project is released under an MIT license.

Compiling
---------
In order to compile this plugin you first need to download the Oculus SDK from the Oculus developer website.
You will also need Visual Studio (tested on 2017), Python 2.x and Scons.

After cloning this repository make sure to initialise the submodules with `git submodule init`
When you've pulled a newer version make sure to run `git submodule update`

Then run `scons oculus=<path to oculus SDK>`

About this repository
---------------------
This repository was created by and is maintained by Bastiaan Olij a.k.a. Mux213

You can follow me on twitter for regular updates here:
https://twitter.com/mux213

Videos about my work with Godot including tutorials on working with VR in Godot can by found on my youtube page:
https://www.youtube.com/channel/UCrbLJYzJjDf2p-vJC011lYw