// src/database/message_repository.cpp
#include "database/message_repository.hpp"
#include "utils/logger.hpp"

MessageRepository::MessageRepository(Database& db) : db_(db) {}

std::optional<Message> MessageRepository::send_message(
    const std::string& sender_id,
    const std::string& recipient_id,
    const std::string& content,
    const std::string& message_type) {
    
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "INSERT INTO messages (sender_id, recipient_id, content, message_type) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING message_id, sender_id, recipient_id, group_id, content, "
            "message_type, created_at, is_read",
            sender_id, recipient_id, content, message_type
        );
        
        txn.commit();
        
        if (!result.empty()) {
            Message msg;
            msg.message_id = result[0]["message_id"].as<std::string>();
            msg.sender_id = result[0]["sender_id"].as<std::string>();
            msg.recipient_id = result[0]["recipient_id"].as<std::string>();
            msg.group_id = result[0]["group_id"].is_null() ? "" : result[0]["group_id"].as<std::string>();
            msg.content = result[0]["content"].as<std::string>();
            msg.message_type = result[0]["message_type"].as<std::string>();
            msg.created_at = result[0]["created_at"].as<std::string>();
            msg.is_read = result[0]["is_read"].as<bool>();
            
            Logger::get()->info("Message sent from {} to {}", sender_id, recipient_id);
            return msg;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to send message: {}", e.what());
    }
    
    return std::nullopt;
}

std::optional<Message> MessageRepository::send_group_message(
    const std::string& sender_id,
    const std::string& group_id,
    const std::string& content,
    const std::string& message_type) {
    
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "INSERT INTO messages (sender_id, group_id, content, message_type) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING message_id, sender_id, recipient_id, group_id, content, "
            "message_type, created_at, is_read",
            sender_id, group_id, content, message_type
        );
        
        txn.commit();
        
        if (!result.empty()) {
            Message msg;
            msg.message_id = result[0]["message_id"].as<std::string>();
            msg.sender_id = result[0]["sender_id"].as<std::string>();
            msg.recipient_id = result[0]["recipient_id"].is_null() ? "" : result[0]["recipient_id"].as<std::string>();
            msg.group_id = result[0]["group_id"].as<std::string>();
            msg.content = result[0]["content"].as<std::string>();
            msg.message_type = result[0]["message_type"].as<std::string>();
            msg.created_at = result[0]["created_at"].as<std::string>();
            msg.is_read = result[0]["is_read"].as<bool>();
            
            Logger::get()->info("Group message sent from {} to group {}", sender_id, group_id);
            return msg;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to send group message: {}", e.what());
    }
    
    return std::nullopt;
}

std::vector<Message> MessageRepository::get_conversation(
    const std::string& user1_id,
    const std::string& user2_id,
    int limit) {
    
    std::vector<Message> messages;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT message_id, sender_id, recipient_id, group_id, content, "
            "message_type, created_at, is_read "
            "FROM messages "
            "WHERE (sender_id = $1 AND recipient_id = $2) "
            "   OR (sender_id = $2 AND recipient_id = $1) "
            "ORDER BY created_at DESC LIMIT $3",
            user1_id, user2_id, limit
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            Message msg;
            msg.message_id = row["message_id"].as<std::string>();
            msg.sender_id = row["sender_id"].as<std::string>();
            msg.recipient_id = row["recipient_id"].as<std::string>();
            msg.group_id = row["group_id"].is_null() ? "" : row["group_id"].as<std::string>();
            msg.content = row["content"].as<std::string>();
            msg.message_type = row["message_type"].as<std::string>();
            msg.created_at = row["created_at"].as<std::string>();
            msg.is_read = row["is_read"].as<bool>();
            messages.push_back(msg);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get conversation: {}", e.what());
    }
    
    return messages;
}

std::vector<Message> MessageRepository::get_group_messages(
    const std::string& group_id,
    int limit) {
    
    std::vector<Message> messages;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT message_id, sender_id, recipient_id, group_id, content, "
            "message_type, created_at, is_read "
            "FROM messages "
            "WHERE group_id = $1 "
            "ORDER BY created_at DESC LIMIT $2",
            group_id, limit
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            Message msg;
            msg.message_id = row["message_id"].as<std::string>();
            msg.sender_id = row["sender_id"].as<std::string>();
            msg.recipient_id = row["recipient_id"].is_null() ? "" : row["recipient_id"].as<std::string>();
            msg.group_id = row["group_id"].as<std::string>();
            msg.content = row["content"].as<std::string>();
            msg.message_type = row["message_type"].as<std::string>();
            msg.created_at = row["created_at"].as<std::string>();
            msg.is_read = row["is_read"].as<bool>();
            messages.push_back(msg);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get group messages: {}", e.what());
    }
    
    return messages;
}

bool MessageRepository::mark_message_read(const std::string& message_id) {
    try {
        pqxx::work txn(db_.get_connection());
        txn.exec_params(
            "UPDATE messages SET is_read = TRUE WHERE message_id = $1",
            message_id
        );
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to mark message as read: {}", e.what());
        return false;
    }
}
