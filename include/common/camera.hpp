#pragma once

// STL modules
#include <memory>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#ifdef __unix__
    // UNIX modules
    #include <sys/mman.h>

    // Library camera
    #include <libcamera/libcamera.h>
#endif

// Library {fmt}
#include <fmt/format.h>

// Library CImg
#define cimg_display 0
#define cimg_use_jpeg 1
#include "external/CImg.h"

// Custom modules
#include "common/utility.hpp"

namespace kc {

/* Namespace aliases and imports */
#ifdef __unix__
    namespace lc = libcamera;
#endif
namespace cimg = cimg_library;

namespace CameraConst
{
    constexpr int CaptureWidth = 4056;
    constexpr int CaptureHeight = 3040;
}

class Camera
{
public:
    // Image captured by the camera
    using Image = cimg::CImg<uint8_t>;

private:
    spdlog::logger m_logger;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    bool m_on;
#ifdef __unix__
    std::unique_ptr<lc::CameraManager> m_manager;
    std::shared_ptr<lc::Camera> m_camera;
    std::unique_ptr<lc::CameraConfiguration> m_cameraConfig;
    std::unique_ptr<lc::FrameBufferAllocator> m_allocator;
#endif
    Image* m_image;

private:
#ifdef __unix__
    /// @brief Libcamera request completed callback
    /// @param request The request that was completed
    void requestCompleted(lc::Request* request);
#endif

public:
    /// @brief Initialize the camera
    Camera();

    ~Camera();

    /// @brief Turn the camera on
    /// @throw std::runtime_error if internal error occurs
    void turnOn();

    /// @brief Turn the camera off
    void turnOff();

    /// @brief Capture an image
    /// @throw std::invalid_argument if camera is not on
    /// @throw std::runtime_error if internal error occurs
    /// @return Captured image
    Image capture();
};

} // namespace kc
