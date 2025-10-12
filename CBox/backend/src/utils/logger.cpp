// src/utils/logger.cpp
#include "utils/logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/chat_server.log", 1024 * 1024 * 10, 3);
        file_sink->set_level(spdlog::level::info);
        
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        logger_ = std::make_shared<spdlog::logger>("chat_server", sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::debug);
        logger_->flush_on(spdlog::level::info);
        
        spdlog::register_logger(logger_);
        
        logger_->info("Logger initialized");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init();
    }
    return logger_;
}