#include "file_dialog.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

std::string FileDialog::openFile(
    const std::string& title,
    const std::string& defaultPath,
    const std::vector<std::string>& filterPatterns,
    const std::string& filterDescription
) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        
        // Set panel properties
        [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        
        // Set default directory if provided
        if (!defaultPath.empty()) {
            NSString* defaultPathNS = [NSString stringWithUTF8String:defaultPath.c_str()];
            NSURL* defaultURL = [NSURL fileURLWithPath:defaultPathNS];
            [panel setDirectoryURL:defaultURL];
        }
        
        // Set file type filters if provided
        if (!filterPatterns.empty()) {
            NSMutableArray* allowedTypes = [NSMutableArray array];
            for (const auto& pattern : filterPatterns) {
                NSString* extension = [NSString stringWithUTF8String:pattern.c_str()];
                // Remove leading dot if present
                if ([extension hasPrefix:@"."]) {
                    extension = [extension substringFromIndex:1];
                }
                [allowedTypes addObject:extension];
            }
            [panel setAllowedFileTypes:allowedTypes];
        }
        
        // Show the panel
        NSModalResponse result = [panel runModal];
        
        if (result == NSModalResponseOK) {
            NSURL* selectedURL = [[panel URLs] firstObject];
            NSString* selectedPath = [selectedURL path];
            return std::string([selectedPath UTF8String]);
        }
        
        return ""; // User cancelled
    }
}

std::string FileDialog::saveFile(
    const std::string& title,
    const std::string& defaultPath,
    const std::vector<std::string>& filterPatterns,
    const std::string& filterDescription
) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        
        // Set panel properties
        [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        
        // Set default directory if provided
        if (!defaultPath.empty()) {
            NSString* defaultPathNS = [NSString stringWithUTF8String:defaultPath.c_str()];
            NSURL* defaultURL = [NSURL fileURLWithPath:defaultPathNS];
            [panel setDirectoryURL:defaultURL];
        }
        
        // Set file type filters if provided
        if (!filterPatterns.empty()) {
            NSMutableArray* allowedTypes = [NSMutableArray array];
            for (const auto& pattern : filterPatterns) {
                NSString* extension = [NSString stringWithUTF8String:pattern.c_str()];
                // Remove leading dot if present
                if ([extension hasPrefix:@"."]) {
                    extension = [extension substringFromIndex:1];
                }
                [allowedTypes addObject:extension];
            }
            [panel setAllowedFileTypes:allowedTypes];
        }
        
        // Show the panel
        NSModalResponse result = [panel runModal];
        
        if (result == NSModalResponseOK) {
            NSURL* selectedURL = [panel URL];
            NSString* selectedPath = [selectedURL path];
            return std::string([selectedPath UTF8String]);
        }
        
        return ""; // User cancelled
    }
}

#else
// Non-macOS implementation would go here
// For now, just return empty string
std::string FileDialog::openFile(
    const std::string& title,
    const std::string& defaultPath,
    const std::vector<std::string>& filterPatterns,
    const std::string& filterDescription
) {
    return "";
}

std::string FileDialog::saveFile(
    const std::string& title,
    const std::string& defaultPath,
    const std::vector<std::string>& filterPatterns,
    const std::string& filterDescription
) {
    return "";
}
#endif