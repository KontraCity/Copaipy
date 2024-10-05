#include "common/http_server.hpp"

namespace kc {

int HttpServer::Connection::GetIndentation(const std::string& query)
{
    if (query.find("pretty=true") == std::string::npos)
        return -1;
    return 4;
}

HttpServer::Connection::Target HttpServer::Connection::ParseTarget(const std::string& target)
{
    size_t queryStartPosition = target.find("?");
    if (queryStartPosition == std::string::npos)
        return { target, {} };
    return { target.substr(0, queryStartPosition), std::string(target.begin() + queryStartPosition + 1, target.end()) };
}

int HttpServer::Connection::GetItemsCount(const std::string& query)
{
    boost::smatch matches;
    if (!boost::regex_search(query, matches, boost::regex(R"(count=(\d+))")))
        return -1;

    try
    {
        int count = std::stoi(matches.str(1));
        if (count <= 0)
            return -1;
        return count;
    }
    catch (...)
    {
        // Number is too big or to small
        return -1;
    }
}

HttpServer::Connection::HistoryFields HttpServer::Connection::GetHistoryFields(const std::string& query)
{
    boost::smatch matches;
    if (!boost::regex_search(query, matches, boost::regex(R"(fields=["']?([\w|]+)["']?)")))
        return {};

    HistoryFields fields = { false, false, false, false };
    std::stringstream stream(matches.str(1));
    std::string string;
    while (std::getline(stream, string, '|'))
    {
        if (string == "temperature")
            fields.temperature = true;
        if (string == "alternative")
            fields.alternative = true;
        if (string == "humidity")
            fields.humidity = true;
        if (string == "pressure")
            fields.pressure = true;
    }
    return fields;
}

void HttpServer::Connection::notFound()
{
    m_response.result(beast::http::status::not_found);
    m_response.set(beast::http::field::content_type, "text/plain");
    beast::ostream(m_response.body()) << "Resource not found\n";
    m_logger->error(m_logMessage("Not Found"));
}

void HttpServer::Connection::methodNotAllowed()
{
    m_response.result(beast::http::status::method_not_allowed);
    m_response.set(beast::http::field::content_type, "text/plain");
    beast::ostream(m_response.body()) << "This method is not allowed\n";
    m_logger->error(m_logMessage("Method Not Allowed"));
}

void HttpServer::Connection::getSensors(Sensors::Location location, int indentation)
{
    Sensors::Recorder::Record record = Sensors::Recorder::Instance->last();
    const auto& measurement = (location == Sensors::Location::Internal ? record.internal : record.external);
    if (!measurement)
    {
        json responseJson;
        responseJson["_success"] = false;
        responseJson["what"] = "Sorry, something went wrong: measurement couldn't be done.";

        m_response.result(beast::http::status::internal_server_error);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->error(m_logMessage("Internal Server Error"));
        return;
    }

    json aht20Object;
    aht20Object["temperature"] = measurement->aht20.temperature;
    aht20Object["humidity"] = measurement->aht20.humidity;

    json bmp280Object;
    bmp280Object["temperature"] = measurement->bmp280.temperature;
    bmp280Object["pressure"] = measurement->bmp280.pressure;

    json responseJson;
    responseJson["_success"] = true;
    responseJson["aht20"] = aht20Object;
    responseJson["bmp280"] = bmp280Object;

    m_response.result(beast::http::status::ok);
    m_response.set(beast::http::field::content_type, "application/json");
    beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
    m_logger->info(m_logMessage("OK"));
}

void HttpServer::Connection::getTrend(Sensors::Location location, int indentation)
{
    Sensors::Recorder::Record trend = Sensors::Recorder::Instance->trend();
    const auto& measurement = (location == Sensors::Location::Internal ? trend.internal : trend.external);
    if (!measurement)
    {
        json responseJson;
        responseJson["_success"] = false;
        responseJson["what"] = "Sorry, something went wrong: trend couldn't be calculated.";

        m_response.result(beast::http::status::internal_server_error);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->error(m_logMessage("Internal Server Error"));
        return;
    }

    json aht20Object;
    aht20Object["temperature"] = measurement->aht20.temperature;
    aht20Object["humidity"] = measurement->aht20.humidity;

    json bmp280Object;
    bmp280Object["temperature"] = measurement->bmp280.temperature;
    bmp280Object["pressure"] = measurement->bmp280.pressure;

    json responseJson;
    responseJson["_success"] = true;
    responseJson["aht20"] = aht20Object;
    responseJson["bmp280"] = bmp280Object;

    m_response.result(beast::http::status::ok);
    m_response.set(beast::http::field::content_type, "application/json");
    beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
    m_logger->info(m_logMessage("OK"));
}

void HttpServer::Connection::getHistory(Sensors::Location location, int itemsCount, HistoryFields fields)
{
    std::string response = "Timestamp;Daylight";
    if (fields.temperature)
        response += ";Temperature";
    if (fields.alternative)
        response += ";Alternative";
    if (fields.humidity)
        response += ";Humidity";
    if (fields.pressure)
        response += ";Pressure";
    response += '\n';

    Sensors::Recorder::HistoryHandle handle = Sensors::Recorder::Instance->historyHandle();
    Sensors::Recorder::History::const_iterator iterator = (itemsCount == -1 || itemsCount > handle.history.size() ? handle.history.begin() : handle.history.end() - itemsCount);
    for (Sensors::Recorder::History::const_iterator end = handle.history.end(); iterator != end; ++iterator)
    {
        response += fmt::format(
            "{};{}",
            Utility::ToUnixTimestamp(iterator->timestamp),
            Utility::IsDaylight(iterator->timestamp) ? "true" : "false"
        );

        const auto& measurement = (location == Sensors::Location::Internal ? iterator->internal : iterator->external);
        if (!measurement)
        {
            if (fields.temperature)
                response += ';';
            if (fields.alternative)
                response += ';';
            if (fields.humidity)
                response += ';';
            if (fields.pressure)
                response += ';';
        }
        else
        {
            if (fields.temperature)
                response += fmt::format(";{:.2f}", measurement->bmp280.temperature);
            if (fields.alternative)
                response += fmt::format(";{:.2f}", measurement->aht20.temperature);
            if (fields.humidity)
                response += fmt::format(";{:.2f}", measurement->aht20.humidity);
            if (fields.pressure)
                response += fmt::format(";{:.2f}", measurement->bmp280.pressure);
        }
        response += '\n';
    }

    m_response.result(beast::http::status::ok);
    m_response.set(beast::http::field::content_type, "text/csv");
    beast::ostream(m_response.body()) << response;
    m_logger->info(m_logMessage("OK"));
}

void HttpServer::Connection::getDisplay(int indentation)
{
    json displayObject;
    displayObject["enabled"] = m_displayUi->enabled();

    json responseJson;
    responseJson["_success"] = true;
    responseJson["display"] = displayObject;

    m_response.result(beast::http::status::ok);
    m_response.set(beast::http::field::content_type, "application/json");
    beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
    m_logger->info(m_logMessage("OK"));
}

void HttpServer::Connection::postDisplay(int indentation)
{
    try
    {
        json requestJson = json::parse(beast::buffers_to_string(m_request.body().data()));
        bool enabled = requestJson.at("enabled");
        enabled ? m_displayUi->enable() : m_displayUi->disable();

        json displayObject;
        displayObject["enabled"] = enabled;

        json responseJson;
        responseJson["_success"] = true;
        responseJson["display"] = displayObject;

        m_response.result(beast::http::status::ok);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->info(m_logMessage(fmt::format("OK: Display is {}", enabled ? "enabled" : "disabled")));
    }
    catch (const json::exception&)
    {
        json responseJson;
        responseJson["_success"] = false;
        responseJson["what"] = "Couldn't parse request JSON";

        m_response.result(beast::http::status::bad_request);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->error(m_logMessage("Bad Request: Couldn't parse request JSON"));
    }
}

void HttpServer::Connection::getMaster(int indentation)
{
    json masterObject;
    masterObject["started"] = m_captureMaster->started();

    json responseJson;
    responseJson["_success"] = true;
    responseJson["master"] = masterObject;

    m_response.result(beast::http::status::ok);
    m_response.set(beast::http::field::content_type, "application/json");
    beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
    m_logger->info(m_logMessage("OK"));
}

void HttpServer::Connection::postMaster(int indentation)
{
    try
    {
        json requestJson = json::parse(beast::buffers_to_string(m_request.body().data()));
        bool started = requestJson.at("started");
        started ? m_captureMaster->start() : m_captureMaster->stop();

        json masterObject;
        masterObject["started"] = started;

        json responseJson;
        responseJson["_success"] = true;
        responseJson["master"] = masterObject;

        m_response.result(beast::http::status::ok);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->info(m_logMessage(fmt::format("OK: Capture master is {}", started ? "started" : "stopped")));
    }
    catch (const json::exception&)
    {
        json responseJson;
        responseJson["_success"] = false;
        responseJson["what"] = "Couldn't parse request JSON";

        m_response.result(beast::http::status::bad_request);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->error(m_logMessage("Bad Request: Couldn't parse request JSON"));
    }
}

void HttpServer::Connection::produceResponse()
{
    m_response.version(m_request.version());
    m_response.keep_alive(false);

    Target target = ParseTarget({ m_request.target().data(), m_request.target().size() });
    int indentation = GetIndentation(target.query);
    if (target.resource == "/api/external")
    {
        if (m_request.method() == beast::http::verb::get)
            getSensors(Sensors::Location::External, indentation);
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/external/trend")
    {
        if (m_request.method() == beast::http::verb::get)
            getTrend(Sensors::Location::External, indentation);
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/external/history")
    {
        if (m_request.method() == beast::http::verb::get)
            getHistory(Sensors::Location::External, GetItemsCount(target.query), GetHistoryFields(target.query));
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/internal")
    {
        if (m_request.method() == beast::http::verb::get)
            getSensors(Sensors::Location::Internal, indentation);
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/internal/trend")
    {
        if (m_request.method() == beast::http::verb::get)
            getTrend(Sensors::Location::Internal, indentation);
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/internal/history")
    {
        if (m_request.method() == beast::http::verb::get)
            getHistory(Sensors::Location::Internal, GetItemsCount(target.query), GetHistoryFields(target.query));
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/display")
    {
        if (m_request.method() == beast::http::verb::get)
            getDisplay(indentation);
        else if (m_request.method() == beast::http::verb::post)
            postDisplay(indentation);
        else
            methodNotAllowed();
        return;
    }
    else if (target.resource == "/api/master")
    {
        if (m_request.method() == beast::http::verb::get)
            getMaster(indentation);
        else if (m_request.method() == beast::http::verb::post)
            postMaster(indentation);
        else
            methodNotAllowed();
        return;
    }

    notFound();
    return;
}

void HttpServer::Connection::sendResponse()
{
    auto self = shared_from_this();
    m_response.content_length(m_response.body().size());
    beast::http::async_write(m_socket, m_response, [self](beast::error_code error, std::size_t bytesTransferred)
    {
        self->m_socket.shutdown(asio::ip::tcp::socket::shutdown_send, error);
        self->m_timeout.cancel();
    });
}

HttpServer::Connection::Connection(Logger logger, Display::Ui::Pointer displayUi, Capture::Master::Pointer captureMaster, asio::ip::tcp::socket& socket)
    : m_logger(logger)
    , m_displayUi(displayUi)
    , m_captureMaster(captureMaster)
    , m_socket(std::move(socket))
    , m_buffer(1024 * 8)
    , m_timeout(m_socket.get_executor(), std::chrono::seconds(10))
{
    m_logMessage = [this](const std::string& message)
    {
        return fmt::format(
            "{} {} from {}: {} {}",
            std::string(m_request.method_string()),
            std::string(m_request.target()),
            m_socket.remote_endpoint().address().to_string(),
            m_response.result_int(),
            message
        );
    };

    m_timeout.async_wait([this](beast::error_code error)
    {
        if (!error)
            m_socket.close(error);
    });
}

void HttpServer::Connection::handleRequest()
{
    auto self = shared_from_this();
    beast::http::async_read(m_socket, m_buffer, m_request, [self](beast::error_code error, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);
        if (error)
            return;

        self->produceResponse();
        self->sendResponse();
    });
}

void HttpServer::startAccepting()
{
    m_acceptor.async_accept(m_socket, [this](beast::error_code error)
    {
        if (error)
        {
            m_logger->error("Couldn't accept connection: \"{}\" ({})", error.message(), error.value());
            return;
        }

        std::make_shared<Connection>(m_logger, m_displayUi, m_captureMaster, m_socket)->handleRequest();
        startAccepting();
    });
}

HttpServer::HttpServer(Display::Ui::Pointer displayUi, Capture::Master::Pointer captureMaster)
    : m_logger(std::make_shared<spdlog::logger>(Utility::CreateLogger("http_server")))
    , m_displayUi(displayUi)
    , m_captureMaster(captureMaster)
    , m_context(1)
    , m_acceptor(m_context, { asio::ip::make_address("0.0.0.0"), Config::Instance->httpPort() })
    , m_socket(m_context)
{}

void HttpServer::start()
{
    m_logger->info("Listening for connections on port {}", Config::Instance->httpPort());
    startAccepting();
    m_context.run();
}

} // namespace kc
