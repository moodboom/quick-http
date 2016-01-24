#! /bin/sh

# MDM This script bootstraps the code for building on a linux/Unix system.
# Run it without parameters from the nix folder after getting a clean copy of the code.
#
# Afterwards, you can configure and make the executable as usual.
# It is recommended that you do this from the nix folder as follows:
#
#     cd nix
#     ../configure && make      # adjust options as needed
#
# Optionally, you can run this script with a "build" parameter to run a vanilla 
# configure and make as described above.
#
# The script will not bootstrap if the config folder exists, unless you provide 
# a "force" parameter.
#
# WARNING: Once you run this, the top and src folders will be polluted with autotools goo.

if [ ! -d copy_from ]; then
    echo
    echo "You need to run this from the nix folder of your autotools project."
    echo "See other autotools projects to create a new code skeleton."
    echo "At a minimum, you need files in [nix/copy_from/] and [src/]."
    echo
    kill -SIGINT $$;
fi

# We need to work from the top folder to keep autotools happy.
cd ..

# MDM COMMON VARIABLES
export disable_gcc_warnings="-Wno-unused-variable -Wno-unused-local-typedefs -Wno-sign-compare -Wno-deprecated-declarations"
# export CC="gcc-4.9"
if [ "$1" = "release" ] || [ "$2" = "release" ]; then

    # RELEASE
    export LDFLAGS=$RELEASE_LDFLAGS
    export LD_LIBRARY_PATH=$RELEASE_LD_LIBRARY_PATH

elif [ "$1" = "debug" ] || [ "$2" = "debug" ]; then

    # DEBUG
    export LDFLAGS=$DEBUG_LDFLAGS
    export LD_LIBRARY_PATH=$DEBUG_LD_LIBRARY_PATH

fi

if [ "$1" = "force" ] || [ "$2" = "force" ] || [ ! -d config ]; then
    echo "Looks like a first run, welcome!"
    echo "Copying in makefiles, creating config dir, etc..."

    # Clean any crap that may have already been created - Eclipse is good at doing this.
    rm -rf .autotools Makefile* aclocal.m4 autom4te.cache build-* config* src/Makefile*

    # MDM I don't want any nix stuff stored outside the nix folder of the repository.
    # Silly autotools is nearly impossible to use without polluting the source tree though.
    # Here, we copy in the cruft, now that we know that this will be a linux/Unix build.
    cp nix/copy_from/Makefile.am Makefile.am
    cp nix/copy_from/configure.ac .
    cp nix/copy_from/Makefile_src.am src/Makefile.am

    mkdir config
    mkdir m4
    touch NEWS
    touch README
    touch AUTHORS
    touch ChangeLog
    libtoolize -c -f

    # MDM Really hard to change these, fuck it we'll go with eclipse defaults.
    mkdir build-Release
    mkdir build-Debug
fi

autoreconf --force --install -I config -I m4

# build if requested
if [ "$1" = "release" ] || [ "$2" = "release" ] || [ "$3" = "release" ]; then

    # keep the build in the release folder
    echo "Building the project in the build-Release folder..."
    cd build-Release
    make distclean
    make clean
    #../configure CC="$CC" CFLAGS="$disable_gcc_warnings" && make
    ../configure CFLAGS="$disable_gcc_warnings" && make
    cd ..
fi

if [ "$1" = "debug" ] || [ "$2" = "debug" ] || [ "$3" = "debug" ]; then

    # keep the build in the debug folder
    echo "Building the project in the debug folder..."
    cd build-Debug
    make distclean
    make clean
    #../configure CC="$CC" CFLAGS="$disable_gcc_warnings -ggdb3 -O0" CXXFLAGS="-ggdb3 -O0" LDFLAGS="-ggdb3 $DEBUG_LDFLAGS" && make
    ../configure CFLAGS="$disable_gcc_warnings -ggdb3 -O0" CXXFLAGS="-ggdb3 -O0" LDFLAGS="-ggdb3 $DEBUG_LDFLAGS" && make
    cd ..
fi
