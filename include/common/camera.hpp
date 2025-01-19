#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>

#ifdef __unix__
    #include <libcamera/libcamera.h>
    namespace lc = libcamera;
#endif

#define cimg_display 0
#define cimg_use_jpeg 1
#include "external/CImg.h"
namespace cimg = cimg_library;

#include <spdlog/spdlog.h>

#include "sensors/recorder.hpp"

namespace cp {

namespace CameraConst {
    constexpr int CaptureWidth = 4056;
    constexpr int CaptureHeight = 3040;

    namespace Ui {
        constexpr uint8_t BlackColor[] = { 0, 0, 0 };
        constexpr int InfoBarHeight = 160;
        constexpr int SideMargin = 20;

        constexpr int SmallTextOffset = 20;
        constexpr int SmallTextSize = 50;
        constexpr int BigTextOffset = 65;
        constexpr int BigTextSize = 80;
    }
}

class Camera {
public:
    using Image = cimg::CImg<uint8_t>;

    struct UiInfo {
        std::string task;
        Sensors::Recorder::Record record;
        Sensors::Recorder::Record trend;
    };

private:
    spdlog::logger m_logger;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    bool m_on = false;
    Image* m_image = nullptr;

#ifdef __unix__
    std::unique_ptr<lc::CameraManager> m_manager;
    std::shared_ptr<lc::Camera> m_camera;
    std::unique_ptr<lc::CameraConfiguration> m_cameraConfig;
    std::unique_ptr<lc::FrameBufferAllocator> m_allocator;
#endif

public:
    Camera();

    ~Camera();

private:
#ifdef __unix__
    void requestCompleted(lc::Request* request);
#endif

public:
    void turnOn();

    void turnOff();

    Image capture();

    Image capture(const UiInfo& info);
};

} // namespace cp
