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
    cmake -G "Ninja" -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Release" ../src
    ninja

And the debug build as well:

    cd debug
    cmake -G "Ninja" -DLLVM_OPTIMIZED_TABLEGEN=1 -DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE="Debug" --enable-debug-symbols --with-oprofile ../src
    ninja

## Rift

In the `rift` directory, first go to the `local` folder









# rift
Playing with llvm

## Instructions

https://docs.google.com/document/d/1Tr5mWd9TdeP4fdBMk2LUkEofbEp1M6G9Azy6OPTuAK0
