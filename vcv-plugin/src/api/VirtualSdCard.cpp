#include "VirtualSdCard.hpp"
#include <logger.hpp>
#include <system.hpp>
#include <cstring>
#include <cstdio>
#include <algorithm>

// Simple WAV header structure
#pragma pack(push, 1)
struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format chunk size (16 for PCM)
    uint16_t audioFormat;   // 1 = PCM, 3 = IEEE float
    uint16_t numChannels;   // 1 = mono, 2 = stereo
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Bytes per second
    uint16_t blockAlign;    // Bytes per sample frame
    uint16_t bitsPerSample; // Bits per sample
};
#pragma pack(pop)

VirtualSdCard& VirtualSdCard::getInstance() {
    static VirtualSdCard instance;
    return instance;
}

VirtualSdCard::VirtualSdCard()
    : m_mounted(false)
{
}

VirtualSdCard::~VirtualSdCard() {
}

void VirtualSdCard::setRootPath(const std::string& path) {
    if (m_rootPath != path) {
        m_rootPath = path;
        rescan();
    }
}

const std::string& VirtualSdCard::getRootPath() const {
    return m_rootPath;
}

bool VirtualSdCard::isMounted() const {
    return m_mounted && !m_rootPath.empty();
}

uint32_t VirtualSdCard::getNumSampleFolders() const {
    return static_cast<uint32_t>(m_folders.size());
}

void VirtualSdCard::getSampleFolderInfo(uint32_t folder, _NT_wavFolderInfo& info) const {
    if (folder >= m_folders.size()) {
        info.name = "";
        info.numSampleFiles = 0;
        return;
    }

    const auto& f = m_folders[folder];

    // Store name in persistent storage
    if (folder >= m_folderNameStorage.size()) {
        m_folderNameStorage.resize(folder + 1);
    }
    m_folderNameStorage[folder] = f.name;

    info.name = m_folderNameStorage[folder].c_str();
    info.numSampleFiles = static_cast<uint32_t>(f.files.size());
}

void VirtualSdCard::getSampleFileInfo(uint32_t folder, uint32_t sample, _NT_wavInfo& info) const {
    if (folder >= m_folders.size()) {
        info.name = "";
        info.numFrames = 0;
        info.sampleRate = 0;
        info.channels = kNT_WavMono;
        info.bits = kNT_WavBits16;
        return;
    }

    const auto& f = m_folders[folder];
    if (sample >= f.files.size()) {
        info.name = "";
        info.numFrames = 0;
        info.sampleRate = 0;
        info.channels = kNT_WavMono;
        info.bits = kNT_WavBits16;
        return;
    }

    const auto& file = f.files[sample];

    // Store name in persistent storage using a unique index
    size_t storageIndex = folder * 1000 + sample; // Simple index calculation
    if (storageIndex >= m_fileNameStorage.size()) {
        m_fileNameStorage.resize(storageIndex + 1);
    }
    m_fileNameStorage[storageIndex] = file.name;

    info.name = m_fileNameStorage[storageIndex].c_str();
    info.numFrames = file.numFrames;
    info.sampleRate = file.sampleRate;
    info.channels = file.channels;
    info.bits = file.bits;
}

