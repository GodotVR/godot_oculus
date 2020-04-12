#!python
import os

# Reads variables from an optional file.
customs = ['../custom.py']
opts = Variables(customs, ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Define our parameters
opts.Add(EnumVariable('target', "Compilation target", 'release', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('bits', "CPU target", '64', ['32', '64']))
opts.AddVariables(
    PathVariable('oculus_path', 'The path where the oculus SDK is installed.', 'ovr_sdk_win/'),
    PathVariable('target_path', 'The path where the lib is installed.', 'demo/addons/godot-oculus/bin/'),
    PathVariable('target_name', 'The library name.', 'godot_oculus', PathVariable.PathAccept),
)
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", 'no'))

# Updates the environment with the option variables.
opts.Update(env)

# Set paths
godot_glad_path = "glad/"
godot_headers_path = "godot_headers/"
target_path = env['target_path']

# Check some environment settings
if env['use_llvm']:
    env['CXX'] = 'clang++'

    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS = ['-fPIC', '-g3','-Og', '-std=c++17'])
    else:
        env.Append(CCFLAGS = ['-fPIC', '-g','-O3', '-std=c++17'])
else:
    # This makes sure to keep the session environment variables on windows,
    # that way you can run scons in a vs 2017 prompt and it will find all the required tools
    env.Append(ENV = os.environ)

    env.Append(CCFLAGS = ['-DWIN32', '-D_WIN32', '-D_WINDOWS', '-W3', '-GR', '-D_CRT_SECURE_NO_WARNINGS'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS = ['-EHsc', '-D_DEBUG', '/MTd'])
    else:
        env.Append(CCFLAGS = ['-O2', '-EHsc', '-DNDEBUG', '/MT'])

# add our oculus library
env.Append(CPPPATH=[env['oculus_path'] + 'LibOVR/Include'])
if env['bits'] == '64':
    target_path += 'win64/'
    env.Append(LIBPATH=[env['oculus_path'] + 'LibOVR/Lib/Windows/x64/Release/VS2017'])
else:
    target_path += 'win32/'
    env.Append(LIBPATH=[env['oculus_path'] + 'LibOVR/Lib/Windows/Win32/Release/VS2017'])

if (os.name == "nt" and os.getenv("VCINSTALLDIR")):
    env.Append(LINKFLAGS=['LibOVR.lib'])
else:
    env.Append(LIBS=['LibOVR'])

# need to add copying the correct file from 'openvr/bin/' + platform_bin to demo/bin/
# for now manually copy the files

# and our stuff
env.Append(CPPPATH=['.', 'src/', godot_headers_path, godot_glad_path])

sources = Glob('src/*.cpp')
sources += Glob('src/*/*.cpp')
sources.append(godot_glad_path + "/glad.c")

library = env.SharedLibrary(target=target_path + env['target_name'], source=sources)
Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
