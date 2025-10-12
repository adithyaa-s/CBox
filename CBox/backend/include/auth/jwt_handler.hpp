#pragma once
#include <string>
#include <optional>

class JWTHandler {
public:
    static std::string generate_token(const std::string& user_id);
    static std::optional<std::string> validate_token(const std::string& token);
    static void set_secret(const std::string& secret);

private:
    static std::string secret_;
};