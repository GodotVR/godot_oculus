# Oculus SDK GDNative plugin for Godot

This repository contains the source code for the Oculus plugin for Godot.
See demo project as an example for how to use the plugin

This is a Windows only plugin!

Currently only the HMD is handled, controllers coming soon!

License
-------
Please check the Oculus SDK for license information in relation to the Oculus SDK used in this project.
The rest of this project is released under an unlicense.

Compiling
---------
In order to compile this plugin you first need to download the Oculus SDK from the Oculus developer website.
You will also need Visual Studio (tested on 2017), Python 2.x and Scons.

After cloning this repository make sure to initialise the submodules with `git submodule init`
When you've pulled a newer version make sure to run `git submodule update`

Then run `scons oculus=<path to oculus SDK>`
