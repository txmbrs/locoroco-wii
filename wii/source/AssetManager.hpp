#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "IsoReader.hpp"

class AssetManager {
public:
    static void Init();
    static bool HasOriginalIso();
    static const std::string& GetIsoPath();

    static bool LoadFile(const std::string& relativePath, std::vector<uint8_t>& outBuffer);
    static IsoReader* GetIsoReader();

private:
    static std::unique_ptr<IsoReader> s_isoReader;
    static bool s_hasIso;
    static std::string s_isoPath;
};
