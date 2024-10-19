cmake_minimum_required(VERSION 3.10)

project(fs VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # generate compile_commands.json
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_definitions(FUSE_USE_VERSION=31)
add_compile_definitions(DEBUG)

# setup fuse
set(FUSE_INCLUDE_DIR /usr/local/include/fuse3)
set(FUSE_LIBRARIES /usr/local/lib/x86_64-linux-gnu/libfuse3.so)

file(GLOB_RECURSE LIB_SOURCES ${CMAKE_SOURCE_DIR}/lib/*.cpp)
add_library(fslib STATIC ${LIB_SOURCES})

add_executable(fs ${CMAKE_SOURCE_DIR}/src/main.cpp)
include_directories(${CMAKE_SOURCE_DIR}/include ${FUSE_INCLUDE_DIR})
target_link_libraries(fs fslib ${FUSE_LIBRARIES})
