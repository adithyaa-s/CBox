// src/database/database.cpp
#include "database/database.hpp"
#include "utils/logger.hpp"

Database::Database(const std::string& connection_string) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connection_string);
        Logger::get()->info("Database connection established");
    } catch (const std::exception& e) {
        Logger::get()->error("Database connection failed: {}", e.what());
        throw;
    }
}

pqxx::connection& Database::get_connection() {
    return *conn_;
}

bool Database::test_connection() {
    try {
        pqxx::work txn(*conn_);
        txn.exec("SELECT 1");
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("Database test failed: {}", e.what());
        return false;
    }
}


