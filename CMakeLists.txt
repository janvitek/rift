cmake_minimum_required(VERSION 2.8.8)

project(rift)

#SET(CMAKE_C_COMPILER "/usr/bin/clang-3.5")
#SET(CMAKE_CXX_COMPILER "/usr/bin/clang++-3.5")


# set LLVM

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

if(${CMAKE_CXX_COMPILER_ID} EQUAL "MSVC")

else()
    add_definitions(-Wall -std=c++11 -Wno-unknown-pragmas)
endif()

# TODO for now, make all builds debug
set(CMAKE_BUILD_TYPE Debug)


# build the shared library for the JIT
file(GLOB SRC "*.cpp" "*.h" "tests/*.ri")
add_executable(${PROJECT_NAME} ${SRC})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core mcjit native irreader linker ipo)

# Link against LLVM libraries
target_link_libraries(${PROJECT_NAME} ${llvm_libs})
