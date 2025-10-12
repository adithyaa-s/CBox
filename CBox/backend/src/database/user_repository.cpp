#include "database/user_repository.hpp"
#include "utils/logger.hpp"

UserRepository::UserRepository(Database& db) : db_(db) {}

std::optional<User> UserRepository::create_user(
    const std::string& username,
    const std::string& email,
    const std::string& password_hash,
    const std::string& display_name) {
    
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "INSERT INTO users (username, email, password_hash, display_name) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING user_id, username, email, password_hash, display_name, status",
            username, email, password_hash, display_name
        );
        
        txn.commit();
        
        if (!result.empty()) {
            User user;
            user.user_id = result[0]["user_id"].as<std::string>();
            user.username = result[0]["username"].as<std::string>();
            user.email = result[0]["email"].as<std::string>();
            user.password_hash = result[0]["password_hash"].as<std::string>();
            user.display_name = result[0]["display_name"].as<std::string>();
            user.status = result[0]["status"].as<std::string>();
            return user;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to create user: {}", e.what());
    }
    
    return std::nullopt;
}

std::optional<User> UserRepository::get_user_by_username(const std::string& username) {
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT user_id, username, email, password_hash, display_name, status "
            "FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        if (!result.empty()) {
            User user;
            user.user_id = result[0]["user_id"].as<std::string>();
            user.username = result[0]["username"].as<std::string>();
            user.email = result[0]["email"].as<std::string>();
            user.password_hash = result[0]["password_hash"].as<std::string>();
            user.display_name = result[0]["display_name"].as<std::string>();
            user.status = result[0]["status"].as<std::string>();
            return user;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get user by username: {}", e.what());
    }
    
    return std::nullopt;
}

std::optional<User> UserRepository::get_user_by_id(const std::string& user_id) {
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT user_id, username, email, password_hash, display_name, status "
            "FROM users WHERE user_id = $1",
            user_id
        );
        
        txn.commit();
        
        if (!result.empty()) {
            User user;
            user.user_id = result[0]["user_id"].as<std::string>();
            user.username = result[0]["username"].as<std::string>();
            user.email = result[0]["email"].as<std::string>();
            user.password_hash = result[0]["password_hash"].as<std::string>();
            user.display_name = result[0]["display_name"].as<std::string>();
            user.status = result[0]["status"].as<std::string>();
            return user;
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to get user by id: {}", e.what());
    }
    
    return std::nullopt;
}

bool UserRepository::update_user_status(const std::string& user_id, const std::string& status) {
    try {
        pqxx::work txn(db_.get_connection());
        txn.exec_params(
            "UPDATE users SET status = $1, last_seen = CURRENT_TIMESTAMP WHERE user_id = $2",
            status, user_id
        );
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to update user status: {}", e.what());
        return false;
    }
}

std::vector<User> UserRepository::search_users(const std::string& query) {
    std::vector<User> users;
    try {
        pqxx::work txn(db_.get_connection());
        auto result = txn.exec_params(
            "SELECT user_id, username, email, password_hash, display_name, status "
            "FROM users WHERE username ILIKE $1 OR display_name ILIKE $1 LIMIT 20",
            "%" + query + "%"
        );
        
        txn.commit();
        
        for (const auto& row : result) {
            User user;
            user.user_id = row["user_id"].as<std::string>();
            user.username = row["username"].as<std::string>();
            user.email = row["email"].as<std::string>();
            user.password_hash = row["password_hash"].as<std::string>();
            user.display_name = row["display_name"].as<std::string>();
            user.status = row["status"].as<std::string>();
            users.push_back(user);
        }
    } catch (const std::exception& e) {
        Logger::get()->error("Failed to search users: {}", e.what());
    }
    
    return users;
}

// src/auth/auth_service.cpp
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
