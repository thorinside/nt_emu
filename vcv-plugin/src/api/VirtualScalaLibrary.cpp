#include "VirtualScalaLibrary.hpp"
#include "VirtualSdCard.hpp"
#include <logger.hpp>
#include <system.hpp>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sstream>

VirtualScalaLibrary& VirtualScalaLibrary::getInstance() {
    static VirtualScalaLibrary instance;
    return instance;
}

VirtualScalaLibrary::VirtualScalaLibrary()
    : m_scanned(false)
{
}

VirtualScalaLibrary::~VirtualScalaLibrary() {
}

void VirtualScalaLibrary::ensureScanned() {
    const std::string& rootPath = VirtualSdCard::getInstance().getRootPath();
    if (!m_scanned || rootPath != m_lastRootPath) {
        m_lastRootPath = rootPath;
        rescan();
    }
}

void VirtualScalaLibrary::rescan() {
    m_files.clear();
    m_nameStorage.clear();
    m_scanned = true;

    const std::string& rootPath = VirtualSdCard::getInstance().getRootPath();
    if (rootPath.empty()) {
        INFO("VirtualScalaLibrary: No root path set");
        return;
    }

    std::string sclPath = rootPath + "/scl";
    if (!rack::system::isDirectory(sclPath)) {
        INFO("VirtualScalaLibrary: scl directory not found at %s", sclPath.c_str());
        return;
    }

    INFO("VirtualScalaLibrary: Scanning %s", sclPath.c_str());

    std::vector<std::string> entries = rack::system::getEntries(sclPath);
    std::sort(entries.begin(), entries.end());

    for (const auto& entry : entries) {
        if (rack::system::isDirectory(entry)) {
            continue;
        }

        std::string ext = rack::system::getExtension(entry);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".scl") {
            continue;
        }

        SclFileInfo info;
        info.fullPath = entry;
        // Get filename without extension
        std::string filename = rack::system::getFilename(entry);
        size_t dotPos = filename.rfind('.');
        if (dotPos != std::string::npos) {
            info.name = filename.substr(0, dotPos);
        } else {
            info.name = filename;
        }

        m_files.push_back(std::move(info));
    }

    INFO("VirtualScalaLibrary: Found %zu .scl files", m_files.size());
}

uint32_t VirtualScalaLibrary::getNumScl() const {
    const_cast<VirtualScalaLibrary*>(this)->ensureScanned();
    return static_cast<uint32_t>(m_files.size());
}

void VirtualScalaLibrary::getSclInfo(uint32_t index, _NT_sclInfo& info) const {
    const_cast<VirtualScalaLibrary*>(this)->ensureScanned();

    if (index >= m_files.size()) {
        info.name = "";
        return;
    }

    if (index >= m_nameStorage.size()) {
        m_nameStorage.resize(index + 1);
    }
    m_nameStorage[index] = m_files[index].name;
    info.name = m_nameStorage[index].c_str();
}

bool VirtualScalaLibrary::readScl(_NT_sclRequest& request) {
    ensureScanned();

    if (request.index >= m_files.size()) {
        WARN("VirtualScalaLibrary: Invalid index %d", request.index);
        request.error = true;
        return false;
    }

    const auto& file = m_files[request.index];

    if (!parseFile(file.fullPath, request)) {
        request.error = true;
        return false;
    }

    // Copy name to request buffer
    if (request.nameBuffer && request.nameBufferSize > 0) {
        std::strncpy(request.nameBuffer, file.name.c_str(), request.nameBufferSize - 1);
        request.nameBuffer[request.nameBufferSize - 1] = '\0';
    }

    request.error = false;

    if (request.callback) {
        request.callback(request.callbackData);
    }

    return true;
}

bool VirtualScalaLibrary::parseFile(const std::string& path, _NT_sclRequest& request) {
    std::ifstream file(path);
    if (!file.is_open()) {
        WARN("VirtualScalaLibrary: Could not open %s", path.c_str());
        return false;
    }

    std::string line;
    int state = 0;  // 0=looking for description, 1=looking for count, 2=reading notes
    uint32_t expectedNotes = 0;
    uint32_t notesParsed = 0;

    while (std::getline(file, line)) {
        // Skip comment lines
        if (!line.empty() && line[0] == '!') {
            continue;
        }

        // Skip blank lines
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        if (state == 0) {
            // First non-comment line is description
            if (request.descriptionBuffer && request.descriptionBufferSize > 0) {
                std::strncpy(request.descriptionBuffer, line.c_str(), request.descriptionBufferSize - 1);
                request.descriptionBuffer[request.descriptionBufferSize - 1] = '\0';
            }
            state = 1;
        } else if (state == 1) {
            // Second non-comment line is note count
            expectedNotes = static_cast<uint32_t>(std::atoi(line.c_str()));
            state = 2;
        } else if (state == 2) {
            // Note definitions
            if (notesParsed >= request.maxNotes) {
                break;
            }

            // Trim leading whitespace
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) {
                continue;
            }
            std::string trimmed = line.substr(start);

            // Check if it's a ratio (contains '/')
            size_t slashPos = trimmed.find('/');
            if (slashPos != std::string::npos) {
                // Ratio format: numerator/denominator
                int32_t num = std::atoi(trimmed.c_str());
                int32_t denom = std::atoi(trimmed.c_str() + slashPos + 1);
                if (denom > 0) {
                    request.notes[notesParsed].numeratorValue = num;
                    request.notes[notesParsed].denominatorValue = -static_cast<int32_t>(denom);
                    notesParsed++;
                }
            } else {
                // Cents format (contains '.') or integer cents
                double cents = std::atof(trimmed.c_str());
                request.notes[notesParsed].octaves = cents / 1200.0;
                notesParsed++;
            }

            if (notesParsed >= expectedNotes) {
                break;
            }
        }
    }

    request.numNotes = notesParsed;

    INFO("VirtualScalaLibrary: Parsed %s: %d notes", path.c_str(), notesParsed);
    return true;
}
