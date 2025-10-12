// src/server/session.cpp
#include "server/session.hpp"
#include "server/session_manager.hpp"
#include "utils/logger.hpp"
#include <boost/json.hpp>

Session::Session(tcp::socket socket, SessionManager& manager)
    : ws_(std::move(socket))
    , manager_(manager)
    , authenticated_(false) {
}

void Session::run() {
    ws_.async_accept(
        beast::bind_front_handler(&Session::on_accept, shared_from_this())
    );
}

void Session::on_accept(beast::error_code ec) {
    if (ec) {
        Logger::get()->error("WebSocket accept error: {}", ec.message());
        return;
    }
    
    Logger::get()->info("WebSocket connection accepted");
    do_read();
}

void Session::do_read() {
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(&Session::on_read, shared_from_this())
    );
}

void Session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec == websocket::error::closed) {
        Logger::get()->info("WebSocket closed gracefully");
        if (authenticated_) {
            manager_.leave(user_id_);
        }
        return;
    }
    
    if (ec) {
        Logger::get()->error("WebSocket read error: {}", ec.message());
        return;
    }
    
    std::string message = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    
    handle_message(message);
    do_read();
}

void Session::handle_message(const std::string& message) {
    try {
        namespace json = boost::json;
        auto parsed = json::parse(message);
        auto& obj = parsed.as_object();
        
        std::string type = obj.at("type").as_string().c_str();
        
        if (type == "auth") {
            std::string token = obj.at("token").as_string().c_str();
            // Validate token and extract user_id
            // Simplified - in production, properly validate JWT
            if (!token.empty()) {
                user_id_ = obj.at("user_id").as_string().c_str();
                authenticated_ = true;
                manager_.join(shared_from_this(), user_id_);
                
                json::object response;
                response["type"] = "auth_success";
                response["user_id"] = user_id_;
                send(json::serialize(response));
                
                Logger::get()->info("User authenticated: {}", user_id_);
            }
        } else if (authenticated_) {
            manager_.handle_client_message(user_id_, message);
        } else {
            json::object error;
            error["type"] = "error";
            error["message"] = "Not authenticated";
            send(json::serialize(error));
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Error handling message: {}", e.what());
        
        namespace json = boost::json;
        json::object error;
        error["type"] = "error";
        error["message"] = "Invalid message format";
        send(json::serialize(error));
    }
}

void Session::send(const std::string& message) {
    auto self = shared_from_this();
    ws_.async_write(
        net::buffer(message),
        [self](beast::error_code ec, std::size_t bytes) {
            self->on_write(ec, bytes);
        }
    );
}

void Session::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        Logger::get()->error("WebSocket write error: {}", ec.message());
        return;
    }
}
