#include "server/websocket_server.hpp"
#include "server/session.hpp"
#include "utils/logger.hpp"

WebSocketServer::WebSocketServer(net::io_context& ioc,
                                tcp::endpoint endpoint,
                                SessionManager& manager)
    : ioc_(ioc)
    , acceptor_(ioc)
    , manager_(manager) {
    
    beast::error_code ec;
    
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        Logger::get()->error("Failed to open acceptor: {}", ec.message());
        return;
    }
    
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        Logger::get()->error("Failed to set reuse_address: {}", ec.message());
        return;
    }
    
    acceptor_.bind(endpoint, ec);
    if (ec) {
        Logger::get()->error("Failed to bind: {}", ec.message());
        return;
    }
    
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        Logger::get()->error("Failed to listen: {}", ec.message());
        return;
    }
    
    Logger::get()->info("WebSocket server listening on {}:{}", 
                       endpoint.address().to_string(), 
                       endpoint.port());
}

void WebSocketServer::run() {
    do_accept();
}

void WebSocketServer::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&WebSocketServer::on_accept, this)
    );
}

void WebSocketServer::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        Logger::get()->error("Accept error: {}", ec.message());
    } else {
        std::make_shared<Session>(std::move(socket), manager_)->run();
    }
    
    do_accept();
}