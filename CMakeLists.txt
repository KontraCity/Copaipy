## --- Project configuration --- ##
cmake_minimum_required(VERSION 3.10)
project(Copaipy)

## --- Compiler configuration --- ##
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
include_directories("include/")

## --- Packages configuration --- ##
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

## --- Executable configuration --- ##
add_executable(Copaipy "source/main.cpp"
    # Capture modules
    "source/capture/event.cpp"
    "source/capture/master.cpp"

    # Common modules
    "source/common/astronomy.cpp"
    "source/common/config.cpp"
    "source/common/http_server.cpp"
    "source/common/i2c.cpp"
    "source/common/stopwatch.cpp"
    "source/common/utility.cpp"

    # Display modules
    "source/display/master.cpp"
    "source/display/ui.cpp"

    # Sensors modules
    "source/sensors/recorder.cpp"
    "source/sensors/sensors.cpp"
)
target_link_libraries(Copaipy PRIVATE
    fmt::fmt
    spdlog::spdlog
)

## --- Raspberry Pi linking configuration --- ##
if (UNIX)
    target_link_libraries(Copaipy PRIVATE "wiringPi")
endif()
