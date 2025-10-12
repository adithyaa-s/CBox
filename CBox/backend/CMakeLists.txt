cmake_minimum_required(VERSION 3.15)
project(ChatBackend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Boost REQUIRED COMPONENTS system json)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(libpqxx REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/database/database.cpp
    src/database/user_repository.cpp
    src/database/message_repository.cpp
    src/database/group_repository.cpp
    src/auth/auth_service.cpp
    src/auth/jwt_handler.cpp
    src/server/websocket_server.cpp
    src/server/session.cpp
    src/server/session_manager.cpp
    src/handlers/message_handler.cpp
    src/handlers/group_handler.cpp
    src/handlers/friend_handler.cpp
    src/utils/logger.cpp
)

# Create executable
add_executable(chat_server ${SOURCES})

# Include directories
target_include_directories(chat_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(chat_server PRIVATE
    Boost::system
    Boost::json
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    libpqxx::pqxx
    pthread
)

# Compiler warnings
if(MSVC)
    target_compile_options(chat_server PRIVATE /W4)
else()
    target_compile_options(chat_server PRIVATE -Wall -Wextra -Wpedantic)
endif()