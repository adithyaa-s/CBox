#pragma once
#include "session_manager.hpp"
#include <boost/asio.hpp>
#include <memory>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class WebSocketServer {
public:
    WebSocketServer(net::io_context& ioc, 
                   tcp::endpoint endpoint,
                   SessionManager& manager);
    
    void run();

private:
    void do_accept();
    void on_accept(boost::system::error_code ec, tcp::socket socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    SessionManager& manager_;
};