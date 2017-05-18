# Installation

The following guide assumes Linux machine (Ubuntu 16.04 LTS). For Windows, enable the "Windows Subsystem for Linux" and then run the commands below. 

## Prerequisites

    sudo apt-get install cmake g++ git  ninja-build subversion
    mkdir pliss
    cd pliss
    mkdir llvm

## LLVM Setup

We now download LLVM and build at least *release* version of LLVM. The following commands download LLVM into the `src` and create a directory for the release build. Inside that directory, we run `cmake` to build LLVM in the release configuration:   

To speed up buil, you can use `make --jobs NUM` where num is number cores. 1.5GB RAM/thread is required, too many cores might run out of memory.

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
