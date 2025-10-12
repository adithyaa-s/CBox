#pragma once
#include <pqxx/pqxx>
#include <memory>
#include <string>

class Database {
public:
    Database(const std::string& connection_string);
    pqxx::connection& get_connection();
    bool test_connection();

private:
    std::unique_ptr<pqxx::connection> conn_;
};