void VirtualSdCard::rescan() {
    m_folders.clear();
    m_folderNameStorage.clear();
    m_fileNameStorage.clear();
    m_mounted = false;

    if (m_rootPath.empty()) {
        INFO("VirtualSdCard: No root path set");
        return;
    }

    // Check if samples directory exists
    std::string samplesPath = m_rootPath + "/samples";
    if (!rack::system::isDirectory(samplesPath)) {
        INFO("VirtualSdCard: Samples directory not found at %s", samplesPath.c_str());
        return;
    }

    INFO("VirtualSdCard: Scanning %s", samplesPath.c_str());

    // List subdirectories in samples folder
    std::vector<std::string> entries = rack::system::getEntries(samplesPath);
    std::sort(entries.begin(), entries.end());

    for (const auto& entry : entries) {
        if (!rack::system::isDirectory(entry)) {
            continue;
        }

        SampleFolder folder;
        folder.fullPath = entry;
        folder.name = rack::system::getFilename(entry);

        // Scan for WAV files in this folder
        std::vector<std::string> files = rack::system::getEntries(entry);
        std::sort(files.begin(), files.end());

        for (const auto& file : files) {
            if (rack::system::isDirectory(file)) {
                continue;
            }

            // Check if it's a WAV file
            std::string ext = rack::system::getExtension(file);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".wav") {
                continue;
            }

            WavFileInfo wavInfo;
            if (scanWavFile(file, wavInfo)) {
                folder.files.push_back(std::move(wavInfo));
            }
        }

        if (!folder.files.empty()) {
            INFO("VirtualSdCard: Found folder '%s' with %zu files",
                 folder.name.c_str(), folder.files.size());
            m_folders.push_back(std::move(folder));
        }
    }

    m_mounted = !m_folders.empty();
    INFO("VirtualSdCard: Mounted with %zu folders", m_folders.size());
}

bool VirtualSdCard::scanWavFile(const std::string& path, WavFileInfo& info) {
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        WARN("VirtualSdCard: Could not open %s", path.c_str());
        return false;
    }

    WavHeader header;
    if (std::fread(&header, sizeof(header), 1, fp) != 1) {
        std::fclose(fp);
        WARN("VirtualSdCard: Could not read header from %s", path.c_str());
        return false;
    }

    // Validate RIFF/WAVE
    if (std::memcmp(header.riff, "RIFF", 4) != 0 ||
        std::memcmp(header.wave, "WAVE", 4) != 0 ||
        std::memcmp(header.fmt, "fmt ", 4) != 0) {
        std::fclose(fp);
        WARN("VirtualSdCard: Invalid WAV header in %s", path.c_str());
        return false;
    }

    // Only support PCM (1) and IEEE float (3)
    if (header.audioFormat != 1 && header.audioFormat != 3) {
        std::fclose(fp);
        WARN("VirtualSdCard: Unsupported audio format %d in %s",
             header.audioFormat, path.c_str());
        return false;
    }

    // Skip any extra format bytes and find the data chunk
    uint32_t extraBytes = header.fmtSize - 16;
    if (extraBytes > 0) {
        std::fseek(fp, extraBytes, SEEK_CUR);
    }

    // Look for data chunk
    char chunkId[4];
    uint32_t chunkSize;
    bool foundData = false;

    while (!foundData && !std::feof(fp)) {
        if (std::fread(chunkId, 4, 1, fp) != 1) break;
        if (std::fread(&chunkSize, 4, 1, fp) != 1) break;

        if (std::memcmp(chunkId, "data", 4) == 0) {
            foundData = true;
        } else {
            // Skip this chunk
            std::fseek(fp, chunkSize, SEEK_CUR);
        }
    }

    std::fclose(fp);

    if (!foundData) {
        WARN("VirtualSdCard: No data chunk found in %s", path.c_str());
        return false;
    }

    // Populate info
    info.name = rack::system::getFilename(path);
    info.fullPath = path;
    info.sampleRate = header.sampleRate;

    // Calculate number of frames
    uint32_t bytesPerFrame = header.blockAlign;
    info.numFrames = chunkSize / bytesPerFrame;

    // Determine channels
    info.channels = (header.numChannels >= 2) ? kNT_WavStereo : kNT_WavMono;

    // Determine bit depth
    if (header.audioFormat == 3) {
        info.bits = kNT_WavBits32; // IEEE float
    } else {
        switch (header.bitsPerSample) {
            case 8:  info.bits = kNT_WavBits8; break;
            case 16: info.bits = kNT_WavBits16; break;
            case 24: info.bits = kNT_WavBits24; break;
            case 32: info.bits = kNT_WavBits32; break;
            default:
                WARN("VirtualSdCard: Unsupported bit depth %d in %s",
                     header.bitsPerSample, path.c_str());
                return false;
        }
    }

    INFO("VirtualSdCard: Scanned %s: %d frames, %d Hz, %s, %d bits",
         info.name.c_str(), info.numFrames, info.sampleRate,
         info.channels == kNT_WavStereo ? "stereo" : "mono",
         header.bitsPerSample);

    return true;
}

