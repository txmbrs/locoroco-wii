#include "AssetManager.hpp"
#include <cstdio>
#include <unistd.h>

std::unique_ptr<IsoReader> AssetManager::s_isoReader = nullptr;
bool AssetManager::s_hasIso = false;
std::string AssetManager::s_isoPath = "";

void AssetManager::Init() {
    s_isoReader = std::make_unique<IsoReader>();
    s_isoPath = IsoReader::FindLocoRocoIso();

    if (!s_isoPath.empty()) {
        if (s_isoReader->Open(s_isoPath)) {
            s_hasIso = true;
            std::printf("[AssetManager] Successfully mounted PSP ISO: %s\n", s_isoPath.c_str());
        } else {
            std::printf("[AssetManager] Found ISO at %s but failed to parse ISO9660\n", s_isoPath.c_str());
        }
    } else {
        std::printf("[AssetManager] No external locoroco.iso found on SD/USB - using built-in high fidelity assets\n");
    }
}

bool AssetManager::HasOriginalIso() {
    return s_hasIso;
}

const std::string& AssetManager::GetIsoPath() {
    return s_isoPath;
}

IsoReader* AssetManager::GetIsoReader() {
    return s_isoReader.get();
}

bool AssetManager::LoadFile(const std::string& relativePath, std::vector<uint8_t>& outBuffer) {
    // 1. If ISO is mounted, check inside ISO PSP_GAME/USRDIR
    if (s_hasIso && s_isoReader) {
        std::string pspPath = "PSP_GAME/USRDIR/" + relativePath;
        if (s_isoReader->ReadFile(pspPath, outBuffer)) {
            return true;
        }
        if (s_isoReader->ReadFile(relativePath, outBuffer)) {
            return true;
        }
    }

    // 2. Check local extracted files on SD
    static const char* searchPrefixes[] = {
        "sd:/apps/LocoRocoWii/assets/",
        "sd:/apps/LocoRocoWii/data/",
        "sd:/apps/LocoRocoWii/",
        "assets/",
        "data/"
    };

    for (const char* prefix : searchPrefixes) {
        std::string full = std::string(prefix) + relativePath;
        FILE* f = std::fopen(full.c_str(), "rb");
        if (f) {
            std::fseek(f, 0, SEEK_END);
            size_t sz = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);
            outBuffer.resize(sz);
            std::fread(outBuffer.data(), 1, sz, f);
            std::fclose(f);
            return true;
        }
    }

    return false;
}
