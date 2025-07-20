#include "file_watcher.h"
#include <sys/stat.h>
#include <iostream>

FileWatcher::FileWatcher() {
}

FileWatcher::~FileWatcher() {
    stopWatching();
}

void FileWatcher::watchFile(const std::string& path, std::function<void()> callback) {
    watched_path_ = path;
    callback_ = callback;
    last_modified_ = getFileModTime(path);
    watching_ = true;
}

void FileWatcher::stopWatching() {
    watching_ = false;
    watched_path_.clear();
    callback_ = nullptr;
}

void FileWatcher::update() {
    if (!watching_ || watched_path_.empty()) return;
    
    time_t current_mod_time = getFileModTime(watched_path_);
    if (current_mod_time > last_modified_) {
        last_modified_ = current_mod_time;
        if (callback_) {
            callback_();
        }
    }
}

time_t FileWatcher::getFileModTime(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}