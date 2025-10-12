
// src/database/group_repository.cpp
#include "database/group_repository.hpp"
#include "utils/logger.hpp"

GroupRepository::GroupRepository(Database& db) : db_(db) {}

std::optional<Group> GroupRepository::create_group(
    const std::string& group_name,
    const std::string& description,
    const std::string& creator_id) {
    
    try {
        pqxx::work txn(db_.get_connection());
        
        // Create group
        auto result = txn.exec_params(
            "INSERT INTO groups (group_name, description, created_by) "
            "VALUES ($1, $2, $3) "
            "RETURNING group_id, group_name, description, created_by",
            group_name, description, creator_id
        );
        
        if (!result.empty()) {
            std::string group_id = result[0]["group_id"].as<std::string>();
            
            // Add creator as admin
            txn.exec_params(
                "INSERT INTO group_members (group_id, user_id, role) "
                "VALUES ($1, $2, 'admin')",
                group_id, creator_id
            );
            
            txn.commit();
            
            Group group;
            group.group_id = group_id;
            group.group_name = result[0]["group_name"].as<std::string>();
            group.description = result[0]["description"].as<std::string>();
            group.created_by = result[0]["created_by"].as<std::string>();
            
            Logger::get()->info("Group created: {} by {}", group_name, creator_id);
            return group;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to create group: {}", e.what());
    }
    
    return std::nullopt;
}

bool GroupRepository::add_member(
    const std::string& group_id,
    const std::string& user_id,
    const std::string& role) {
    
    try {
        pqxx::work txn(db_.get_connection());
        txn.exec_params(
            "INSERT INTO group_members (group_id, user_id, role) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (group_id, user_id) DO NOTHING",
            group_id, user_id, role
        );
        txn.commit();
        
        Logger::get()->info("User {} added to group {}", user_id, group_id);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to add member to group: {}", e.what());
        return false;
    }
}

bool GroupRepository::remove_member(
    const std::string& group_id,
    const std::string& user_id) {
    
    try {
        pqxx::work txn(db_.get_connection());
        txn.exec_params(
            "DELETE FROM group_members WHERE group_id = $1 AND user_id = $2",
            group_id, user_id
        );
        txn.commit();
        
        Logger::get()->info("User {} removed from group {}", user_id, group_id);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to remove member from group: {}", e.what());
        return false;
    }
}

std::vector<Group> GroupRepository::get_user_groups(const std::string& user_id) {
    std::vector<Group> groups;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT g.group_id, g.group_name, g.description, g.created_by "
            "FROM groups g "
            "JOIN group_members gm ON g.group_id = gm.group_id "
            "WHERE gm.user_id = $1",
            user_id
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            Group group;
            group.group_id = row["group_id"].as<std::string>();
            group.group_name = row["group_name"].as<std::string>();
            group.description = row["description"].as<std::string>();
            group.created_by = row["created_by"].as<std::string>();
            groups.push_back(group);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get user groups: {}", e.what());
    }
    
    return groups;
}

std::vector<GroupMember> GroupRepository::get_group_members(const std::string& group_id) {
    std::vector<GroupMember> members;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT group_id, user_id, role FROM group_members WHERE group_id = $1",
            group_id
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            GroupMember member;
            member.group_id = row["group_id"].as<std::string>();
            member.user_id = row["user_id"].as<std::string>();
            member.role = row["role"].as<std::string>();
            members.push_back(member);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get group members: {}", e.what());
    }
    
    return members;
}

bool GroupRepository::is_member(const std::string& group_id, const std::string& user_id) {
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT 1 FROM group_members WHERE group_id = $1 AND user_id = $2",
            group_id, user_id
        );
        txn.commit();// src/database/message_repository.cpp
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

// src/database/group_repository.cpp
#include "database/group_repository.hpp"
#include "utils/logger.hpp"

GroupRepository::GroupRepository(Database& db) : db_(db) {}

std::optional<Group> GroupRepository::create_group(
    const std::string& group_name,
    const std::string& description,
    const std::string& creator_id) {
    
    try {
        pqxx::work txn(db_.get_connection());
        
        // Create group
        auto result = txn.exec_params(
            "INSERT INTO groups (group_name, description, created_by) "
            "VALUES ($1, $2, $3) "
            "RETURNING group_id, group_name, description, created_by",
            group_name, description, creator_id
        );
        
        if (!result.empty()) {
            std::string group_id = result[0]["group_id"].as<std::string>();
            
            // Add creator as admin
            txn.exec_params(
                "INSERT INTO group_members (group_id, user_id, role) "
                "VALUES ($1, $2, 'admin')",
                group_id, creator_id
            );
            
            txn.commit();
            
            Group group;
            group.group_id = group_id;
            group.group_name = result[0]["group_name"].as<std::string>();
            group.description = result[0]["description"].as<std::string>();
            group.created_by = result[0]["created_by"].as<std::string>();
            
            Logger::get()->info("Group created: {} by {}", group_name, creator_id);
            return group;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to create group: {}", e.what());
    }
    
    return std::nullopt;
}

bool GroupRepository::add_member(
    const std::string& group_id,
    const std::string& user_id,
    const std::string& role) {
    
    try {
        pqxx::work txn(db_.get_connection());
        txn.exec_params(
            "INSERT INTO group_members (group_id, user_id, role) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (group_id, user_id) DO NOTHING",
            group_id, user_id, role
        );
        txn.commit();
        
        Logger::get()->info("User {} added to group {}", user_id, group_id);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to add member to group: {}", e.what());
        return false;
    }
}

bool GroupRepository::remove_member(
    const std::string& group_id,
    const std::string& user_id) {
    
    try {
        pqxx::work txn(db_.get_connection());
        txn.exec_params(
            "DELETE FROM group_members WHERE group_id = $1 AND user_id = $2",
            group_id, user_id
        );
        txn.commit();
        
        Logger::get()->info("User {} removed from group {}", user_id, group_id);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to remove member from group: {}", e.what());
        return false;
    }
}

std::vector<Group> GroupRepository::get_user_groups(const std::string& user_id) {
    std::vector<Group> groups;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT g.group_id, g.group_name, g.description, g.created_by "
            "FROM groups g "
            "JOIN group_members gm ON g.group_id = gm.group_id "
            "WHERE gm.user_id = $1",
            user_id
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            Group group;
            group.group_id = row["group_id"].as<std::string>();
            group.group_name = row["group_name"].as<std::string>();
            group.description = row["description"].as<std::string>();
            group.created_by = row["created_by"].as<std::string>();
            groups.push_back(group);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get user groups: {}", e.what());
    }
    
    return groups;
}

std::vector<GroupMember> GroupRepository::get_group_members(const std::string& group_id) {
    std::vector<GroupMember> members;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT group_id, user_id, role FROM group_members WHERE group_id = $1",
            group_id
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            GroupMember member;
            member.group_id = row["group_id"].as<std::string>();
            member.user_id = row["user_id"].as<std::string>();
            member.role = row["role"].as<std::string>();
            members.push_back(member);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get group members: {}", e.what());
    }
    
    return members;
}

bool GroupRepository::is_member(const std::string& group_id, const std::string& user_id) {
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT 1 FROM group_members WHERE group_id = $1 AND user_id = $2",
            group_id, user_id
        );
        txn.commit();
        
        return !result.empty();
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to check group membership: {}", e.what());
        return false;
    }
}
        
        return !result.empty();
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to check group membership: {}", e.what());
        return false;
    }
}