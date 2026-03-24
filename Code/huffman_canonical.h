#ifndef HUFFMAN_CANONICAL_H
#define HUFFMAN_CANONICAL_H

#include <vector>
#include <cstdint>
#include <string>
#include <map>

struct CanonicalHuffmanHeader {
    uint32_t magicNumber;
    uint8_t version;
    uint64_t originalSize;
    uint64_t compressedSize;
    uint32_t checksum;
    uint16_t numSymbols;
};

struct CanonicalCode {
    uint32_t code;
    uint8_t length;
};

class HuffmanCanonical {
public:
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);

    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        std::string& errorMsg);

private:
    static std::map<uint8_t, uint64_t> calculateFrequencies(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> getCodeLengths(const std::map<uint8_t, uint64_t>& frequencies);
    static std::map<uint8_t, CanonicalCode> buildCanonicalCodes(const std::vector<uint8_t>& codeLengths);
    static uint32_t calculateChecksum(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> encodeCanonical(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decodeCanonical(const std::vector<uint8_t>& input, size_t originalSize, const std::vector<uint8_t>& codeLengths);
};

#endif