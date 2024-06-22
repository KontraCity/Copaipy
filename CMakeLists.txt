## --- Project configuration --- ##
cmake_minimum_required(VERSION 3.10)
project(Copaipy)

## --- Compiler configuration --- ##
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
include_directories("include/")

## --- Packages configuration --- ##
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

## --- Executable configuration --- ##
add_executable(Copaipy "source/main.cpp"
    # Common modules
    "source/common/config.cpp"
    "source/common/i2c.cpp"
    "source/common/sensors.cpp"
    "source/common/utility.cpp"

    # Display modules
    "source/display/master.cpp"
    "source/display/ui.cpp"
)
target_link_libraries(Copaipy PRIVATE
    fmt::fmt
    spdlog::spdlog
    "wiringPi"
)
