#ifndef RAW_CONVERT_H
#define RAW_CONVERT_H

#include <vector>
#include <cstdint>
#include <string>

struct RawResult {
    std::vector<uint8_t> data;
    uint8_t type;
    uint32_t width;
    uint32_t height;
    uint32_t pixels;
    size_t jpgSize;
    size_t rawSize;
};

bool jpgToRaw(const std::string& inputFile, RawResult& out);
bool convertImagesToRaw(const std::string& inputDir, const std::string& outputDir);
bool extractPixelData(const std::string& rawPath, std::vector<uint8_t>& pixelData, uint8_t& type, uint32_t& pixels);

#endif