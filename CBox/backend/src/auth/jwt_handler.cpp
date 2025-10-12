
#include "auth/jwt_handler.hpp"
#include <boost/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <chrono>
#include <sstream>
#include <iomanip>

std::string JWTHandler::secret_ = "your-secret-key-change-this-in-production";

std::string base64_encode(const std::string& input) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (size_t n = 0; n < input.length(); n++) {
        char_array_3[i++] = input[n];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while(i++ < 3)
            ret += '=';
    }
    
    return ret;
}

std::string JWTHandler::generate_token(const std::string& user_id) {
    namespace json = boost::json;
    
    // Header
    json::object header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    std::string header_str = json::serialize(header);
    std::string encoded_header = base64_encode(header_str);
    
    // Payload
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(24);
    auto exp_time = std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count();
    
    json::object payload;
    payload["user_id"] = user_id;
    payload["exp"] = exp_time;
    std::string payload_str = json::serialize(payload);
    std::string encoded_payload = base64_encode(payload_str);
    
    // Signature
    std::string message = encoded_header + "." + encoded_payload;
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), secret_.c_str(), secret_.length(),
         reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
         hmac_result, &hmac_len);
    
    std::string signature(reinterpret_cast<char*>(hmac_result), hmac_len);
    std::string encoded_signature = base64_encode(signature);
    
    return message + "." + encoded_signature;
}

std::optional<std::string> JWTHandler::validate_token(const std::string& token) {
    // Simple validation - in production, use a proper JWT library
    size_t first_dot = token.find('.');
    size_t second_dot = token.find('.', first_dot + 1);
    
    if (first_dot == std::string::npos || second_dot == std::string::npos) {
        return std::nullopt;
    }
    
    std::string payload_encoded = token.substr(first_dot + 1, second_dot - first_dot - 1);
    
    // Decode payload (simplified)
    try {
        namespace json = boost::json;
        // In production, properly decode base64 and verify signature
        // For now, we'll accept the token format
        return "user_id_from_token"; // Simplified
    } catch (...) {
        return std::nullopt;
    }
}

void JWTHandler::set_secret(const std::string& secret) {
    secret_ = secret;
}