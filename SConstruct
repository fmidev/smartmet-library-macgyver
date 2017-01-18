#
# SConstruct for building Macgyver
#
# Usage:
#       scons [-j 4] [-Q] [debug=1|profile=1] [objdir=<path>] smartmet-macgyver.so|lib
#             [windows_boost_path=<path>]
#
# Notes:
#       The three variants (debug/release/profile) share the same output and 
#       object file names;
#       changing from one version to another will cause a full recompile.
#       This is intentional (instead of keeping, say, out/release/...)
#       for compatibility with existing test suite etc. and because normally
#       different machines are used only for debug/release/profile.
#           -- AKa 11-Sep-2008
#
# Windows usage:
#	Run 'vcvars32.bat' or similar before using us, to give right
#	env.var. setups for Visual C++ 2008 command line tools & headers.
#
import os.path, os

Help(""" 
    Usage: scons [-j 4] [-Q] [debug=1|profile=1] [objdir=<path>] smartmet-macgyver.so|lib
    
    Or just use 'make release|debug|profile', which point right back to us.
""") 

Decider("MD5-timestamp") 

DEBUG=      int( ARGUMENTS.get("debug", 0) ) != 0
PROFILE=    int( ARGUMENTS.get("profile", 0) ) != 0
RELEASE=    (not DEBUG) and (not PROFILE)     # default

OBJDIR=     ARGUMENTS.get("objdir","obj")

WINDOWS_BOOST_PATH= ARGUMENTS.get("windows_boost_path","")

env= Environment( )

LINUX=  env["PLATFORM"]=="posix"
OSX=    env["PLATFORM"]=="darwin"
WINDOWS= env["PLATFORM"]=="win32"

env["CC"] = "gcc"
env["CXX"] = "g++"

#
# SCons does not pass env.vars automatically through to executing commands.
# On Windows, we want it to get them all (Visual C++ 2008).
#
if WINDOWS:
    env.Replace( ENV= os.environ )

env.Append( CPPPATH= [ "./include" ] )

if WINDOWS:
    if env["CC"]=="cl":
        env.Append( CXXFLAGS= ["/EHsc"] )
else:
    env.Append( CPPDEFINES= ["UNIX"] )
    env.Append( CXXFLAGS= [
	"-MD",
	"-fdiagnostics-color=always",
        "-fPIC",
	"-Wall",
	"-Wno-float-equal",
	"-Wno-padded",
	"-Wno-missing-noreturn",
	"-Wno-unknown-pragmas",
	"-std=c++0x"
    ] )

#	"-Wno-global-constructors",
#	"-Wno-c++98-compat",
#	"-Wno-c++11-extensions",
#	"-Wno-c++11-long-long",

BOOST_POSTFIX=""
BOOST_PREFIX=""

if WINDOWS:
    # Installed from 'boost_1_35_0_setup.exe' from BoostPro Internet page.
    #
    env.Append( CPPPATH= [ WINDOWS_BOOST_PATH ] )
    env.Append( LIBPATH= [ WINDOWS_BOOST_PATH + "/lib" ] )
    if DEBUG:
        BOOST_POSTFIX= "-vc90-gd-1_35"
    else:
        BOOST_POSTFIX= "-vc90-1_35"
        BOOST_PREFIX= "lib"

elif OSX:
    # Boost from Fink
    #
    env.Append( CPPPATH= [ "/sw/include" ] )
    env.Append( LIBPATH= [ "/sw/lib" ] )

#
# Debug settings
#
if DEBUG:
    if WINDOWS:
        if env["CC"]=="cl":
            env.AppendUnique( CPPDEFINES=["_DEBUG","DEBUG"] )
            # Debug multithreaded DLL runtime, no opt.
            env.AppendUnique( CCFLAGS=["/MDd", "/Od"] )
            # Each obj gets own .PDB so parallel building (-jN) works
            env.AppendUnique( CCFLAGS=["/Zi", "/Fd${TARGET}.pdb"] )
    else:
        env.Append( CXXFLAGS=["-O0", "-g", "-Werror"] )


#
# Release settings
#
if RELEASE or PROFILE:
    if WINDOWS:
        if env["CC"]=="cl":
            # multithreaded DLL runtime, reasonable opt.
            env.AppendUnique( CCFLAGS=["/MD", "/Ox"] )
    else:
        env.Append( CPPDEFINES="NDEBUG",
                    CXXFLAGS= ["-g", "-O2", "-Wuninitialized"]
                  )


#
# Profile settings
#
if PROFILE:
    if WINDOWS: 
        { }     # TBD
    else:
        env.Append( CXXFLAGS="-g -pg" )


objs= []

env.Append( LIBS = [ "boost_filesystem",
                     "boost_thread",
                     "boost_date_time",
                     "boost_regex",
                     "boost_system",
		     "fmt",
                     "ctpp2",
                     "pthread",
                     "rt"
	    ] )
env.ParseConfig("pkg-config --libs icu-i18n");

env.Append( CPPDEFINES="FMI_MULTITHREAD" )
if not WINDOWS:
    env.Append( CPPDEFINES= "_REENTRANT" )

for fn in Glob("source/*.cpp"): 
    s= os.path.basename( str(fn) )
    obj_s= OBJDIR+"/"+ s.replace(".cpp","")
    obj = env.SharedObject( obj_s, fn )
    objs += obj
 
# Make just the static lib

out_postfix= WINDOWS and (DEBUG and "_debug" or "_release") or ""

env.SharedLibrary( "smartmet-macgyver"+out_postfix, objs )
