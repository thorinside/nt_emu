#pragma once

#include <distingnt/wav.h>
#include <string>
#include <vector>
#include <memory>

/**
 * VirtualSdCard - Emulates the SD card sample folder structure for nt_emu
 *
 * Maps a local filesystem folder to the virtual SD card root.
 * Sample folders are expected under <root>/samples/
 * Each subfolder is a "sample folder" containing .wav files.
 */
class VirtualSdCard {
public:
    // Singleton access
    static VirtualSdCard& getInstance();

    // Disable copy/move
    VirtualSdCard(const VirtualSdCard&) = delete;
    VirtualSdCard& operator=(const VirtualSdCard&) = delete;
    VirtualSdCard(VirtualSdCard&&) = delete;
    VirtualSdCard& operator=(VirtualSdCard&&) = delete;

    // Path management
    void setRootPath(const std::string& path);
    const std::string& getRootPath() const;
    bool isMounted() const;

    // Folder enumeration
    uint32_t getNumSampleFolders() const;
    void getSampleFolderInfo(uint32_t folder, _NT_wavFolderInfo& info) const;

    // File enumeration
    void getSampleFileInfo(uint32_t folder, uint32_t sample, _NT_wavInfo& info) const;

    // Sample reading
    bool readSampleFrames(const _NT_wavRequest& request);

    // Force re-scan of folders (call after changing root path)
    void rescan();

private:
    VirtualSdCard();
    ~VirtualSdCard();

    // Internal data structures
    struct WavFileInfo {
        std::string name;           // Filename without path
        std::string fullPath;       // Full path to file
        uint32_t numFrames;
        uint32_t sampleRate;
        _NT_wavChannels channels;
        _NT_wavBits bits;
    };

    struct SampleFolder {
        std::string name;           // Folder name
        std::string fullPath;       // Full path to folder
        std::vector<WavFileInfo> files;
    };

    // Scan a WAV file and populate its info
    bool scanWavFile(const std::string& path, WavFileInfo& info);

    // Convert between formats during reading
    bool convertSamples(const void* src, void* dst,
                       uint32_t numFrames,
                       _NT_wavChannels srcChannels, _NT_wavChannels dstChannels,
                       _NT_wavBits srcBits, _NT_wavBits dstBits);

    std::string m_rootPath;
    std::vector<SampleFolder> m_folders;
    bool m_mounted;

    // Static storage for folder/file names (API returns const char*)
    mutable std::vector<std::string> m_folderNameStorage;
    mutable std::vector<std::string> m_fileNameStorage;
};
