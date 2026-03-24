#ifndef COMPRESSORS_H
#define COMPRESSORS_H

#include <vector>
#include <cstdint>
#include <string>

struct CompressionResult {
    std::string filename;
    size_t originalSize;
    size_t compressedSize;
    double compressionRatio;
    double compressionFactor;
    long long encodeTimeMs;
    long long decodeTimeMs;
    bool verified;
    std::string errorMsg;
};

class Compressors {
public:
    static CompressionResult HA(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult RLE(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult BWTRLE(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult BWTMTFHA(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult BWTMTFRLEHA(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult LZSS(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult LZSSHA(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult LZW(const std::string& inputPath, const std::string& outputPath);
    static CompressionResult LZWHA(const std::string& inputPath, const std::string& outputPath);

    static void printResult(const CompressionResult& result);
    static void printSummary(const std::vector<CompressionResult>& results);
};

#endif