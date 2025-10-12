#pragma once
#include "../database/message_repository.hpp"
#include <string>

class SessionManager;

class MessageHandler {
public:
    explicit MessageHandler(MessageRepository& msg_repo);
    
    void set_session_manager(SessionManager* manager);
    void handle_send_message(const std::string& sender_id,
                           const std::string& recipient_id,
                           const std::string& content);
    
    void handle_send_group_message(const std::string& sender_id,
                                  const std::string& group_id,
                                  const std::string& content);
    
    std::string handle_get_conversation(const std::string& user1_id,
                                       const std::string& user2_id);

private:
    MessageRepository& msg_repo_;
    SessionManager* session_manager_;
};