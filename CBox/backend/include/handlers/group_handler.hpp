#pragma once
#include "../database/group_repository.hpp"
#include <string>

class SessionManager;

class GroupHandler {
public:
    explicit GroupHandler(GroupRepository& group_repo);
    
    void set_session_manager(SessionManager* manager);
    std::string handle_create_group(const std::string& creator_id,
                                   const std::string& group_name,
                                   const std::string& description);
    
    std::string handle_add_member(const std::string& group_id,
                                 const std::string& user_id);
    
    std::string handle_get_groups(const std::string& user_id);

private:
    GroupRepository& group_repo_;
    SessionManager* session_manager_;
};