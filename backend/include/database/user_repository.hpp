#pragma once
#include "database.hpp"
#include <string>
#include <optional>
#include <vector>

struct User {
    std::string user_id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string display_name;
    std::string status;
};

class UserRepository {
public:
    explicit UserRepository(Database& db);
    
    std::optional<User> create_user(const std::string& username, 
                                    const std::string& email,
                                    const std::string& password_hash,
                                    const std::string& display_name);
    std::optional<User> get_user_by_username(const std::string& username);
    std::optional<User> get_user_by_id(const std::string& user_id);
    bool update_user_status(const std::string& user_id, const std::string& status);
    std::vector<User> search_users(const std::string& query);

private:
    Database& db_;
};