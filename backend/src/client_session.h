#pragma once
#include <boost/asio.hpp>
#include <memory>

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket);
    void start();

private:
    void read();
    void write(const std::string& message);

    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};
