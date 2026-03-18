#pragma once

#include <cstddef>
#include <distingnt/microtuning.h>
#include <string>
#include <vector>

/**
 * VirtualScalaLibrary - Provides .scl (Scala) file access for nt_emu
 *
 * Follows the same singleton pattern as VirtualSdCard.
 * Reads .scl files from <virtualSdCardRoot>/scl/
 */
class VirtualScalaLibrary {
public:
    static VirtualScalaLibrary& getInstance();

    VirtualScalaLibrary(const VirtualScalaLibrary&) = delete;
    VirtualScalaLibrary& operator=(const VirtualScalaLibrary&) = delete;
    VirtualScalaLibrary(VirtualScalaLibrary&&) = delete;
    VirtualScalaLibrary& operator=(VirtualScalaLibrary&&) = delete;

    void rescan();

    uint32_t getNumScl() const;
    void getSclInfo(uint32_t index, _NT_sclInfo& info) const;
    bool readScl(_NT_sclRequest& request);

private:
    VirtualScalaLibrary();
    ~VirtualScalaLibrary();

    struct SclFileInfo {
        std::string name;       // Filename without extension
        std::string fullPath;
    };

    bool parseFile(const std::string& path, _NT_sclRequest& request);
    void ensureScanned();

    std::vector<SclFileInfo> m_files;
    bool m_scanned;
    std::string m_lastRootPath;

    mutable std::vector<std::string> m_nameStorage;
};
