#include "database.h"
#include <iostream>

Database::Database(const std::string& connInfo) {
    conn = PQconnectdb(connInfo.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
        exit(1);
    }
}

Database::~Database() {
    PQfinish(conn);
}

bool Database::executeQuery(const std::string& query) {
    PGresult* res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

PGresult* Database::executeSelect(const std::string& query) {
    PGresult* res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Select failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return nullptr;
    }
    return res;
}
