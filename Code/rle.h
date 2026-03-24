#ifndef RLE_H
#define RLE_H

#include <vector>
#include <cstdint>
#include <string>

struct RLEHeader {
    uint8_t Ms;
    uint8_t Mc;
    uint64_t originalSize;
    uint64_t encodedSize;
};

struct CompressionStats {
    std::string filename;
    size_t originalSize = 0;
    size_t encodedSize = 0;
    double compressionRatio = 0.0;
    double compressionFactor = 0.0;
    double savingsPercent = 0.0;
    std::string analysis;
};

class RLE {
public:
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input,
        uint8_t Ms,
        uint8_t Mc);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input,
        uint8_t Ms,
        uint8_t Mc);

    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        uint8_t Ms,
        uint8_t Mc,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static CompressionStats analyzeFile(const std::string& inputPath,
        uint8_t Ms,
        uint8_t Mc,
        std::string& errorMsg);

    static CompressionStats compressFile(const std::string& inputPath,
        const std::string& outputPath,
        uint8_t Ms,
        uint8_t Mc,
        std::string& errorMsg);

    static double getCompressionRatio(size_t originalSize, size_t encodedSize);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        uint8_t Ms,
        uint8_t Mc,
        std::string& errorMsg);

    static void printStats(const CompressionStats& stats);
};

#endif