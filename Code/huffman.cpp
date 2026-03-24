#include "huffman.h"
#include "bitstream.h"
#include <vector>
#include <map>
#include <queue>
#include <fstream>
#include <algorithm>
#include <memory>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace std;

HuffmanNode::HuffmanNode(uint8_t sym, uint64_t freq)
    : symbol(sym), frequency(freq), left(nullptr), right(nullptr) {
}

HuffmanNode::HuffmanNode(uint64_t freq, shared_ptr<HuffmanNode> l, shared_ptr<HuffmanNode> r)
    : symbol(0), frequency(freq), left(l), right(r) {
}

bool HuffmanNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}

map<uint8_t, uint64_t> Huffman::calculateFrequencies(const vector<uint8_t>& data) {
    map<uint8_t, uint64_t> frequencies;
    for (uint8_t byte : data) {
        frequencies[byte]++;
    }
    return frequencies;
}

shared_ptr<HuffmanNode> Huffman::buildTree(const map<uint8_t, uint64_t>& frequencies) {
    auto cmp = [](const shared_ptr<HuffmanNode>& a, const shared_ptr<HuffmanNode>& b) {
        return a->frequency > b->frequency;
        };

    priority_queue<shared_ptr<HuffmanNode>, vector<shared_ptr<HuffmanNode>>, decltype(cmp)> pq(cmp);

    for (const auto& pair : frequencies) {
        pq.push(make_shared<HuffmanNode>(pair.first, pair.second));
    }

    if (pq.empty()) {
        return nullptr;
    }

    if (pq.size() == 1) {
        auto node = pq.top();
        pq.pop();
        return make_shared<HuffmanNode>(node->frequency, node, nullptr);
    }

    while (pq.size() > 1) {
        auto left = pq.top();
        pq.pop();
        auto right = pq.top();
        pq.pop();

        auto parent = make_shared<HuffmanNode>(left->frequency + right->frequency, left, right);
        pq.push(parent);
    }

    return pq.top();
}

void Huffman::generateCodes(shared_ptr<HuffmanNode> node, vector<bool>& currentCode, map<uint8_t, vector<bool>>& codes) {
    if (!node) return;

    if (node->isLeaf()) {
        codes[node->symbol] = currentCode;
        return;
    }

    if (node->left) {
        currentCode.push_back(0);
        generateCodes(node->left, currentCode, codes);
        currentCode.pop_back();
    }

    if (node->right) {
        currentCode.push_back(1);
        generateCodes(node->right, currentCode, codes);
        currentCode.pop_back();
    }
}

void Huffman::writeTreeToBitStream(shared_ptr<HuffmanNode> node, BitWriter& writer) {
    if (!node) return;

    if (node->isLeaf()) {
        writer.writeBit(1);
        writer.writeBits(node->symbol, 8);
    }
    else {
        writer.writeBit(0);
        writeTreeToBitStream(node->left, writer);
        writeTreeToBitStream(node->right, writer);
    }
}

shared_ptr<HuffmanNode> Huffman::readTreeFromBitStream(BitReader& reader) {
    if (!reader.hasMore()) return nullptr;

    uint8_t flag = reader.readBit();

    if (flag == 1) {
        uint8_t symbol = static_cast<uint8_t>(reader.readBits(8));
        return make_shared<HuffmanNode>(symbol, 0);
    }
    else {
        auto left = readTreeFromBitStream(reader);
        auto right = readTreeFromBitStream(reader);
        return make_shared<HuffmanNode>(0, left, right);
    }
}

size_t Huffman::calculateTreeSize(shared_ptr<HuffmanNode> node) {
    if (!node) return 0;

    size_t size = 0;
    if (node->isLeaf()) {
        size += 1 + 1;
    }
    else {
        size += 1;
        size += calculateTreeSize(node->left);
        size += calculateTreeSize(node->right);
    }
    return size;
}

uint32_t Huffman::calculateChecksum(const vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 5) - checksum + byte;
    }
    return checksum;
}

vector<uint8_t> Huffman::encode(const vector<uint8_t>& input) {
    BitWriter writer;

    if (input.empty()) {
        return writer.getData();
    }

    map<uint8_t, uint64_t> frequencies = calculateFrequencies(input);

    shared_ptr<HuffmanNode> root = buildTree(frequencies);

    if (!root) {
        return writer.getData();
    }

    writeTreeToBitStream(root, writer);

    uint64_t originalSize = input.size();
    writer.writeBits(originalSize, 64);

    map<uint8_t, vector<bool>> codes;
    vector<bool> currentCode;
    generateCodes(root, currentCode, codes);

    for (uint8_t byte : input) {
        const vector<bool>& code = codes[byte];
        for (bool bit : code) {
            writer.writeBit(bit ? 1 : 0);
        }
    }

    writer.flush();
    return writer.getData();
}

