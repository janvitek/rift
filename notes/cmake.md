# `cmake` Crash Course

Unlike traditional build systems (`make`, etc.), [`cmake`](www.cmake.org) is not really a build system of its own, but rather a generator for various existing build systems on different platforms (`make`, `ninja`, `msbuild`, etc.).

The definition of the projects to be built resides in file called `CMakeLists.txt`. This file lists all prerequisites, libraries and executables that should be built and describes how to build them (include paths, libraries to link to, etc.).

When `cmake` is invoked, it must be supplied the path to this file. It reads the descriptions and generates the build instructions for the project in the directory it is called from (thus allowing for out of tree builds). The target build system is automatically selected based on the platform and other circumstances, but can be overriden by changing the generator, `-G` argument. Additional arguments can be passed to `cmake` as well at this time, as the `llvm`’s cmake command line indicates, specifying the debug type, enabling backends and RTTI.

Once the build instructions are generated, one can either use the preferred target build system (`make`, `ninja`, etc.) directly on the generated files, or by calling `cmake --build path_to_build_dir` let cmake invoke the proper tools itself.

The `--target` install argument with llvm’s build instructs `cmake` to also install the built project, if any installation steps are specified (an analogy to `make install` command).
