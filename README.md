# Installation

The following guide assumes Linux machine (tested on Ubuntu 16.04 LTS). 

For Windows user, enable the Windows Subsystem for Linux and then run the commands below. 

> TODO check this on apple? 

## Prerequisites

    sudo apt-get install cmake g++ git  ninja-build subversion

## LLVM Setup

    svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_400/final/ src
    mkdir release
    mkdir debug

Now to build the release version of LLVM, execute the following from the root directory:

    cd release
    cmake -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Release" ../src
    make

And the debug build as well:

    cd debug
    cmake -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Debug" --enable-debug-symbols --with-oprofile ../src
    make

## Rift

In the `rift` directory, first go to the `local` folder and do the following:

    cp cmake.cmake.default cmake.cmake

Then open `cmake.cmake` and update the `LLVM_COMPILE_DIR` to point to your directory where you compiled llvm in (the one with `src`, `debug` and `release` subdirectories from previous step. You may also change the `LLVM_LINK_AGAINST` to your preferred LLVM build type (`"Debug"` or `"Release"`).

When done, execute the following from the `rift` folder:

    mkdir build
    cd build
    cmake ..
    make

When make finishes, you can run rift by typing:

    ./rift
