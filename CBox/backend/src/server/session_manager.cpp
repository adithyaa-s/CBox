
// src/server/session_manager.cpp
#include "server/session_manager.hpp"
#include "server/session.hpp"
#include "handlers/message_handler.hpp"
#include "handlers/group_handler.hpp"
#include "handlers/friend_handler.hpp"
#include "utils/logger.hpp"
#include <boost/json.hpp>

SessionManager::SessionManager(MessageHandler& msg_handler,
                              GroupHandler& group_handler,
                              FriendHandler& friend_handler)
    : msg_handler_(msg_handler)
    , group_handler_(group_handler)
    , friend_handler_(friend_handler) {
    
    msg_handler_.set_session_manager(this);
    group_handler_.set_session_manager(this);
    friend_handler_.set_session_manager(this);
}

void SessionManager::join(std::shared_ptr<Session> session, const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[user_id] = session;
    Logger::get()->info("Session joined: {}", user_id);
}

void SessionManager::leave(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(user_id);
    Logger::get()->info("Session left: {}", user_id);
}

void SessionManager::send_to_user(const std::string& user_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(user_id);
    if (it != sessions_.end()) {
        it->second->send(message);
    }
}

bool SessionManager::is_user_online(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(user_id) != sessions_.end();
}

void SessionManager::handle_client_message(const std::string& user_id, const std::string& message) {
    try {
        namespace json = boost::json;
        auto parsed = json::parse(message);
        auto& obj = parsed.as_object();
        
        std::string type = obj.at("type").as_string().c_str();
        
        if (type == "send_message") {
            std::string recipient_id = obj.at("recipient_id").as_string().c_str();
            std::string content = obj.at("content").as_string().c_str();
            msg_handler_.handle_send_message(user_id, recipient_id, content);
            
        } else if (type == "send_group_message") {
            std::string group_id = obj.at("group_id").as_string().c_str();
            std::string content = obj.at("content").as_string().c_str();
            msg_handler_.handle_send_group_message(user_id, group_id, content);
            
        } else if (type == "get_conversation") {
            std::string other_user_id = obj.at("user_id").as_string().c_str();
            std::string response = msg_handler_.handle_get_conversation(user_id, other_user_id);
            send_to_user(user_id, response);
            
        } else if (type == "create_group") {
            std::string group_name = obj.at("group_name").as_string().c_str();
            std::string description = obj.at("description").as_string().c_str();
            std::string response = group_handler_.handle_create_group(user_id, group_name, description);
            send_to_user(user_id, response);
            
        } else if (type == "add_group_member") {
            std::string group_id = obj.at("group_id").as_string().c_str();
            std::string member_id = obj.at("user_id").as_string().c_str();
            std::string response = group_handler_.handle_add_member(group_id, member_id);
            send_to_user(user_id, response);
            
        } else if (type == "get_groups") {
            std::string response = group_handler_.handle_get_groups(user_id);
            send_to_user(user_id, response);
            
        } else if (type == "send_friend_request") {
            std::string receiver_username = obj.at("username").as_string().c_str();
            std::string response = friend_handler_.handle_send_friend_request(user_id, receiver_username);
            send_to_user(user_id, response);
            
        } else if (type == "accept_friend_request") {
            std::string request_id = obj.at("request_id").as_string().c_str();
            std::string response = friend_handler_.handle_accept_friend_request(user_id, request_id);
            send_to_user(user_id, response);
            
        } else if (type == "get_friend_requests") {
            std::string response = friend_handler_.handle_get_friend_requests(user_id);
            send_to_user(user_id, response);
            
        } else if (type == "get_friends") {
            std::string response = friend_handler_.handle_get_friends(user_id);
            send_to_user(user_id, response);
        }
        
    } catch (const std::exception& e) {
        Logger::get()->error("Error handling client message: {}", e.what());
    }
}

// src/server/websocket_server.cpp
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