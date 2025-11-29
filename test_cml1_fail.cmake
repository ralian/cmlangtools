cmake_minimum_required(VERSION 3.24)
project(test_cml1_fail LANGUAGES CXX)

add_library(lib0)
add_library(lib1)
add_library(lib2)

target_link_libraries(lib0 lib1 lib2)
