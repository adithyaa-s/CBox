#pragma once
#include "../database/database.hpp"
#include <string>

class SessionManager;

class FriendHandler {
public:
    explicit FriendHandler(Database& db);
    
    void set_session_manager(SessionManager* manager);
    std::string handle_send_friend_request(const std::string& sender_id,
                                          const std::string& receiver_username);
    
    std::string handle_accept_friend_request(const std::string& user_id,
                                            const std::string& request_id);
    
    std::string handle_reject_friend_request(const std::string& user_id,
                                            const std::string& request_id);
    
    std::string handle_get_friend_requests(const std::string& user_id);
    
    std::string handle_get_friends(const std::string& user_id);

private:
    Database& db_;
    SessionManager* session_manager_;
};