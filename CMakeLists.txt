cmake_minimum_required(VERSION 3.4.3)

project(rift)

# see if there is local configuration available

include(${CMAKE_SOURCE_DIR}/local/cmake.cmake OPTIONAL)

# setup LLVM

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Now build our tools

include_directories(${CMAKE_SOURCE_DIR}/src)

add_definitions(-Wall -std=c++11 -Wno-unknown-pragmas)
file(GLOB SRC
    "src/*.cpp"
    "src/*.h"
    "src/**/*.cpp"
    "src/**/*.h"
    "README.md"
    "LICENSE"
    "notes/*"
)
add_executable(${PROJECT_NAME} ${SRC})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs core support mcjit native irreader linker ipo executionengine runtimedyld)
#llvm_map_components_to_libnames(llvm_libs all)
# Link against LLVM libraries
target_link_libraries(${PROJECT_NAME} ${llvm_libs})
message(STATUS "LLVM libs detected: ${llvm_libs}")
