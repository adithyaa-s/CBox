#pragma once
#include <boost/asio.hpp>

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);
private:
    void accept();
    boost::asio::ip::tcp::acceptor acceptor_;
};
