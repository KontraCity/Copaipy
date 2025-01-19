cmake_minimum_required(VERSION 3.10)
project(Copaipy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
include_directories("include/")

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Freetype REQUIRED)

add_executable(Copaipy "source/main.cpp"
    # Capture modules
    "source/capture/event.cpp"
    "source/capture/master.cpp"

    # Common modules
    "source/common/astronomy.cpp"
    "source/common/camera.cpp"
    "source/common/config.cpp"
    "source/common/http_server.cpp"
    "source/common/i2c.cpp"
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
    JPEG::JPEG
    Freetype::Freetype
)

if (UNIX)
    find_package(PkgConfig)
    pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)

    include_directories(${LIBCAMERA_INCLUDE_DIRS})
    add_compile_options("-Wall -Wextra -Wpedantic -Wno-psabi")

    target_link_libraries(Copaipy PRIVATE
        "wiringPi"
        "boost_regex"
        "jpeg"
        PkgConfig::LIBCAMERA
    )
endif()
