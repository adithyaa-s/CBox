#pragma once
#include "database.hpp"
#include <string>
#include <vector>
#include <optional>

struct Group {
    std::string group_id;
    std::string group_name;
    std::string description;
    std::string created_by;
};

struct GroupMember {
    std::string group_id;
    std::string user_id;
    std::string role;
};

class GroupRepository {
public:
    explicit GroupRepository(Database& db);
    
    std::optional<Group> create_group(const std::string& group_name,
                                     const std::string& description,
                                     const std::string& creator_id);
    
    bool add_member(const std::string& group_id, const std::string& user_id, 
                   const std::string& role = "member");
    
    bool remove_member(const std::string& group_id, const std::string& user_id);
    
    std::vector<Group> get_user_groups(const std::string& user_id);
    
    std::vector<GroupMember> get_group_members(const std::string& group_id);
    
    bool is_member(const std::string& group_id, const std::string& user_id);

private:
    Database& db_;
};