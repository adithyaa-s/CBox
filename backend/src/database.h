#pragma once
#include <string>
#include <libpq-fe.h>

class Database {
public:
    Database(const std::string& connInfo);
    ~Database();

    bool executeQuery(const std::string& query);
    PGresult* executeSelect(const std::string& query);

private:
    PGconn* conn;
};
