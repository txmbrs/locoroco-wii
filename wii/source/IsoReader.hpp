#pragma once

#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>

class IsoReader {
public:
    struct IsoFileEntry {
        std::string name;
        uint32_t lba;
        uint32_t size;
        bool isDirectory;
    };

    IsoReader();
    ~IsoReader();

    bool Open(const std::string& path);
    void Close();

    bool IsOpen() const { return m_file != nullptr; }
    const std::string& GetPath() const { return m_isoPath; }

    bool FindFile(const std::string& path, IsoFileEntry& outEntry);
    bool ReadFile(const IsoFileEntry& entry, std::vector<uint8_t>& outBuffer);
    bool ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer);

    static std::string FindLocoRocoIso();

private:
    bool ReadDirectory(uint32_t dirLba, uint32_t dirSize, std::vector<IsoFileEntry>& outEntries);

    FILE* m_file = nullptr;
    std::string m_isoPath;
    uint32_t m_rootLba = 0;
    uint32_t m_rootSize = 0;
};