vector<uint8_t> Huffman::decode(const vector<uint8_t>& input) {
    BitReader reader(input);
    vector<uint8_t> output;

    if (input.empty()) {
        return output;
    }

    shared_ptr<HuffmanNode> root = readTreeFromBitStream(reader);

    if (!root) {
        return output;
    }

    if (!reader.hasMore()) {
        return output;
    }

    uint64_t originalSize = reader.readBits(64);

    for (uint64_t i = 0; i < originalSize; i++) {
        shared_ptr<HuffmanNode> current = root;

        while (current && !current->isLeaf()) {
            if (!reader.hasMore()) {
                return output;
            }
            uint8_t bit = reader.readBit();
            if (bit == 0) {
                current = current->left;
            }
            else {
                current = current->right;
            }
        }

        if (current && current->isLeaf()) {
            output.push_back(current->symbol);
        }
        else {
            return output;
        }
    }

    return output;
}

bool Huffman::encodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    vector<uint8_t> encodedData = encode(inputData);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return true;
}

bool Huffman::decodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    vector<uint8_t> decodedData = decode(inputData);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create decoded file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    outFile.close();

    return true;
}



bool Huffman::encodeFileWithStats(const string& inputPath, const string& outputPath,
    CompressionStatsHuffman& stats, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    stats.originalSize = inputData.size();
    stats.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    map<uint8_t, uint64_t> frequencies = calculateFrequencies(inputData);
    shared_ptr<HuffmanNode> root = buildTree(frequencies);

    if (!root) {
        errorMsg = "Failed to build Huffman tree";
        return false;
    }

    stats.treeSize = calculateTreeSize(root);

    BitWriter writer;
    writeTreeToBitStream(root, writer);
    stats.metadataSize = writer.getData().size();

    writer.writeBits(stats.originalSize, 64);

    map<uint8_t, vector<bool>> codes;
    vector<bool> currentCode;
    generateCodes(root, currentCode, codes);

    for (uint8_t byte : inputData) {
        const vector<bool>& code = codes[byte];
        for (bool bit : code) {
            writer.writeBit(bit ? 1 : 0);
        }
    }

    writer.flush();
    vector<uint8_t> encodedData = writer.getData();

    stats.encodedSize = encodedData.size();
    stats.encodedDataSize = stats.encodedSize - stats.metadataSize - 8;
    stats.compressionRatio = (double)stats.encodedSize / stats.originalSize * 100.0;
    stats.compressionFactor = (double)stats.originalSize / stats.encodedSize;
    stats.metadataOverhead = (double)stats.metadataSize / stats.encodedSize * 100.0;

    double entropy = 0.0;
    for (const auto& pair : frequencies) {
        double prob = (double)pair.second / stats.originalSize;
        entropy -= prob * log2(prob);
    }

    stats.efficiency = (entropy * stats.originalSize / 8.0) / stats.encodedSize * 100.0;

    uint32_t checksum = calculateChecksum(encodedData);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    HuffmanFileHeader header;
    header.magicNumber = 0x4846464D;
    header.version = 1;
    header.originalSize = stats.originalSize;
    header.encodedDataSize = encodedData.size();
    header.checksum = checksum;

    outFile.write(reinterpret_cast<const char*>(&header.magicNumber), sizeof(header.magicNumber));
    outFile.write(reinterpret_cast<const char*>(&header.version), sizeof(header.version));
    outFile.write(reinterpret_cast<const char*>(&header.originalSize), sizeof(header.originalSize));
    outFile.write(reinterpret_cast<const char*>(&header.encodedDataSize), sizeof(header.encodedDataSize));
    outFile.write(reinterpret_cast<const char*>(&header.checksum), sizeof(header.checksum));
    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return true;
}

bool Huffman::decodeFileWithVerify(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    HuffmanFileHeader header;
    inFile.read(reinterpret_cast<char*>(&header.magicNumber), sizeof(header.magicNumber));
    if (header.magicNumber != 0x4846464D) {
        errorMsg = "Invalid file format: wrong magic number";
        return false;
    }

    inFile.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
    if (header.version != 1) {
        errorMsg = "Unsupported file version";
        return false;
    }

    inFile.read(reinterpret_cast<char*>(&header.originalSize), sizeof(header.originalSize));
    inFile.read(reinterpret_cast<char*>(&header.encodedDataSize), sizeof(header.encodedDataSize));
    inFile.read(reinterpret_cast<char*>(&header.checksum), sizeof(header.checksum));

    vector<uint8_t> encodedData(header.encodedDataSize);
    inFile.read(reinterpret_cast<char*>(encodedData.data()), header.encodedDataSize);
    inFile.close();

    uint32_t checksum = calculateChecksum(encodedData);
    if (checksum != header.checksum) {
        errorMsg = "Checksum verification failed: file may be corrupted";
        return false;
    }

    vector<uint8_t> decodedData = decode(encodedData);

    if (decodedData.size() != header.originalSize) {
        errorMsg = "Decoded size mismatch: expected " + to_string(header.originalSize) +
            ", got " + to_string(decodedData.size());
        return false;
    }

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create decoded file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    outFile.close();

    return true;
}



