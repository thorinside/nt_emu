#include "config.h"
#include <iostream>
#include <fstream>
#include <filesystem>

Config::Config() {
    config_file_path_ = getDefaultConfigPath();
    setDefaults();
}

Config::~Config() {
}

std::string Config::getDefaultConfigPath() {
    // Use platform-appropriate config directory
    std::filesystem::path config_dir;
    
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        config_dir = std::filesystem::path(appdata) / "DistingNTEmulator";
    } else {
        config_dir = std::filesystem::current_path();
    }
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home) {
        config_dir = std::filesystem::path(home) / "Library" / "Application Support" / "DistingNTEmulator";
    } else {
        config_dir = std::filesystem::current_path();
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        config_dir = std::filesystem::path(home) / ".config" / "DistingNTEmulator";
    } else {
        config_dir = std::filesystem::current_path();
    }
#endif
    
    // Create config directory if it doesn't exist
    try {
        std::filesystem::create_directories(config_dir);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create config directory: " << e.what() << std::endl;
        // Fall back to current directory
        config_dir = std::filesystem::current_path();
    }
    
    return (config_dir / "config.json").string();
}

void Config::setDefaults() {
    audio_config_ = AudioConfiguration(); // Use default constructor values
}

bool Config::load() {
    return load(config_file_path_);
}

bool Config::save() {
    return save(config_file_path_);
}

bool Config::load(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // Config file doesn't exist - use defaults
            std::cout << "Config file not found, using defaults: " << filename << std::endl;
            setDefaults();
            return true;
        }
        
        nlohmann::json j;
        file >> j;
        
        // Parse JSON into configuration
        if (j.contains("audio")) {
            audio_config_ = j["audio"].get<AudioConfiguration>();
        } else {
            // Legacy format or missing audio section - use defaults
            audio_config_ = AudioConfiguration();
        }
        
        std::cout << "Configuration loaded successfully from: " << filename << std::endl;
        std::cout << "Audio device IDs: input=" << audio_config_.input_device_id 
                  << " output=" << audio_config_.output_device_id << std::endl;
        std::cout << "Buffer size: " << audio_config_.buffer_size 
                  << " Sample rate: " << audio_config_.sample_rate << std::endl;
        
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        last_error_ = std::string("JSON parsing error: ") + e.what();
        std::cerr << "Failed to parse config file: " << last_error_ << std::endl;
        setDefaults(); // Use defaults on parse error
        return false;
    } catch (const std::exception& e) {
        last_error_ = std::string("Config loading error: ") + e.what();
        std::cerr << "Failed to load config: " << last_error_ << std::endl;
        setDefaults(); // Use defaults on error
        return false;
    }
}

bool Config::save(const std::string& filename) {
    try {
        // Create directory if it doesn't exist
        std::filesystem::path file_path(filename);
        std::filesystem::create_directories(file_path.parent_path());
        
        nlohmann::json j;
        j["version"] = "1.0";
        j["audio"] = audio_config_;
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Failed to open config file for writing: " + filename;
            std::cerr << last_error_ << std::endl;
            return false;
        }
        
        file << j.dump(4); // Pretty print with 4-space indentation
        file.close();
        
        std::cout << "Configuration saved successfully to: " << filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Config saving error: ") + e.what();
        std::cerr << "Failed to save config: " << last_error_ << std::endl;
        return false;
    }
}

void Config::setAudioConfig(const AudioConfiguration& config) {
    audio_config_ = config;
}