bool VirtualSdCard::readSampleFrames(const _NT_wavRequest& request) {
    if (request.folder >= m_folders.size()) {
        WARN("VirtualSdCard: Invalid folder index %d", request.folder);
        return false;
    }

    const auto& folder = m_folders[request.folder];
    if (request.sample >= folder.files.size()) {
        WARN("VirtualSdCard: Invalid sample index %d in folder %d",
             request.sample, request.folder);
        return false;
    }

    const auto& file = folder.files[request.sample];

    FILE* fp = std::fopen(file.fullPath.c_str(), "rb");
    if (!fp) {
        WARN("VirtualSdCard: Could not open %s for reading", file.fullPath.c_str());
        return false;
    }

    // Read header to find data chunk
    WavHeader header;
    if (std::fread(&header, sizeof(header), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }

    // Skip extra format bytes
    uint32_t extraBytes = header.fmtSize - 16;
    if (extraBytes > 0) {
        std::fseek(fp, extraBytes, SEEK_CUR);
    }

    // Find data chunk
    char chunkId[4];
    uint32_t chunkSize;
    bool foundData = false;

    while (!foundData && !std::feof(fp)) {
        if (std::fread(chunkId, 4, 1, fp) != 1) break;
        if (std::fread(&chunkSize, 4, 1, fp) != 1) break;

        if (std::memcmp(chunkId, "data", 4) == 0) {
            foundData = true;
        } else {
            std::fseek(fp, chunkSize, SEEK_CUR);
        }
    }

    if (!foundData) {
        std::fclose(fp);
        return false;
    }

    // Calculate source bytes per frame
    uint32_t srcBytesPerSample;
    switch (file.bits) {
        case kNT_WavBits8:  srcBytesPerSample = 1; break;
        case kNT_WavBits16: srcBytesPerSample = 2; break;
        case kNT_WavBits24: srcBytesPerSample = 3; break;
        case kNT_WavBits32: srcBytesPerSample = 4; break;
        default: srcBytesPerSample = 2; break;
    }
    uint32_t srcChannels = (file.channels == kNT_WavStereo) ? 2 : 1;
    uint32_t srcBytesPerFrame = srcBytesPerSample * srcChannels;

    // Seek to start offset
    if (request.startOffset > 0) {
        std::fseek(fp, request.startOffset * srcBytesPerFrame, SEEK_CUR);
    }

    // Check if we need conversion
    bool needsConversion = (file.channels != request.channels) ||
                          (file.bits != request.bits);

    if (!needsConversion) {
        // Direct read
        uint32_t dstBytesPerSample;
        switch (request.bits) {
            case kNT_WavBits8:  dstBytesPerSample = 1; break;
            case kNT_WavBits16: dstBytesPerSample = 2; break;
            case kNT_WavBits24: dstBytesPerSample = 3; break;
            case kNT_WavBits32: dstBytesPerSample = 4; break;
            default: dstBytesPerSample = 2; break;
        }
        uint32_t dstChannels = (request.channels == kNT_WavStereo) ? 2 : 1;
        uint32_t bytesToRead = request.numFrames * dstBytesPerSample * dstChannels;

        size_t bytesRead = std::fread(request.dst, 1, bytesToRead, fp);
        std::fclose(fp);

        bool success = (bytesRead == bytesToRead);
        if (request.callback) {
            request.callback(request.callbackData, success);
        }
        return true;
    }

    // Need to convert - read into temporary buffer
    size_t srcBufferSize = request.numFrames * srcBytesPerFrame;
    std::vector<uint8_t> srcBuffer(srcBufferSize);
    size_t bytesRead = std::fread(srcBuffer.data(), 1, srcBufferSize, fp);
    std::fclose(fp);

    if (bytesRead != srcBufferSize) {
        // Partial read - fill remaining with zeros
        std::memset(srcBuffer.data() + bytesRead, 0, srcBufferSize - bytesRead);
    }

    // Convert samples
    bool success = convertSamples(srcBuffer.data(), request.dst,
                                  request.numFrames,
                                  file.channels, request.channels,
                                  file.bits, request.bits);

    if (request.callback) {
        request.callback(request.callbackData, success);
    }

    return true;
}

bool VirtualSdCard::convertSamples(const void* src, void* dst,
                                   uint32_t numFrames,
                                   _NT_wavChannels srcChannels, _NT_wavChannels dstChannels,
                                   _NT_wavBits srcBits, _NT_wavBits dstBits) {
    // First convert source to float buffer
    uint32_t srcNumChannels = (srcChannels == kNT_WavStereo) ? 2 : 1;
    uint32_t dstNumChannels = (dstChannels == kNT_WavStereo) ? 2 : 1;

    std::vector<float> floatBuffer(numFrames * srcNumChannels);
    const uint8_t* srcBytes = static_cast<const uint8_t*>(src);

    // Convert source to float
    for (uint32_t i = 0; i < numFrames * srcNumChannels; i++) {
        float sample = 0.0f;
        switch (srcBits) {
            case kNT_WavBits8:
                sample = (static_cast<float>(srcBytes[i]) - 128.0f) / 128.0f;
                break;
            case kNT_WavBits16: {
                int16_t s = static_cast<int16_t>(srcBytes[i*2] | (srcBytes[i*2+1] << 8));
                sample = static_cast<float>(s) / 32768.0f;
                break;
            }
            case kNT_WavBits24: {
                int32_t s = srcBytes[i*3] | (srcBytes[i*3+1] << 8) | (srcBytes[i*3+2] << 16);
                if (s & 0x800000) s |= 0xFF000000; // Sign extend
                sample = static_cast<float>(s) / 8388608.0f;
                break;
            }
            case kNT_WavBits32: {
                float f;
                std::memcpy(&f, srcBytes + i*4, 4);
                sample = f;
                break;
            }
        }
        floatBuffer[i] = sample;
    }

    // Convert channels if needed
    std::vector<float> channelBuffer;
    float* channelData;
    if (srcNumChannels != dstNumChannels) {
        channelBuffer.resize(numFrames * dstNumChannels);
        if (srcNumChannels == 1 && dstNumChannels == 2) {
            // Mono to stereo
            for (uint32_t i = 0; i < numFrames; i++) {
                channelBuffer[i*2] = floatBuffer[i];
                channelBuffer[i*2+1] = floatBuffer[i];
            }
        } else {
            // Stereo to mono
            for (uint32_t i = 0; i < numFrames; i++) {
                channelBuffer[i] = (floatBuffer[i*2] + floatBuffer[i*2+1]) * 0.5f;
            }
        }
        channelData = channelBuffer.data();
    } else {
        channelData = floatBuffer.data();
    }

    // Convert float to destination format
    uint8_t* dstBytes = static_cast<uint8_t*>(dst);
    for (uint32_t i = 0; i < numFrames * dstNumChannels; i++) {
        float sample = std::max(-1.0f, std::min(1.0f, channelData[i]));
        switch (dstBits) {
            case kNT_WavBits8:
                dstBytes[i] = static_cast<uint8_t>((sample * 128.0f) + 128.0f);
                break;
            case kNT_WavBits16: {
                int16_t s = static_cast<int16_t>(sample * 32767.0f);
                dstBytes[i*2] = s & 0xFF;
                dstBytes[i*2+1] = (s >> 8) & 0xFF;
                break;
            }
            case kNT_WavBits24: {
                int32_t s = static_cast<int32_t>(sample * 8388607.0f);
                dstBytes[i*3] = s & 0xFF;
                dstBytes[i*3+1] = (s >> 8) & 0xFF;
                dstBytes[i*3+2] = (s >> 16) & 0xFF;
                break;
            }
            case kNT_WavBits32: {
                std::memcpy(dstBytes + i*4, &sample, 4);
                break;
            }
        }
    }

    return true;
}
