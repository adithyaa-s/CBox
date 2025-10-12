#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>

class Session;
class MessageHandler;
class GroupHandler;
class FriendHandler;

class SessionManager {
public:
    SessionManager(MessageHandler& msg_handler,
                  GroupHandler& group_handler,
                  FriendHandler& friend_handler);
    
    void join(std::shared_ptr<Session> session, const std::string& user_id);
    void leave(const std::string& user_id);
    void send_to_user(const std::string& user_id, const std::string& message);
    void handle_client_message(const std::string& user_id, const std::string& message);
    bool is_user_online(const std::string& user_id);

private:
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    std::mutex mutex_;
    MessageHandler& msg_handler_;
    GroupHandler& group_handler_;
    FriendHandler& friend_handler_;
};