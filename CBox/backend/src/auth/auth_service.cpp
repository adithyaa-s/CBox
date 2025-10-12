#include "auth/auth_service.hpp"
#include "auth/jwt_handler.hpp"
#include "utils/logger.hpp"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

AuthService::AuthService(UserRepository& user_repo) : user_repo_(user_repo) {}

std::string AuthService::hash_password(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool AuthService::verify_password(const std::string& password, const std::string& hash) {
    return hash_password(password) == hash;
}

std::optional<std::string> AuthService::register_user(
    const std::string& username,
    const std::string& email,
    const std::string& password,
    const std::string& display_name) {
    
    std::string password_hash = hash_password(password);
    auto user = user_repo_.create_user(username, email, password_hash, display_name);
    
    if (user) {
        Logger::get()->info("User registered: {}", username);
        return user->user_id;
    }
    
    return std::nullopt;
}

std::optional<std::pair<std::string, std::string>> AuthService::login(
    const std::string& username,
    const std::string& password) {
    
    auto user = user_repo_.get_user_by_username(username);
    if (!user) {
        Logger::get()->warn("Login failed: user not found - {}", username);
        return std::nullopt;
    }
    
    if (!verify_password(password, user->password_hash)) {
        Logger::get()->warn("Login failed: invalid password - {}", username);
        return std::nullopt;
    }
    
    std::string token = JWTHandler::generate_token(user->user_id);
    Logger::get()->info("User logged in: {}", username);
    
    return std::make_pair(user->user_id, token);
}

std::optional<std::string> AuthService::validate_token(const std::string& token) {
    return JWTHandler::validate_token(token);
}