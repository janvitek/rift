# Installation

The following guide assumes Linux machine (tested on Ubuntu 16.04 LTS). 

For Windows user, enable the Windows Subsystem for Linux and then run the commands below. 

> TODO check this on apple? 

## Prerequisites

    sudo apt-get install cmake g++ git  ninja-build subversion
    mkdir pliss
    cd pliss
    mkdir llvm

## LLVM Setup

We now have to download LLVM and build at least *release* version of LLVM we can link rift against. The following commands download LLVM into the `src` and create a directory for the release build. Inside that directory, we run `cmake` to build LLVM in the release configuration:    

    cd llvm
    svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_400/final/ src
    mkdir release
    cd release
    cmake -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Release" ../src
    make
    cd ..

Optionally, you can build a *debug* version as well. This version will take longer time to link with, but will produce better errors and debugging for LLVM related bugs: 

    mkdir debug
    cd debug
    cmake -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Debug" --enable-debug-symbols --with-oprofile ../src
    make
    cd ..

## Rift

Now download *rift*:

    git clone WHERE IS RIFT LOCATED
    cd rift
    mkdir build
    cd build
    cmake ..
    make


When make finishes, you can run rift by typing:

    ./rift
