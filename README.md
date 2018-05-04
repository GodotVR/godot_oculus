# Oculus SDK GDNative plugin for Godot

This repository contains the source code for the Oculus plugin for Godot.
See demo project as an example for how to use the plugin

This is a Windows only plugin!

The leading version of this repository now lives at:
https://github.com/GodotVR/godot_oculus

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