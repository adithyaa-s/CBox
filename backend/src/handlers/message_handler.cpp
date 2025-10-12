// src/handlers/message_handler.cpp
#include "handlers/message_handler.hpp"
#include "server/session_manager.hpp"
#include "utils/logger.hpp"
#include <boost/json.hpp>

MessageHandler::MessageHandler(MessageRepository& msg_repo)
    : msg_repo_(msg_repo)
    , session_manager_(nullptr) {
}

void MessageHandler::set_session_manager(SessionManager* manager) {
    session_manager_ = manager;
}

void MessageHandler::handle_send_message(
    const std::string& sender_id,
    const std::string& recipient_id,
    const std::string& content) {
    
    auto message = msg_repo_.send_message(sender_id, recipient_id, content);
    
    if (message) {
        namespace json = boost::json;
        
        // Send to sender (confirmation)
        json::object sender_response;
        sender_response["type"] = "message_sent";
        sender_response["message_id"] = message->message_id;
        sender_response["recipient_id"] = recipient_id;
        sender_response["content"] = content;
        sender_response["created_at"] = message->created_at;
        
        if (session_manager_) {
            session_manager_->send_to_user(sender_id, json::serialize(sender_response));
            
            // Send to recipient if online
            if (session_manager_->is_user_online(recipient_id)) {
                json::object recipient_response;
                recipient_response["type"] = "new_message";
                recipient_response["message_id"] = message->message_id;
                recipient_response["sender_id"] = sender_id;
                recipient_response["content"] = content;
                recipient_response["created_at"] = message->created_at;
                
                session_manager_->send_to_user(recipient_id, json::serialize(recipient_response));
            }
        }
        
        Logger::get()->info("Message delivered from {} to {}", sender_id, recipient_id);
    } else {
        Logger::get()->error("Failed to send message from {} to {}", sender_id, recipient_id);
    }
}

void MessageHandler::handle_send_group_message(
    const std::string& sender_id,
    const std::string& group_id,
    const std::string& content) {
    
    auto message = msg_repo_.send_group_message(sender_id, group_id, content);
    
    if (message && session_manager_) {
        namespace json = boost::json;
        
        json::object response;
        response["type"] = "group_message";
        response["message_id"] = message->message_id;
        response["sender_id"] = sender_id;
        response["group_id"] = group_id;
        response["content"] = content;
        response["created_at"] = message->created_at;
        
        std::string response_str = json::serialize(response);
        
        // In production, get all group members and send to online ones
        // For now, simplified implementation
        Logger::get()->info("Group message sent from {} to group {}", sender_id, group_id);
    }
}

std::string MessageHandler::handle_get_conversation(
    const std::string& user1_id,
    const std::string& user2_id) {
    
    auto messages = msg_repo_.get_conversation(user1_id, user2_id);
    
    namespace json = boost::json;
    json::object response;
    response["type"] = "conversation";
    
    json::array messages_array;
    for (const auto& msg : messages) {
        json::object msg_obj;
        msg_obj["message_id"] = msg.message_id;
        msg_obj["sender_id"] = msg.sender_id;
        msg_obj["recipient_id"] = msg.recipient_id;
        msg_obj["content"] = msg.content;
        msg_obj["message_type"] = msg.message_type;
        msg_obj["created_at"] = msg.created_at;
        msg_obj["is_read"] = msg.is_read;
        messages_array.push_back(msg_obj);
    }
    
    response["messages"] = messages_array;
    return json::serialize(response);
}
