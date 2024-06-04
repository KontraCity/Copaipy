## --- Project configuration --- ##
cmake_minimum_required(VERSION 3.10)
project(Copaipy)

## --- Compiler configuration --- ##
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories("include/")

## --- Packages configuration --- ##
find_package(fmt CONFIG REQUIRED)

## --- Executable configuration --- ##
add_executable(Copaipy "source/main.cpp")
target_link_libraries(Copaipy fmt::fmt)
