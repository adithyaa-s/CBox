#include "server.h"
#include "database.h"
#include <boost/asio.hpp>
#include <iostream>

int main() {
    try {
        boost::asio::io_context io_context;

        Server server(io_context, 12345);
        std::cout << "Chat server running on port 12345..." << std::endl;

        // Example DB connection
        Database db("host=localhost port=5432 dbname=chat user=postgres password=yourpassword");
        db.executeQuery("CREATE TABLE IF NOT EXISTS messages (id SERIAL PRIMARY KEY, user TEXT, message TEXT);");

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
