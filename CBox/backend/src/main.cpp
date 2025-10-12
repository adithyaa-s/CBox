#include "database/database.hpp"
#include "database/user_repository.hpp"
#include "database/message_repository.hpp"
#include "database/group_repository.hpp"
#include "auth/auth_service.hpp"
#include "auth/jwt_handler.hpp"
#include "server/websocket_server.hpp"
#include "server/session_manager.hpp"
#include "handlers/message_handler.hpp"
#include "handlers/group_handler.hpp"
#include "handlers/friend_handler.hpp"
#include "utils/logger.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <csignal>
#include <atomic>

// Global flag for graceful shutdown
std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received. Cleaning up...\n";
        shutdown_requested = true;
    }
}

int main(int argc, char* argv[]) {
    try {
        // Initialize logger first
        Logger::init();
        Logger::get()->info("==============================================");
        Logger::get()->info("  Chat Server Starting...");
        Logger::get()->info("==============================================");
        
        // ==================== CONFIGURATION ====================
        // TODO: Move these to config file or environment variables in production
        
        // Database configuration
        const std::string db_host = "localhost";
        const std::string db_port = "5432";
        const std::string db_name = "chat_app";
        const std::string db_user = "chatuser";
        const std::string db_password = "chatpassword";
        
        // Build connection string
        const std::string db_connection = 
            "host=" + db_host + " " +
            "port=" + db_port + " " +
            "dbname=" + db_name + " " +
            "user=" + db_user + " " +
            "password=" + db_password;
        
        // JWT secret (CHANGE THIS IN PRODUCTION!)
        const std::string jwt_secret = "your-super-secret-jwt-key-change-in-production-min-32-chars";
        
        // Server configuration
        const std::string host = "0.0.0.0";  // Listen on all interfaces
        const unsigned short port = 8080;     // WebSocket port
        
        // Thread pool configuration
        int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;  // Fallback if detection fails
        
        Logger::get()->info("Configuration:");
        Logger::get()->info("  - Database: {}@{}/{}", db_user, db_host, db_name);
        Logger::get()->info("  - Server: {}:{}", host, port);
        Logger::get()->info("  - Threads: {}", num_threads);
        
        // ==================== SET JWT SECRET ====================
        JWTHandler::set_secret(jwt_secret);
        Logger::get()->info("JWT secret configured");
        
        // ==================== DATABASE INITIALIZATION ====================
        Logger::get()->info("Connecting to database...");
        Database db(db_connection);
        
        if (!db.test_connection()) {
            Logger::get()->error("Database connection test failed!");
            Logger::get()->error("Please ensure PostgreSQL is running and database exists.");
            Logger::get()->error("Run: psql -U chatuser -d chat_app -h localhost < schema.sql");
            return 1;
        }
        Logger::get()->info("Database connected successfully ✓");
        
        // ==================== INITIALIZE REPOSITORIES ====================
        Logger::get()->info("Initializing repositories...");
        UserRepository user_repo(db);
        MessageRepository msg_repo(db);
        GroupRepository group_repo(db);
        Logger::get()->info("Repositories initialized ✓");
        
        // ==================== INITIALIZE SERVICES ====================
        Logger::get()->info("Initializing services...");
        AuthService auth_service(user_repo);
        Logger::get()->info("Auth service initialized ✓");
        
        // ==================== INITIALIZE HANDLERS ====================
        Logger::get()->info("Initializing handlers...");
        MessageHandler msg_handler(msg_repo);
        GroupHandler group_handler(group_repo);
        FriendHandler friend_handler(db);
        Logger::get()->info("Handlers initialized ✓");
        
        // ==================== INITIALIZE SESSION MANAGER ====================
        Logger::get()->info("Initializing session manager...");
        SessionManager session_manager(msg_handler, group_handler, friend_handler);
        Logger::get()->info("Session manager initialized ✓");
        
        // ==================== INITIALIZE IO CONTEXT ====================
        Logger::get()->info("Initializing I/O context with {} threads...", num_threads);
        boost::asio::io_context ioc{num_threads};
        
        // ==================== CREATE WEBSOCKET SERVER ====================
        Logger::get()->info("Creating WebSocket server...");
        auto endpoint = boost::asio::ip::tcp::endpoint{
            boost::asio::ip::make_address(host), 
            port
        };
        
        WebSocketServer server(ioc, endpoint, session_manager);
        server.run();
        
        Logger::get()->info("==============================================");
        Logger::get()->info("🚀 Server started successfully!");
        Logger::get()->info("==============================================");
        Logger::get()->info("WebSocket server listening on ws://{}:{}", host, port);
        Logger::get()->info("Using {} worker threads", num_threads);
        Logger::get()->info("Press Ctrl+C to stop the server");
        Logger::get()->info("==============================================");
        
        // ==================== SETUP SIGNAL HANDLERS ====================
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        // ==================== RUN IO CONTEXT ON MULTIPLE THREADS ====================
        std::vector<std::thread> threads;
        threads.reserve(num_threads - 1);
        
        // Spawn worker threads
        for (int i = 0; i < num_threads - 1; ++i) {
            threads.emplace_back([&ioc, i] {
                Logger::get()->debug("Worker thread {} started", i + 1);
                try {
                    ioc.run();
                    Logger::get()->debug("Worker thread {} stopped", i + 1);
                } catch (const std::exception& e) {
                    Logger::get()->error("Worker thread {} error: {}", i + 1, e.what());
                }
            });
        }
        
        // Run on main thread as well
        Logger::get()->debug("Main I/O thread started");
        try {
            ioc.run();
            Logger::get()->debug("Main I/O thread stopped");
        } catch (const std::exception& e) {
            Logger::get()->error("Main I/O thread error: {}", e.what());
        }
        
        // ==================== WAIT FOR ALL THREADS ====================
        Logger::get()->info("Waiting for worker threads to finish...");
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        Logger::get()->info("==============================================");
        Logger::get()->info("Server stopped gracefully");
        Logger::get()->info("==============================================");
        
    } catch (const std::exception& e) {
        // Log fatal error
        if (Logger::get()) {
            Logger::get()->critical("Fatal error: {}", e.what());
        } else {
            std::cerr << "Fatal error: " << e.what() << std::endl;
        }
        
        std::cerr << "\n==============================================\n";
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << "==============================================\n";
        std::cerr << "\nTroubleshooting steps:\n";
        std::cerr << "1. Check PostgreSQL is running:\n";
        std::cerr << "   sudo systemctl status postgresql\n";
        std::cerr << "2. Verify database exists:\n";
        std::cerr << "   psql -U chatuser -d chat_app -h localhost\n";
        std::cerr << "3. Ensure port 8080 is not in use:\n";
        std::cerr << "   sudo lsof -i :8080\n";
        std::cerr << "4. Check logs for details:\n";
        std::cerr << "   tail -f logs/chat_server.log\n";
        std::cerr << "==============================================\n";
        
        return 1;
    }
    
    return 0;
}