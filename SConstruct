#!python
import os, subprocess

# Local dependency paths
godot_glad_path = ARGUMENTS.get("glad", "glad")
godot_headers_path = ARGUMENTS.get("headers", "godot_headers/")
oculus_path = ARGUMENTS.get("oculus", os.getenv("OCULUS_SDK_PATH", "../../ovr_sdk_win_1.35.0/"))

# default to release build, add target=debug to build debug build
target = ARGUMENTS.get("target", "release")

# this really only is supported on windows...
platform = ARGUMENTS.get("platform", "windows")

# This makes sure to keep the session environment variables on windows, 
# that way you can run scons in a vs 2017 prompt and it will find all the required tools
env = Environment()
if platform == "windows":
    env = Environment(ENV = os.environ)

bits = '64'
if 'bits' in env:
    bits = env['bits']

if ARGUMENTS.get("use_llvm", "no") == "yes":
    env["CXX"] = "clang++"

def add_sources(sources, directory):
    for file in os.listdir(directory):
        if file.endswith('.c'):
            sources.append(directory + '/' + file)
        elif file.endswith('.cpp'):
            sources.append(directory + '/' + file)

#if platform == "osx":
#    env.Append(CCFLAGS = ['-g','-O3', '-arch', 'x86_64'])
#    env.Append(LINKFLAGS = ['-arch', 'x86_64'])
#
#if platform == "linux":
#    env.Append(CCFLAGS = ['-fPIC', '-g','-O3', '-std=c++14'])
#    env.Append(CXXFLAGS='-std=c++0x')
#    env.Append(LINKFLAGS = ['-Wl,-R,\'$$ORIGIN\''])

if platform == "windows":
    if target == "debug":
        env.Append(CCFLAGS = ['-EHsc', '-D_DEBUG', '/MTd'])
    else:
        env.Append(CCFLAGS = ['-O2', '-EHsc', '-DNDEBUG', '/MT'])

# add our oculus library
env.Append(CPPPATH=[oculus_path + 'LibOVR/Include'])
if bits == '64':
    env.Append(LIBPATH=[oculus_path + 'LibOVR/Lib/Windows/x64/Release/VS2015'])
else:
    env.Append(LIBPATH=[oculus_path + 'LibOVR/Lib/Windows/Win32/Release/VS2015'])

if (os.name == "nt" and os.getenv("VCINSTALLDIR")):
    env.Append(LINKFLAGS=['LibOVR.lib'])
else:
    env.Append(LIBS=['LibOVR'])

# need to add copying the correct file from 'openvr/bin/' + platform_bin to demo/bin/
# for now manually copy the files

# and our stuff
env.Append(CPPPATH=['.', godot_headers_path, godot_glad_path])

sources = []
add_sources(sources, "src")
sources.append(godot_glad_path + "/glad.c")

library = env.SharedLibrary(target='demo/addons/godot-oculus/bin/win64/godot_oculus', source=sources)
Default(library)
