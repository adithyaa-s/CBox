#include "client_session.h"
#include <iostream>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)) {}

void ClientSession::start() {
    read();
}

void ClientSession::read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string msg(data_, length);
                std::cout << "Received: " << msg << std::endl;
                write("Message received\n");
                read();
            }
        });
}

void ClientSession::write(const std::string& message) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(message),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                std::cerr << "Write failed: " << ec.message() << std::endl;
            }
        });
}
