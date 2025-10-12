
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
