#include "server.h"
#include "client_session.h"
#include <iostream>

Server::Server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    accept();
}

void Server::accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket){
        if (!ec) {
            std::make_shared<ClientSession>(std::move(socket))->start();
        }
        accept();
    });
}
