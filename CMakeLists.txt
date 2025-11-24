cmake_minimum_required(VERSION 3.24)
project(cmlangtools LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Yo dawg I heard you like CMake (can't use cmake_SOURCE_DIR)
set(cmakesrc_SOURCE_DIR cmakesrc)
FetchContent_Declare(
    cmakesrc EXCLUDE_FROM_ALL SYSTEM
    GIT_REPOSITORY  https://github.com/Kitware/CMake.git
    GIT_TAG         v4.1.2
)

FetchContent_MakeAvailable(cmakesrc)
FetchContent_GetProperties(cmakesrc BINARY_DIR cmakesrc_BUILD_DIR)

# For now, we don't want to build/link all of CMake, but might have to later for stability
add_executable(cmlangtools cmlangtools.cpp)
target_link_libraries(cmlangtools PRIVATE CMakeLib)

# Add include directories
target_include_directories(cmlangtools PRIVATE
    ${cmakesrc_SOURCE_DIR}/Source/LexerParser
    ${cmakesrc_SOURCE_DIR}/Source
    ${cmakesrc_SOURCE_DIR}/Utilities
    ${cmakesrc_SOURCE_DIR}/Utilities/std
    ${cmakesrc_BUILD_DIR}/Source
    ${cmakesrc_BUILD_DIR}/Utilities
)
