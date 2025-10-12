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
