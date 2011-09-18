project(craneweb)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckFunctionExists)

set(CMAKE_REQUIRED_INCLUDES 	stdlib.h)
set(CMAKE_REQUIRED_INCLUDES 	stdint.h)
set(CMAKE_REQUIRED_INCLUDES 	inttypes.h)

if(CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-g -Os -Wall)# -Werror
endif(CMAKE_COMPILER_IS_GNUCC)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

option(ENABLE_BUILTIN_REGEX "Enable the builtin regex library." ON)
if(ENABLE_BUILTIN_REGEX)
    message(STATUS "Enabled the builtin regex library (BSD).")
endif(ENABLE_BUILTIN_REGEX)

option(ENABLE_BUILTIN_MONGOOSE "Enable the builtin web server Mongoose." ON)
if(ENABLE_BUILTIN_MONGOOSE)
    message(STATUS "Enabled the builtin web server: Mongoose.")
endif(ENABLE_BUILTIN_MONGOOSE)


add_subdirectory(src)
add_subdirectory(examples)

