// src/handlers/friend_handler.cpp
#include "handlers/friend_handler.hpp"
#include "server/session_manager.hpp"
#include "utils/logger.hpp"
#include <boost/json.hpp>

FriendHandler::FriendHandler(Database& db)
    : db_(db)
    , session_manager_(nullptr) {
}

void FriendHandler::set_session_manager(SessionManager* manager) {
    session_manager_ = manager;
}

std::string FriendHandler::handle_send_friend_request(
    const std::string& sender_id,
    const std::string& receiver_username) {
    
    namespace json = boost::json;
    json::object response;
    
    try {
        pqxx::work txn(db_.get_connection());
        
        // Get receiver user_id
        auto user_result = txn.exec_params(
            "SELECT user_id FROM users WHERE username = $1",
            receiver_username
        );
        
        if (user_result.empty()) {
            response["type"] = "error";
            response["message"] = "User not found";
            return json::serialize(response);
        }
        
        std::string receiver_id = user_result[0]["user_id"].as<std::string>();
        
        // Check if already friends
        auto friendship_check = txn.exec_params(
            "SELECT 1 FROM friendships "
            "WHERE (user1_id = $1 AND user2_id = $2) OR (user1_id = $2 AND user2_id = $1)",
            sender_id < receiver_id ? sender_id : receiver_id,
            sender_id < receiver_id ? receiver_id : sender_id
        );
        
        if (!friendship_check.empty()) {
            response["type"] = "error";
            response["message"] = "Already friends";
            return json::serialize(response);
        }
        
        // Create friend request
        auto result = txn.exec_params(
            "INSERT INTO friend_requests (sender_id, receiver_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT (sender_id, receiver_id) DO NOTHING "
            "RETURNING request_id",
            sender_id, receiver_id
        );
        
        txn.commit();
        
        if (!result.empty()) {
            std::string request_id = result[0]["request_id"].as<std::string>();
            
            response["type"] = "friend_request_sent";
            response["request_id"] = request_id;
            response["receiver_id"] = receiver_id;
            
            // Notify receiver if online
            if (session_manager_ && session_manager_->is_user_online(receiver_id)) {
                json::object notification;
                notification["type"] = "friend_request_received";
                notification["request_id"] = request_id;
                notification["sender_id"] = sender_id;
                session_manager_->send_to_user(receiver_id, json::serialize(notification));
            }
            
            Logger::get()->info("Friend request sent from {} to {}", sender_id, receiver_id);
        } else {
            response["type"] = "error";
            response["message"] = "Request already exists";
        }
        
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to send friend request: {}", e.what());
        response["type"] = "error";
        response["message"] = "Failed to send friend request";
    }
    
    return json::serialize(response);
}

std::string FriendHandler::handle_accept_friend_request(
    const std::string& user_id,
    const std::string& request_id) {
    
    namespace json = boost::json;
    json::object response;
    
    try {
        pqxx::work txn(db_.get_connection());
        
        // Get request details
        auto request_result = txn.exec_params(
            "SELECT sender_id, receiver_id FROM friend_requests "
            "WHERE request_id = $1 AND receiver_id = $2 AND status = 'pending'",
            request_id, user_id
        );
        
        if (request_result.empty()) {
            response["type"] = "error";
            response["message"] = "Friend request not found";
            return json::serialize(response);
        }
        
        std::string sender_id = request_result[0]["sender_id"].as<std::string>();
        std::string receiver_id = request_result[0]["receiver_id"].as<std::string>();
        
        // Update request status
        txn.exec_params(
            "UPDATE friend_requests SET status = 'accepted', updated_at = CURRENT_TIMESTAMP "
            "WHERE request_id = $1",
            request_id
        );
        
        // Create friendship
        txn.exec_params(
            "INSERT INTO friendships (user1_id, user2_id) VALUES ($1, $2)",
            sender_id < receiver_id ? sender_id : receiver_id,
            sender_id < receiver_id ? receiver_id : sender_id
        );
        
        txn.commit();
        
        response["type"] = "friend_request_accepted";
        response["request_id"] = request_id;
        response["friend_id"] = sender_id;
        
        // Notify sender if online
        if (session_manager_ && session_manager_->is_user_online(sender_id)) {
            json::object notification;
            notification["type"] = "friend_request_accepted";
            notification["friend_id"] = receiver_id;
            session_manager_->send_to_user(sender_id, json::serialize(notification));
        }
        
        Logger::get()->info("Friend request accepted: {}", request_id);
        
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to accept friend request: {}", e.what());
        response["type"] = "error";
        response["message"] = "Failed to accept friend request";
    }
    
    return json::serialize(response);
}

