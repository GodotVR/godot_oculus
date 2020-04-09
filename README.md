# Oculus SDK GDNative plugin for Godot

This repository contains the source code for the Oculus plugin for Godot.
See demo project as an example for how to use the plugin.

This is a Windows only plugin!

The leading version of this repository lives at:
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
You will also need Visual Studio (tested on 2019), Python 3.x and Scons.

After cloning this repository make sure to initialise the submodules with `git submodule init`
When you've pulled a newer version make sure to run `git submodule update`

If you unzipped the Oculus SDK into the `ovr_sdk_win` folder in this repo you can simply run: `scons`
Else run `scons oculus=<path to oculus SDK>` so it knows where to find the path.

The precompiled version in this repository have been compiled with Visual Studio 2019 and using the Oculus SDK 1.43.0
You will need to install the latest Visual C++ redistributable when deploying the plugin:
https://support.microsoft.com/en-au/help/2977003/the-latest-supported-visual-c-downloads

About this repository
---------------------
This repository was created by and is maintained by Bastiaan Olij a.k.a. Mux213

You can follow me on twitter for regular updates here:
https://twitter.com/mux213

Videos about my work with Godot including tutorials on working with VR in Godot can by found on my youtube page:
https://www.youtube.com/BastiaanOlij
