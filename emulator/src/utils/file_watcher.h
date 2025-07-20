#pragma once

#include <string>
#include <functional>

class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();
    
    void watchFile(const std::string& path, std::function<void()> callback);
    void stopWatching();
    void update();
    
private:
    std::string watched_path_;
    std::function<void()> callback_;
    time_t last_modified_ = 0;
    bool watching_ = false;
    
    time_t getFileModTime(const std::string& path);
};