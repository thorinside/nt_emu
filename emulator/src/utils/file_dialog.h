#pragma once

#include <string>
#include <vector>

class FileDialog {
public:
    // Show a file open dialog
    // Returns the selected file path, or empty string if cancelled
    static std::string openFile(
        const std::string& title = "Open File",
        const std::string& defaultPath = "",
        const std::vector<std::string>& filterPatterns = {},
        const std::string& filterDescription = ""
    );
    
    // Show a file save dialog
    // Returns the selected file path, or empty string if cancelled
    static std::string saveFile(
        const std::string& title = "Save File",
        const std::string& defaultPath = "",
        const std::vector<std::string>& filterPatterns = {},
        const std::string& filterDescription = ""
    );
};