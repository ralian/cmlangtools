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

# This is a port of bison that works on Windows and has a CML.
# On non-windows we could just use the makefile for bison, or system bison.
# It might actually be entirely unnecessary!
#set(winflexbison_SOURCE_DIR winflexbison)
#FetchContent_Declare(
#    winflexbison EXCLUDE_FROM_ALL SYSTEM
#    GIT_REPOSITORY  https://github.com/lexxmark/winflexbison.git
#    GIT_TAG         v2.5.25
#)

FetchContent_MakeAvailable(cmakesrc)
FetchContent_GetProperties(cmakesrc BINARY_DIR cmakesrc_BUILD_DIR)

# For now, we don't want to build/link all of CMake, but might have to later for stability
add_executable(cmlangtools 
    ${cmakesrc_SOURCE_DIR}/Source/cmExprParserHelper.cxx
    ${cmakesrc_SOURCE_DIR}/Source/cmStringAlgorithms.cxx
    ${cmakesrc_SOURCE_DIR}/Source/kwsys/Directory.cxx
    ${cmakesrc_SOURCE_DIR}/Source/kwsys/EncodingC.c
    ${cmakesrc_SOURCE_DIR}/Source/kwsys/EncodingCXX.cxx
    ${cmakesrc_SOURCE_DIR}/Source/kwsys/RegularExpression.cxx
    ${cmakesrc_SOURCE_DIR}/Source/kwsys/Status.cxx
    ${cmakesrc_SOURCE_DIR}/Source/kwsys/SystemTools.cxx
    ${cmakesrc_SOURCE_DIR}/Source/LexerParser/cmExprLexer.cxx
    ${cmakesrc_SOURCE_DIR}/Source/LexerParser/cmExprParser.cxx
    ${cmakesrc_SOURCE_DIR}/Source/LexerParser/cmListFileLexer.c
    cmlangtools.cpp
)
target_compile_definitions(cmlangtools PRIVATE
    KWSYS_ENCODING_DEFAULT_CODEPAGE=CP_UTF8
    KWSYS_NAMESPACE=cmsys
)

# Add include directories
target_include_directories(cmlangtools PRIVATE
    ${cmakesrc_SOURCE_DIR}/Source/LexerParser
    ${cmakesrc_SOURCE_DIR}/Source
    ${cmakesrc_SOURCE_DIR}/Utilities
    ${cmakesrc_BUILD_DIR}/Source
    ${cmakesrc_BUILD_DIR}/Utilities
)

target_include_directories(cmlangtools PRIVATE SYSTEM
    ${cmakesrc_SOURCE_DIR}/Utilities/std
)
