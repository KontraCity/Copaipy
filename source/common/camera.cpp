#include "common/camera.hpp"

namespace kc {

#ifdef __unix__
void Camera::requestCompleted(lc::Request* request)
{
    if (request->status() == lc::Request::RequestCancelled)
    {
        m_cv.notify_all();
        return;
    }

    const lc::FrameBuffer::Plane& plane = request->buffers().begin()->second->planes().at(0);
    void* buffer = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
    if (buffer == MAP_FAILED)
    {
        m_logger.error("Image capture failed: Couldn't map data buffer [errno: {}]", errno);
        return;
    }

    const lc::StreamConfiguration& streamConfig = m_cameraConfig->at(0);
    unsigned int calculatedWidth = plane.length / 3 / streamConfig.size.height;
    m_image->assign(reinterpret_cast<uint8_t*>(buffer), 3, calculatedWidth, streamConfig.size.height, 1);
    m_image->permute_axes("yzcx");
    if (streamConfig.size.width != calculatedWidth)
        m_image->crop(0, streamConfig.size.width - 1);

    int result = munmap(buffer, plane.length);
    if (result == -1)
    {
        m_logger.error("Image capture failed: Couldn't unmap data buffer [errno: {}]", errno);
        return;
    }
    m_cv.notify_all();
}
#endif

Camera::Camera()
    : m_logger(Utility::CreateLogger("camera"))
    , m_on(false)
    , m_image(nullptr)
{
#ifdef __unix__
    lc::logSetLevel("RPI", "ERROR");
    lc::logSetLevel("RPiSdn", "ERROR");
    lc::logSetLevel("Camera", "ERROR");
#endif
}

Camera::~Camera()
{
    turnOff();
}

void Camera::turnOn()
{
    std::lock_guard lock(m_mutex);
    if (m_on)
        return;

#ifdef __unix__
    std::unique_ptr<lc::CameraManager> manager = std::make_unique<lc::CameraManager>();
    manager->start();
    if (manager->cameras().empty())
        throw std::runtime_error("kc::Camera::turnOn(): No cameras detected");

    if (manager->cameras().size() > 1)
        throw std::runtime_error("kc::Camera::turnOn(): Multiple cameras detected (ambiguous)");
    std::shared_ptr<lc::Camera> camera = manager->cameras().at(0);
    camera->requestCompleted.connect(this, &Camera::requestCompleted);

    int result = camera->acquire();
    if (result < 0)
        throw std::runtime_error(fmt::format("kc::Camera::turnOn(): Couldn't acquire camera [result: {}]", result));

    std::unique_ptr<lc::CameraConfiguration> cameraConfig = camera->generateConfiguration({ lc::StreamRole::Raw });
    if (!cameraConfig)
    {
        camera->release();
        throw std::runtime_error("kc::Camera::turnOn(): Couldn't generate camera configuration");
    }

    lc::StreamConfiguration& streamConfig = cameraConfig->at(0);
    streamConfig.size.width = CameraConst::CaptureWidth;
    streamConfig.size.height = CameraConst::CaptureHeight;
    streamConfig.pixelFormat = lc::formats::BGR888;
    if (cameraConfig->validate() == lc::CameraConfiguration::Status::Invalid)
    {
        camera->release();
        throw std::runtime_error(fmt::format("kc::Camera::turnOn(): Couldn't validate stream config \"{}\"", streamConfig.toString()));
    }

    result = camera->configure(cameraConfig.get());
    if (result < 0)
    {
        camera->release();
        throw std::runtime_error(fmt::format("kc::Camera::turnOn(): Couldn't configure camera [result: {}]", result));
    }

    std::unique_ptr<lc::FrameBufferAllocator> allocator = std::make_unique<lc::FrameBufferAllocator>(camera);
    result = allocator->allocate(streamConfig.stream());
    if (result < 0)
    {
        camera->release();
        throw std::runtime_error(fmt::format("kc::Camera::turnOn(): Couldn't allocate frame buffer [result: {}]", result));
    }

    m_manager = std::move(manager);
    m_camera = std::move(camera);
    m_cameraConfig = std::move(cameraConfig);
    m_allocator = std::move(allocator);
#endif
    m_on = true;
}

void Camera::turnOff()
{
    std::lock_guard lock(m_mutex);
    if (!m_on)
        return;

#ifdef __unix__
    m_allocator->free(m_cameraConfig->at(0).stream());
    m_allocator.reset();
    m_cameraConfig.reset();
    m_camera->release();
    m_camera.reset();
    m_manager.reset();
#endif
    m_on = false;
}

Camera::Image Camera::capture()
{
    std::unique_lock lock(m_mutex);
    if (!m_on)
        throw std::invalid_argument("kc::Camera::capture(): Camera is not on");;

#ifdef __unix__
    std::unique_ptr<lc::Request> request = m_camera->createRequest();
    if (!request)
        throw std::runtime_error("kc::Camera::capture(): Couldn't create capture request");

    lc::Stream* stream = m_cameraConfig->at(0).stream();
    int result = request->addBuffer(stream, m_allocator->buffers(stream).at(0).get());
    if (result < 0)
        throw std::runtime_error(fmt::format("kc::Camera::capture(): Couldn't add buffer to capture request [result: {}]", result));

    result = m_camera->start();
    if (result < 0)
        throw std::runtime_error(fmt::format("kc::Camera::capture(): Couldn't start camera [result: {}]", result));

    Image image;
    m_image = &image;

    result = m_camera->queueRequest(request.get());
    if (result < 0)
        throw std::runtime_error(fmt::format("kc::Camera::capture(): Couldn't queue capture request [result: {}]", result));

    m_cv.wait(lock);
    result = m_camera->stop();
    if (result < 0)
        throw std::runtime_error(fmt::format("kc::Camera::capture(): Couldn't stop camera [result: {}]", result));

    m_image = nullptr;
    return image;
#else
    return Image(CameraConst::CaptureWidth, CameraConst::CaptureHeight, 1, 3);
#endif
}

} // namespace kc
