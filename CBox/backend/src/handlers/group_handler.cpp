// src/handlers/group_handler.cpp
#include "handlers/group_handler.hpp"
#include "server/session_manager.hpp"
#include "utils/logger.hpp"
#include <boost/json.hpp>

GroupHandler::GroupHandler(GroupRepository& group_repo)
    : group_repo_(group_repo)
    , session_manager_(nullptr) {
}

void GroupHandler::set_session_manager(SessionManager* manager) {
    session_manager_ = manager;
}

std::string GroupHandler::handle_create_group(
    const std::string& creator_id,
    const std::string& group_name,
    const std::string& description) {
    
    auto group = group_repo_.create_group(group_name, description, creator_id);
    
    namespace json = boost::json;
    json::object response;
    
    if (group) {
        response["type"] = "group_created";
        response["group_id"] = group->group_id;
        response["group_name"] = group->group_name;
        response["description"] = group->description;
        response["created_by"] = group->created_by;
        
        Logger::get()->info("Group created: {} by {}", group_name, creator_id);
    } else {
        response["type"] = "error";
        response["message"] = "Failed to create group";
    }
    
    return json::serialize(response);
}

std::string GroupHandler::handle_add_member(
    const std::string& group_id,
    const std::string& user_id) {
    
    bool success = group_repo_.add_member(group_id, user_id);
    
    namespace json = boost::json;
    json::object response;
    
    if (success) {
        response["type"] = "member_added";
        response["group_id"] = group_id;
        response["user_id"] = user_id;
        
        // Notify the added member if online
        if (session_manager_ && session_manager_->is_user_online(user_id)) {
            json::object notification;
            notification["type"] = "added_to_group";
            notification["group_id"] = group_id;
            session_manager_->send_to_user(user_id, json::serialize(notification));
        }
    } else {
        response["type"] = "error";
        response["message"] = "Failed to add member";
    }
    
    return json::serialize(response);
}

std::string GroupHandler::handle_get_groups(const std::string& user_id) {
    auto groups = group_repo_.get_user_groups(user_id);
    
    namespace json = boost::json;
    json::object response;
    response["type"] = "groups";
    
    json::array groups_array;
    for (const auto& group : groups) {
        json::object group_obj;
        group_obj["group_id"] = group.group_id;
        group_obj["group_name"] = group.group_name;
        group_obj["description"] = group.description;
        group_obj["created_by"] = group.created_by;
        groups_array.push_back(group_obj);
    }
    
    response["groups"] = groups_array;
    return json::serialize(response);
}
