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
    m_displayUi->updateEventTimestamp(pt::second_clock::local_time());
    try
    {
        Sensors::Measurement measurement = Sensors::Measure(m_config, location);

        json aht20Object;
        aht20Object["temperature"] = measurement.aht20.temperature;
        aht20Object["humidity"] = measurement.aht20.humidity;

        json bmp280Object;
        bmp280Object["temperature"] = measurement.bmp280.temperature;
        bmp280Object["pressure"] = measurement.bmp280.pressure;

        json responseJson;
        responseJson["_success"] = true;
        responseJson["aht20"] = aht20Object;
        responseJson["bmp280"] = bmp280Object;

        m_response.result(beast::http::status::ok);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->info(m_logMessage("OK"));

        m_displayUi->showMessage({
            location == Sensors::Location::External ? "Ext. measurement" : "Int. measurement", {
                { fmt::format("\7{:>15}", m_socket.remote_endpoint().address().to_string()) },
                { fmt::format("\6{:>15}", "Successful \1") }
            }
        });
    }
    catch (const std::runtime_error& error)
    {
        json responseJson;
        responseJson["_success"] = false;
        responseJson["what"] = error.what();

        m_response.result(beast::http::status::internal_server_error);
        m_response.set(beast::http::field::content_type, "application/json");
        beast::ostream(m_response.body()) << responseJson.dump(indentation) << '\n';
        m_logger->error(m_logMessage(fmt::format("Internal Server Error: {}", error.what())));

        m_displayUi->showMessage({
            location == Sensors::Location::External ? "Ext. measurement" : "Int. measurement", {
                { fmt::format("\7{:>15}", m_socket.remote_endpoint().address().to_string()) },
                { fmt::format("\6{:>15}", "Sensor error \2"), 1.0, 4 },
            }
        });
    }
}

void HttpServer::Connection::getDisplay(int indentation)
{
    json displayJson;
    displayJson["enabled"] = m_displayUi->enabled();

    json responseJson;
    responseJson["_success"] = true;
    responseJson["display"] = displayJson;

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

        json displayJson;
        displayJson["enabled"] = enabled;

        json responseJson;
        responseJson["_success"] = true;
        responseJson["display"] = displayJson;

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
    else if (target.resource == "/api/internal")
    {
        if (m_request.method() == beast::http::verb::get)
            getSensors(Sensors::Location::Internal, indentation);
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

HttpServer::Connection::Connection(Logger logger, Config::Pointer config, Display::Ui::Pointer displayUi, asio::ip::tcp::socket& socket)
    : m_logger(logger)
    , m_config(config)
    , m_displayUi(displayUi)
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

        std::make_shared<Connection>(m_logger, m_config, m_displayUi, m_socket)->handleRequest();
        startAccepting();
    });
}

HttpServer::HttpServer(Config::Pointer config, Display::Ui::Pointer displayUi)
    : m_logger(std::make_shared<spdlog::logger>(Utility::CreateLogger("http_server")))
    , m_config(config)
    , m_displayUi(displayUi)
    , m_context(1)
    , m_acceptor(m_context, { asio::ip::make_address("0.0.0.0"), config->httpPort() })
    , m_socket(m_context)
{}

void HttpServer::start()
{
    m_logger->info("Listening for connections on port {}", m_config->httpPort());
    startAccepting();
    m_context.run();
}

} // namespace kc
