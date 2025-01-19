#pragma once

#include <memory>
#include <functional>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
namespace beast = boost::beast;
namespace asio = boost::asio;

#include <spdlog/spdlog.h>

#include "capture/master.hpp"
#include "display/ui.hpp"
#include "sensors/recorder.hpp"

namespace cp {

class HttpServer {
public:
    using Logger = std::shared_ptr<spdlog::logger>;

    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        using LogMessageFunction = std::function<std::string(std::string)>;

        struct Target {
            std::string resource;
            std::string query;
        };

        struct HistoryFields {
            bool temperature = true;
            bool alternative = true;
            bool humidity = true;
            bool pressure = true;
        };

    private:
        Logger m_logger;
        LogMessageFunction m_logMessage;
        Display::Ui::Pointer m_displayUi;
        Capture::Master::Pointer m_captureMaster;
        asio::ip::tcp::socket m_socket;
        beast::flat_buffer m_buffer;
        beast::http::request<beast::http::dynamic_body> m_request;
        beast::http::response<beast::http::dynamic_body> m_response;
        asio::steady_timer m_timeout;

    public:
        /// @brief Initialize connection
        /// @param logger HTTP server logger
        /// @param displayUi Initialized display UI
        /// @param captureMaster Initialized capture master
        /// @param socket Connection socket
        Connection(Logger logger, Display::Ui::Pointer displayUi, Capture::Master::Pointer captureMaster, asio::ip::tcp::socket& socket);

    private:
        // "404 Not Found"
        void notFound();

        // "405 Method Not Allowed"
        void methodNotAllowed();

        // GET /api/<location>
        void getSensors(Sensors::Location location, int indentation);

        // GET "/api/<location>/trend"
        void getTrend(Sensors::Location location, int indentation);

        // GET "/api/<location>/history"
        void getHistory(Sensors::Location location, int itemsCount, HistoryFields fields);

        // GET "/api/display"
        void getDisplay(int indentation);

        // POST "/api/display"
        void postDisplay(int indentation);

        // GET "/api/master"
        void getMaster(int indentation);

        // POST "/api/master"
        void postMaster(int indentation);

        void produceResponse();

        void sendResponse();
    
    public:
        void handleRequest();
    };

private:
    Logger m_logger;
    Display::Ui::Pointer m_displayUi;
    Capture::Master::Pointer m_captureMaster;
    boost::asio::io_context m_context;
    asio::ip::tcp::acceptor m_acceptor;
    asio::ip::tcp::socket m_socket;

    void startAccepting();

public:
    HttpServer(Display::Ui::Pointer displayUi, Capture::Master::Pointer captureMaster);

    void start();
};

} // namespace cp
