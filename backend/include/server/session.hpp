#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class SessionManager;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(tcp::socket socket, SessionManager& manager);
    
    void run();
    void send(const std::string& message);
    const std::string& get_user_id() const { return user_id_; }
    bool is_authenticated() const { return authenticated_; }

private:
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void handle_message(const std::string& message);

    websocket::stream<tcp::socket> ws_;
    SessionManager& manager_;
    beast::flat_buffer buffer_;
    std::string user_id_;
    bool authenticated_;
};