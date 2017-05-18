# Installation

The following guide assumes Linux machine (Ubuntu 16.04 LTS). For Windows, enable the "Windows Subsystem for Linux" and then run the commands below.

> If you have opted for the virtual machine, you do not have to do the LLVM setup as the `pliss\llvm` directory has been created for you, and LLVM in release and debug versions has been already built so you can continue with the *rift* chapter. 

## Prerequisites

    sudo apt-get install cmake g++ git  ninja-build subversion
    mkdir pliss
    cd pliss
    mkdir llvm

## LLVM Setup

We now download LLVM and build at least *release* version of LLVM. The following commands download LLVM into the `src` and create a directory for the release build. Inside that directory, we run `cmake` to build LLVM in the release configuration:   

To speed up the build, you can use `make --jobs NUM` where num is number cores. 1.5GB RAM/thread is required, too many cores might run out of memory.

    cd llvm
    svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_400/final/ src
    mkdir release
    cd release
    cmake -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Release" ../src
    make
    cd ..

Optionally, you can build a *debug* version. This version takes longer time to link, but  produces better errors and helps debugging.

    mkdir debug
    cd debug
    cmake -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Debug" --enable-debug-symbols --with-oprofile ../src
    make
    cd ..
    
And finally, return back to the `pliss` directory to install rift;

    cd ..

## Rift

Now download rift:

    git clone WHERE IS RIFT LOCATED
    cd rift
    mkdir build
    cd build
    cmake ..
    make


When make finishes, you can run rift by typing:

    ./rift
    
## Possible problems

The directory structure in the `pliss` folder should be the following:


    pliss
        llvm
            src
            release
            debug
        rift
 
 If the directory structure is different, LLVM would not be found by the build script. 

        
        
         
