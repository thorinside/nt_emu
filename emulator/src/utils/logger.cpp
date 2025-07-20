#include "logger.h"
#include <iostream>

void Logger::log(const std::string& message) {
    std::cout << "[LOG] " << message << std::endl;
}

void Logger::error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void Logger::info(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}