std::string FriendHandler::handle_reject_friend_request(
    const std::string& user_id,
    const std::string& request_id) {
    
    namespace json = boost::json;
    json::object response;
    
    try {
        pqxx::work txn(db_.get_connection());
        
        txn.exec_params(
            "UPDATE friend_requests SET status = 'rejected', updated_at = CURRENT_TIMESTAMP "
            "WHERE request_id = $1 AND receiver_id = $2",
            request_id, user_id
        );
        
        txn.commit();
        
        response["type"] = "friend_request_rejected";
        response["request_id"] = request_id;
        
        Logger::get()->info("Friend request rejected: {}", request_id);
        
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to reject friend request: {}", e.what());
        response["type"] = "error";
        response["message"] = "Failed to reject friend request";
    }
    
    return json::serialize(response);
}

std::string FriendHandler::handle_get_friend_requests(const std::string& user_id) {
    namespace json = boost::json;
    json::object response;
    response["type"] = "friend_requests";
    
    try {
        pqxx::work txn(db_.get_connection());
        
        auto result = txn.exec_params(
            "SELECT fr.request_id, fr.sender_id, u.username, u.display_name, fr.created_at "
            "FROM friend_requests fr "
            "JOIN users u ON fr.sender_id = u.user_id "
            "WHERE fr.receiver_id = $1 AND fr.status = 'pending' "
            "ORDER BY fr.created_at DESC",
            user_id
        );
        
        txn.commit();
        
        json::array requests_array;
        for (const auto& row : result) {
            json::object req_obj;
            req_obj["request_id"] = row["request_id"].as<std::string>();
            req_obj["sender_id"] = row["sender_id"].as<std::string>();
            req_obj["username"] = row["username"].as<std::string>();
            req_obj["display_name"] = row["display_name"].as<std::string>();
            req_obj["created_at"] = row["created_at"].as<std::string>();
            requests_array.push_back(req_obj);
        }
        
        response["requests"] = requests_array;
        
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get friend requests: {}", e.what());
        response["type"] = "error";
        response["message"] = "Failed to get friend requests";
    }
    
    return json::serialize(response);
}

std::string FriendHandler::handle_get_friends(const std::string& user_id) {
    namespace json = boost::json;
    json::object response;
    response["type"] = "friends";
    
    try {
        pqxx::work txn(db_.get_connection());
        
        auto result = txn.exec_params(
            "SELECT u.user_id, u.username, u.display_name, u.status "
            "FROM friendships f "
            "JOIN users u ON (CASE WHEN f.user1_id = $1 THEN f.user2_id ELSE f.user1_id END) = u.user_id "
            "WHERE f.user1_id = $1 OR f.user2_id = $1 "
            "ORDER BY u.username",
            user_id
        );
        
        txn.commit();
        
        json::array friends_array;
        for (const auto& row : result) {
            json::object friend_obj;
            friend_obj["user_id"] = row["user_id"].as<std::string>();
            friend_obj["username"] = row["username"].as<std::string>();
            friend_obj["display_name"] = row["display_name"].as<std::string>();
            friend_obj["status"] = row["status"].as<std::string>();
            friends_array.push_back(friend_obj);
        }
        
        response["friends"] = friends_array;
        
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get friends: {}", e.what());
        response["type"] = "error";
        response["message"] = "Failed to get friends";
    }
    
    return json::serialize(response);
}