bool Huffman::verifyCycle(const string& inputPath, const string& encodedPath,
    const string& decodedPath, string& errorMsg) {
    if (!encodeFile(inputPath, encodedPath, errorMsg)) {
        return false;
    }

    if (!decodeFile(encodedPath, decodedPath, errorMsg)) {
        return false;
    }

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);

    if (!origFile.is_open() || !decFile.is_open()) {
        errorMsg = "Cannot open files for comparison";
        return false;
    }

    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    if (origData.size() != decData.size()) {
        return false;
    }

    for (size_t i = 0; i < origData.size(); i++) {
        if (origData[i] != decData[i]) {
            return false;
        }
    }

    return true;
}

double Huffman::getCompressionRatio(size_t originalSize, size_t encodedSize) {
    if (originalSize == 0) return 0.0;
    return (double)encodedSize / originalSize * 100.0;
}

CompressionStatsHuffman Huffman::analyzeFile(const string& inputPath, string& errorMsg) {
    CompressionStatsHuffman stats;
    stats.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);
    stats.originalSize = 0;
    stats.encodedSize = 0;
    stats.metadataSize = 0;
    stats.treeSize = 0;
    stats.encodedDataSize = 0;
    stats.compressionRatio = 0.0;
    stats.compressionFactor = 0.0;
    stats.metadataOverhead = 0.0;
    stats.efficiency = 0.0;

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open file: " + inputPath;
        return stats;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    stats.originalSize = inputData.size();

    map<uint8_t, uint64_t> frequencies = calculateFrequencies(inputData);
    shared_ptr<HuffmanNode> root = buildTree(frequencies);

    if (!root) {
        errorMsg = "Failed to build Huffman tree";
        return stats;
    }

    stats.treeSize = calculateTreeSize(root);

    BitWriter writer;
    writeTreeToBitStream(root, writer);
    stats.metadataSize = writer.getData().size();

    writer.writeBits(stats.originalSize, 64);

    map<uint8_t, vector<bool>> codes;
    vector<bool> currentCode;
    generateCodes(root, currentCode, codes);

    for (uint8_t byte : inputData) {
        const vector<bool>& code = codes[byte];
        for (bool bit : code) {
            writer.writeBit(bit ? 1 : 0);
        }
    }

    writer.flush();
    vector<uint8_t> encodedData = writer.getData();

    stats.encodedSize = encodedData.size();
    stats.encodedDataSize = stats.encodedSize - stats.metadataSize - 8;
    stats.compressionRatio = (double)stats.encodedSize / stats.originalSize * 100.0;
    stats.compressionFactor = (double)stats.originalSize / stats.encodedSize;
    stats.metadataOverhead = (double)stats.metadataSize / stats.encodedSize * 100.0;

    double entropy = 0.0;
    for (const auto& pair : frequencies) {
        double prob = (double)pair.second / stats.originalSize;
        entropy -= prob * log2(prob);
    }

    stats.efficiency = (entropy * stats.originalSize / 8.0) / stats.encodedSize * 100.0;

    return stats;
}

void Huffman::printStats(const CompressionStatsHuffman& stats) {
    cout << "File: " << stats.filename << "\n";
    cout << "----------------------------------------\n";
    cout << "Original size: " << stats.originalSize << " bytes\n";
    cout << "Encoded size:  " << stats.encodedSize << " bytes\n";
    cout << "Metadata size: " << stats.metadataSize << " bytes\n";
    cout << "  - Tree structure: " << stats.treeSize << " bytes\n";
    cout << "  - Original size: 8 bytes\n";
    cout << "  - Header: " << (stats.metadataSize - stats.treeSize - 8) << " bytes\n";
    cout << "Compression ratio: " << fixed << setprecision(2) << stats.compressionRatio << "% of original\n";
    cout << "Compression factor: " << fixed << setprecision(2) << stats.compressionFactor << ":1\n";
    cout << "Metadata overhead: " << fixed << setprecision(2) << stats.metadataOverhead << "% of encoded file\n";
    cout << "Huffman efficiency: " << fixed << setprecision(2) << stats.efficiency << "% of entropy bound\n";
}