#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include "bitstream.h"

struct HuffmanNode {
    uint8_t symbol;
    uint64_t frequency;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(uint8_t sym, uint64_t freq);
    HuffmanNode(uint64_t freq, std::shared_ptr<HuffmanNode> l, std::shared_ptr<HuffmanNode> r);
    bool isLeaf() const;
};

struct HuffmanFileHeader {
    uint32_t magicNumber;
    uint8_t version;
    uint64_t originalSize;
    uint64_t encodedDataSize;
    uint32_t checksum;
};

struct CompressionStatsHuffman {
    std::string filename;
    size_t originalSize;
    size_t encodedSize;
    size_t metadataSize;
    size_t treeSize;
    size_t encodedDataSize;
    double compressionRatio;
    double compressionFactor;
    double metadataOverhead;
    double efficiency;
};

class Huffman {
public:
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);

    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool encodeFileWithStats(const std::string& inputPath,
        const std::string& outputPath,
        CompressionStatsHuffman& stats,
        std::string& errorMsg);

    static bool decodeFileWithVerify(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        std::string& errorMsg);

    static double getCompressionRatio(size_t originalSize, size_t encodedSize);

    static CompressionStatsHuffman analyzeFile(const std::string& inputPath, std::string& errorMsg);

    static void printStats(const CompressionStatsHuffman& stats);

private:
    static std::map<uint8_t, uint64_t> calculateFrequencies(const std::vector<uint8_t>& data);

    static std::shared_ptr<HuffmanNode> buildTree(const std::map<uint8_t, uint64_t>& frequencies);

    static void generateCodes(std::shared_ptr<HuffmanNode> node,
        std::vector<bool>& currentCode,
        std::map<uint8_t, std::vector<bool>>& codes);

    static void writeTreeToBitStream(std::shared_ptr<HuffmanNode> node, BitWriter& writer);

    static std::shared_ptr<HuffmanNode> readTreeFromBitStream(BitReader& reader);

    static size_t calculateTreeSize(std::shared_ptr<HuffmanNode> node);

    static uint32_t calculateChecksum(const std::vector<uint8_t>& data);
};

#endif