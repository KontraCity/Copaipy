#include "common/camera.hpp"
using namespace kc::CameraConst;

namespace kc {

Camera::Image Camera::CreateText(const std::u32string& string, uint32_t height)
{
    static bool initialized = false;
    static FT_Library library;
    static FT_Face face;
    if (!initialized)
    {
        FT_Error result = FT_Init_FreeType(&library);
        if (result)
            throw std::runtime_error(fmt::format("kc::Camera::CreateText(): Couldn't initialize FreeType library [result: {}]", result));

        result = FT_New_Memory_Face(library, Font::CascadiaCode.data(), static_cast<FT_Long>(Font::CascadiaCode.size()), 0, &face);
        if (result)
            throw std::runtime_error(fmt::format("kc::Camera::CreateText(): Couldn't create new memory face [result: {}]", result));
        initialized = true;
    }

    FT_Error result = FT_Set_Pixel_Sizes(face, 0, height * face->height / (face->height - face->descender));
    if (result)
        throw std::runtime_error(fmt::format("kc::Camera::CreateText(): Couldn't set pixel sizes [result: {}]", result));
    height = (face->size->metrics.height) / 64;

    Camera::Image text;
    int offset = 0;
    for (char32_t character : string)
    {
        FT_UInt characterIndex = FT_Get_Char_Index(face, static_cast<FT_ULong>(character));
        if (!characterIndex)
            throw std::runtime_error(fmt::format("kc::Camera::CreateText(): Couldn't get character index [character: {}]", static_cast<int>(character)));

        result = FT_Load_Glyph(face, characterIndex, FT_LOAD_DEFAULT);
        if (result)
            throw std::runtime_error(fmt::format("kc::Camera::CreateText(): Couldn't load character glyph [result: {}]", result));

        result = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (result)
            throw std::runtime_error(fmt::format("kc::Camera::CreateText(): Couldn't render character glyph [result: {}]", result));

        int width = face->glyph->advance.x >> 6;
        if (text.is_empty())
        {
            text.assign(width, height);
            text.draw_rectangle(0, 0, text.width(), text.height(), Ui::BlackColor);
        }
        else
        {
            text.crop(0, offset + width);
            text.draw_rectangle(offset, 0, text.width(), text.height(), Ui::BlackColor);
        }

        Camera::Image bitmap(face->glyph->bitmap.buffer, face->glyph->bitmap.width, face->glyph->bitmap.rows);
        text.draw_image(offset + face->glyph->bitmap_left, height - face->glyph->bitmap_top + (face->size->metrics.descender / 64), bitmap);
        offset += width;
    }

    text.resize(text.width(), text.height(), 1, 3);
    return text;
}

char32_t Camera::TrendSymbol(double trend)
{
    if (trend > 1.0)
        return U'↑';
    if (trend > 0.3)
        return U'⇡';
    if (trend >= -0.3)
        return U'~';
    if (trend >= -1.0)
        return U'⇣';
    return U'↓';
}

std::u32string Camera::CreateMeasurementString(const std::optional<Sensors::Measurement>& measurement, const std::optional<Sensors::Measurement>& trend)
{
    if (!measurement)
        return U"×××.×°C× ×××.×%× ××××.×hPa×";

    if (!trend)
    {
        return fmt::format(
            U"{:>+5.1f}°C× {:>5.1f}%× {:>6.1f}hPa×",
            measurement->bmp280.temperature,
            measurement->aht20.humidity,
            measurement->bmp280.pressure
        );
    }

    return fmt::format(
        U"{:>+5.1f}°C{} {:>5.1f}%{} {:>6.1f}hPa{}",
        measurement->bmp280.temperature,
        TrendSymbol(trend->bmp280.temperature),
        measurement->aht20.humidity,
        TrendSymbol(trend->aht20.humidity),
        measurement->bmp280.pressure,
        TrendSymbol(trend->bmp280.pressure)
    );
}

void Camera::DrawUi(Image& image, const UiInfo& info)
{
    image.draw_rectangle(0, image.height() - Ui::InfoBarHeight, image.width(), image.height(), Ui::BlackColor, 1.0);
    image.draw_image(Ui::SideMargin, image.height() - Ui::InfoBarHeight + Ui::SmallTextOffset, CreateText(U"External", Ui::SmallTextSize));
    image.draw_image(Ui::SideMargin, image.height() - Ui::InfoBarHeight + Ui::BigTextOffset, CreateText(CreateMeasurementString(info.record.external, info.trend.external), Ui::BigTextSize));

    Image taskText = CreateText(fmt::format(U"[{}] ", std::u32string(info.task.begin(), info.task.end())), Ui::BigTextSize);
    Image timestampText = CreateText(fmt::format(
        U"{:#02d}.{:#02d}.{:#04d} {:#02d}:{:#02d}:{:#02d}",
        static_cast<int>(info.record.timestamp.date().day()),
        info.record.timestamp.date().month().as_number(),
        static_cast<int>(info.record.timestamp.date().year()),
        info.record.timestamp.time_of_day().hours(),
        info.record.timestamp.time_of_day().minutes(),
        info.record.timestamp.time_of_day().seconds()
    ), Ui::BigTextSize);
    int totalWidth = taskText.width() + timestampText.width();
    image.draw_image(image.width() / 2 - totalWidth / 2, image.height() - Ui::InfoBarHeight + Ui::SmallTextOffset, CreateText(U"Task", Ui::SmallTextSize));
    image.draw_image(image.width() / 2 - totalWidth / 2, image.height() - Ui::InfoBarHeight + Ui::BigTextOffset, taskText);
    image.draw_image(image.width() / 2 - totalWidth / 2 + taskText.width(), image.height() - Ui::InfoBarHeight + Ui::SmallTextOffset, CreateText(U"Timestamp", Ui::SmallTextSize));
    image.draw_image(image.width() / 2 - totalWidth / 2 + taskText.width(), image.height() - Ui::InfoBarHeight + Ui::BigTextOffset, timestampText);

    Image text = CreateText(U"Internal", Ui::SmallTextSize);
    image.draw_image(image.width() - Ui::SideMargin - text.width(), image.height() - Ui::InfoBarHeight + Ui::SmallTextOffset, text);
    text = CreateText(CreateMeasurementString(info.record.external, info.trend.external), Ui::BigTextSize);
    image.draw_image(image.width() - Ui::SideMargin - text.width(), image.height() - Ui::InfoBarHeight + Ui::BigTextOffset, text);
}

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
    streamConfig.size.width = CaptureWidth;
    streamConfig.size.height = CaptureHeight;
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
    return Image(CaptureWidth, CaptureHeight, 1, 3);
#endif
}

Camera::Image Camera::capture(const UiInfo& info)
{
    Image image = capture();
    DrawUi(image, info);
    return image;
}

} // namespace kc
