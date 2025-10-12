#pragma once
#include "database.hpp"
#include <string>
#include <vector>
#include <optional>

struct Message {
    std::string message_id;
    std::string sender_id;
    std::string recipient_id;
    std::string group_id;
    std::string content;
    std::string message_type;
    std::string created_at;
    bool is_read;
};

class MessageRepository {
public:
    explicit MessageRepository(Database& db);
    
    std::optional<Message> send_message(const std::string& sender_id,
                                       const std::string& recipient_id,
                                       const std::string& content,
                                       const std::string& message_type = "text");
    
    std::optional<Message> send_group_message(const std::string& sender_id,
                                             const std::string& group_id,
                                             const std::string& content,
                                             const std::string& message_type = "text");
    
    std::vector<Message> get_conversation(const std::string& user1_id,
                                         const std::string& user2_id,
                                         int limit = 50);
    
    std::vector<Message> get_group_messages(const std::string& group_id,
                                           int limit = 50);
    
    bool mark_message_read(const std::string& message_id);

private:
    Database& db_;
};