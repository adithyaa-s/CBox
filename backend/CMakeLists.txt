cmake_minimum_required(VERSION 3.15)
project(ChatBackend)

set(CMAKE_CXX_STANDARD 17)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system thread)

# PostgreSQL
find_package(PostgreSQL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS} include)

add_executable(chat-backend
    src/main.cpp
    src/server.cpp
    src/client_session.cpp
    src/database.cpp
)

target_link_libraries(chat-backend
    ${Boost_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    pthread
)
