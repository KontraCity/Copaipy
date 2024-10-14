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
#include <fmt/xchar.h>

// Library CImg
#define cimg_display 0
#define cimg_use_jpeg 1
#include "external/CImg.h"

// Library FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#include "external/font.hpp"

// Custom modules
#include "common/utility.hpp"
#include "sensors/recorder.hpp"

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

    namespace Ui
    {
        constexpr uint8_t BlackColor[] = { 0, 0, 0 };
        constexpr int InfoBarHeight = 160;
        constexpr int SideMargin = 20;

        constexpr int SmallTextOffset = 20;
        constexpr int SmallTextSize = 50;
        constexpr int BigTextOffset = 65;
        constexpr int BigTextSize = 80;
    }
}

class Camera
{
public:
    // Image captured by the camera
    using Image = cimg::CImg<uint8_t>;

    // Info shown by camera UI
    struct UiInfo
    {
        std::string task;
        Sensors::Recorder::Record record;
        Sensors::Recorder::Record trend;
    };

private:
    /// @brief Create text sprite
    /// @param string Text string
    /// @param height Text height
    /// @throw std::runtime_error if internal error occurs
    /// @return Created text sprite
    static Image CreateText(const std::u32string& string, uint32_t height);

    /// @brief Get trend symbol
    /// @param trend The trend to get trend symbol for
    /// @return Trend symbol
    static char32_t TrendSymbol(double trend);

    /// @brief Create measurement string
    /// @param measurement The measurement
    /// @param trend Measurement trend
    /// @return Measurement string
    static std::u32string CreateMeasurementString(const std::optional<Sensors::Measurement>& measurement, const std::optional<Sensors::Measurement>& trend);

    /// @brief Draw camera UI to image
    /// @param image The image to draw to
    /// @param info UI info to draw
    static void DrawUi(Image& image, const UiInfo& info);

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

    /// @brief Capture an image with camera UI
    /// @param info UI info to include in image
    /// @throw std::invalid_argument if camera is not on
    /// @throw std::runtime_error if internal error occurs
    /// @return Captured image with camera UI
    Image capture(const UiInfo& info);
};

} // namespace kc
