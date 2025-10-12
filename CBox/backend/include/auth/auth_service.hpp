#pragma once
#include "../database/user_repository.hpp"
#include <string>
#include <optional>

class AuthService {
public:
    explicit AuthService(UserRepository& user_repo);
    
    std::optional<std::string> register_user(const std::string& username,
                                            const std::string& email,
                                            const std::string& password,
                                            const std::string& display_name);
    
    std::optional<std::pair<std::string, std::string>> login(const std::string& username,
                                                             const std::string& password);
    
    std::optional<std::string> validate_token(const std::string& token);
    
    static std::string hash_password(const std::string& password);
    static bool verify_password(const std::string& password, const std::string& hash);

private:
    UserRepository& user_repo_;
};