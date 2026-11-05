#include "IsoReader.hpp"
#include <cstring>
#include <algorithm>
#include <unistd.h>

static constexpr size_t SECTOR_SIZE = 2048;

IsoReader::IsoReader() {}

IsoReader::~IsoReader() {
    Close();
}

bool IsoReader::Open(const std::string& path) {
    Close();

    m_file = std::fopen(path.c_str(), "rb");
    if (!m_file) return false;

    // ISO9660 Primary Volume Descriptor is at Sector 16 (offset 16 * 2048 = 0x8000)
    if (std::fseek(m_file, 16 * SECTOR_SIZE, SEEK_SET) != 0) {
        Close();
        return false;
    }

    uint8_t pvd[SECTOR_SIZE];
    if (std::fread(pvd, 1, SECTOR_SIZE, m_file) != SECTOR_SIZE) {
        Close();
        return false;
    }

    // Check PVD magic: type 1, standard identifier 'CD001'
    if (pvd[0] != 1 || std::memcmp(pvd + 1, "CD001", 5) != 0) {
        Close();
        return false;
    }

    // Root directory record is at offset 156 in PVD
    const uint8_t* rootRecord = pvd + 156;
    m_rootLba = rootRecord[2] | (rootRecord[3] << 8) | (rootRecord[4] << 16) | (rootRecord[5] << 24);
    m_rootSize = rootRecord[10] | (rootRecord[11] << 8) | (rootRecord[12] << 16) | (rootRecord[13] << 24);

    m_isoPath = path;
    return true;
}

void IsoReader::Close() {
    if (m_file) {
        std::fclose(m_file);
        m_file = nullptr;
    }
    m_isoPath.clear();
    m_rootLba = 0;
    m_rootSize = 0;
}

bool IsoReader::ReadDirectory(uint32_t dirLba, uint32_t dirSize, std::vector<IsoFileEntry>& outEntries) {
    if (!m_file || dirSize == 0) return false;

    std::vector<uint8_t> buffer(dirSize);
    if (std::fseek(m_file, static_cast<long>(dirLba) * SECTOR_SIZE, SEEK_SET) != 0) return false;
    if (std::fread(buffer.data(), 1, dirSize, m_file) != dirSize) return false;

    size_t offset = 0;
    while (offset < dirSize) {
        uint8_t recordLen = buffer[offset];
        if (recordLen == 0) {
            // Sector boundary alignment
            size_t nextSector = ((offset / SECTOR_SIZE) + 1) * SECTOR_SIZE;
            offset = nextSector;
            continue;
        }

        if (offset + recordLen > dirSize) break;

        const uint8_t* rec = buffer.data() + offset;
        uint32_t lba = rec[2] | (rec[3] << 8) | (rec[4] << 16) | (rec[5] << 24);
        uint32_t size = rec[10] | (rec[11] << 8) | (rec[12] << 16) | (rec[13] << 24);
        uint8_t flags = rec[25];
        uint8_t nameLen = rec[32];

        std::string name;
        if (nameLen > 0 && (offset + 33 + nameLen <= dirSize)) {
            if (nameLen == 1 && rec[33] == 0) {
                name = ".";
            } else if (nameLen == 1 && rec[33] == 1) {
                name = "..";
            } else {
                name.assign(reinterpret_cast<const char*>(rec + 33), nameLen);
                // Strip trailing ISO version number ";1"
                size_t semi = name.find(';');
                if (semi != std::string::npos) name = name.substr(0, semi);
            }
        }

        if (!name.empty() && name != "." && name != "..") {
            IsoFileEntry entry;
            entry.name = name;
            entry.lba = lba;
            entry.size = size;
            entry.isDirectory = (flags & 0x02) != 0;
            outEntries.push_back(entry);
        }

        offset += recordLen;
    }

    return true;
}

bool IsoReader::FindFile(const std::string& path, IsoFileEntry& outEntry) {
    if (!IsOpen()) return false;

    // Split path by '/' or '\'
    std::vector<std::string> parts;
    size_t start = 0;
    while (start < path.size()) {
        size_t end = path.find_first_of("/\\", start);
        if (end == std::string::npos) end = path.size();
        if (end > start) {
            std::string part = path.substr(start, end - start);
            parts.push_back(part);
        }
        start = end + 1;
    }

    if (parts.empty()) return false;

    uint32_t currentLba = m_rootLba;
    uint32_t currentSize = m_rootSize;

    for (size_t i = 0; i < parts.size(); ++i) {
        std::vector<IsoFileEntry> entries;
        if (!ReadDirectory(currentLba, currentSize, entries)) return false;

        bool found = false;
        for (const auto& entry : entries) {
            if (strcasecmp(entry.name.c_str(), parts[i].c_str()) == 0) {
                if (i == parts.size() - 1) {
                    outEntry = entry;
                    return true;
                } else if (entry.isDirectory) {
                    currentLba = entry.lba;
                    currentSize = entry.size;
                    found = true;
                    break;
                }
            }
        }
        if (!found) return false;
    }

    return false;
}

bool IsoReader::ReadFile(const IsoFileEntry& entry, std::vector<uint8_t>& outBuffer) {
    if (!IsOpen() || entry.size == 0) return false;

    outBuffer.resize(entry.size);
    if (std::fseek(m_file, static_cast<long>(entry.lba) * SECTOR_SIZE, SEEK_SET) != 0) return false;
    if (std::fread(outBuffer.data(), 1, entry.size, m_file) != entry.size) return false;

    return true;
}

bool IsoReader::ReadFile(const std::string& path, std::vector<uint8_t>& outBuffer) {
    IsoFileEntry entry;
    if (!FindFile(path, entry)) return false;
    return ReadFile(entry, outBuffer);
}

std::string IsoReader::FindLocoRocoIso() {
    static const char* searchPaths[] = {
        "sd:/apps/LocoRocoWii/locoroco.iso",
        "sd:/apps/LocoRocoWii/game.iso",
        "sd:/apps/LocoRocoWii/LocoRoco.iso",
        "sd:/locoroco.iso",
        "usb:/apps/LocoRocoWii/locoroco.iso",
        "usb:/locoroco.iso",
        "locoroco.iso",
        "game.iso"
    };

    for (const char* p : searchPaths) {
        if (access(p, R_OK) == 0) {
            return std::string(p);
        }
    }

    return "";
}
