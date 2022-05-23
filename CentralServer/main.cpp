#define BOOST_LOG_DYN_LINK 1
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include "lfrconnectionsmanager.h"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket) : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void read_request()
    {
        auto self = shared_from_this();

        http::async_read(socket_, buffer_, request_, [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if(ec)
                std::cout << ec.message() << std::endl;
            self->process_request();
        });
    }

    // Determine what needs to be done with the request message.
    void process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                    << "Invalid request-method '"
                    << std::string(request_.method_string())
                    << "'";
            break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void create_response()
    {
        if(request_.target() == "/connect")
        {
            BOOST_LOG_TRIVIAL(debug) << "connect get";
            response_.set(http::field::content_type, "text/plane");
            beast::ostream(response_.body()) << "8081";
        }
    }

    // Asynchronously transmit the response message.
    void write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(socket_, response_, [self](beast::error_code ec, std::size_t)
        {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            self->deadline_.cancel();
        });
    }

    // Check whether we have spent enough time on this connection.
    void check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
                    [self](beast::error_code ec)
        {
            if(!ec)
            {
                // Close socket to cancel any outstanding operation.
                self->socket_.close(ec);
            }
        });
    }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket,
                          [&](beast::error_code ec)
    {
        if(!ec)
            std::make_shared<http_connection>(std::move(socket))->start();
        http_server(acceptor, socket);
    });
}

void init_logging()
{
    boost::log::add_common_attributes();
    boost::log::add_file_log(
                boost::log::keywords::file_name = "sample_%N.log",
                boost::log::keywords::rotation_size = 10 * 1024 * 1024,
                boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
                boost::log::keywords::format = "[%TimeStamp%]: %Message%",
                boost::log::keywords::auto_flush = true
            );
    boost::log::add_console_log(
                std::cout,
                boost::log::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%",
                boost::log::keywords::auto_flush = true
            );
}

int main(int argc, char* argv[])
{
    init_logging();
    BOOST_LOG_TRIVIAL(debug) << "Main start" << std::flush;
    try
    {
		auto const address = net::ip::make_address("127.0.0.1");
        unsigned short port = 8080;

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);
        BOOST_LOG_TRIVIAL(debug) << "HTTP server created";

        LFRConnectionsManager manager({address, 8081});
        BOOST_LOG_TRIVIAL(debug) << "LFR manager created";

        manager.accepting();
        BOOST_LOG_TRIVIAL(debug) << "LFR manager accepting";

        std::thread t = std::thread([&]()
        {
            manager.run();
        });
        BOOST_LOG_TRIVIAL(debug) << "LFR manager thread created";


        BOOST_LOG_TRIVIAL(debug) << "runnung context";
        ioc.run();
    }
    catch(std::exception const& e)
    {
        BOOST_LOG_TRIVIAL(fatal) << "Error: " << e.what();
        return EXIT_FAILURE;
    }
}

