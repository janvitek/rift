message(STATUS "Loading local configuration")


include_directories("/opt/llvm/4.0.0/src/include")


set(LLVM_DIR "/opt/llvm/4.0.0/release/cmake/modules/CMakeFiles")
#set(LLVM_DIR "/opt/llvm/4.0.0/debug/cmake/modules/CMakeFiles")

#add_definitions(-std=c++11 -O2)
add_definitions(-std=c++11 -g)

