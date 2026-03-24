#include "huffman_canonical.h"
#include "bitstream.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <queue>
#include <cmath>
#include <functional>

using namespace std;

const uint32_t HUFFMAN_CANONICAL_MAGIC = 0x4843464D;
const uint8_t HUFFMAN_CANONICAL_VERSION = 1;

struct HuffmanNodeCanonical {
    uint8_t symbol;
    uint64_t frequency;
    shared_ptr<HuffmanNodeCanonical> left;
    shared_ptr<HuffmanNodeCanonical> right;

    HuffmanNodeCanonical(uint8_t sym, uint64_t freq) : symbol(sym), frequency(freq), left(nullptr), right(nullptr) {}
    HuffmanNodeCanonical(uint64_t freq, shared_ptr<HuffmanNodeCanonical> l, shared_ptr<HuffmanNodeCanonical> r)
        : symbol(0), frequency(freq), left(l), right(r) {
    }
    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

uint32_t HuffmanCanonical::calculateChecksum(const vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 5) - checksum + byte;
    }
    return checksum;
}

map<uint8_t, uint64_t> HuffmanCanonical::calculateFrequencies(const vector<uint8_t>& data) {
    map<uint8_t, uint64_t> frequencies;
    for (uint8_t byte : data) {
        frequencies[byte]++;
    }
    return frequencies;
}

vector<uint8_t> HuffmanCanonical::getCodeLengths(const map<uint8_t, uint64_t>& frequencies) {
    if (frequencies.empty()) return vector<uint8_t>(256, 0);

    auto cmp = [](const shared_ptr<HuffmanNodeCanonical>& a, const shared_ptr<HuffmanNodeCanonical>& b) {
        return a->frequency > b->frequency;
        };

    priority_queue<shared_ptr<HuffmanNodeCanonical>, vector<shared_ptr<HuffmanNodeCanonical>>, decltype(cmp)> pq(cmp);

    for (const auto& pair : frequencies) {
        pq.push(make_shared<HuffmanNodeCanonical>(pair.first, pair.second));
    }

    if (pq.size() == 1) {
        auto node = pq.top();
        vector<uint8_t> codeLengths(256, 0);
        codeLengths[node->symbol] = 1;
        return codeLengths;
    }

    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        auto parent = make_shared<HuffmanNodeCanonical>(left->frequency + right->frequency, left, right);
        pq.push(parent);
    }

    auto root = pq.top();
    vector<uint8_t> codeLengths(256, 0);

    function<void(shared_ptr<HuffmanNodeCanonical>, int)> traverse;
    traverse = [&](shared_ptr<HuffmanNodeCanonical> node, int depth) {
        if (!node) return;
        if (node->isLeaf()) {
            codeLengths[node->symbol] = depth;
        }
        else {
            traverse(node->left, depth + 1);
            traverse(node->right, depth + 1);
        }
        };

    traverse(root, 0);

    return codeLengths;
}

map<uint8_t, CanonicalCode> HuffmanCanonical::buildCanonicalCodes(const vector<uint8_t>& codeLengths) {
    map<uint8_t, CanonicalCode> codes;

    vector<pair<uint8_t, uint8_t>> symbolsWithLength;
    for (int i = 0; i < 256; i++) {
        if (codeLengths[i] > 0) {
            symbolsWithLength.push_back({ static_cast<uint8_t>(i), codeLengths[i] });
        }
    }

    sort(symbolsWithLength.begin(), symbolsWithLength.end(),
        [](const pair<uint8_t, uint8_t>& a, const pair<uint8_t, uint8_t>& b) {
            if (a.second != b.second) return a.second < b.second;
            return a.first < b.first;
        });

    uint32_t currentCode = 0;
    uint8_t prevLength = 0;

    for (const auto& symLen : symbolsWithLength) {
        uint8_t symbol = symLen.first;
        uint8_t length = symLen.second;

        if (length > prevLength) {
            currentCode <<= (length - prevLength);
        }

        CanonicalCode code;
        code.code = currentCode;
        code.length = length;
        codes[symbol] = code;

        currentCode++;
        prevLength = length;
    }

    return codes;
}

vector<uint8_t> HuffmanCanonical::encodeCanonical(const vector<uint8_t>& input) {
    BitWriter writer;

    if (input.empty()) return writer.getData();

    map<uint8_t, uint64_t> frequencies = calculateFrequencies(input);
    vector<uint8_t> codeLengths = getCodeLengths(frequencies);
    map<uint8_t, CanonicalCode> codes = buildCanonicalCodes(codeLengths);

    writer.writeBits(HUFFMAN_CANONICAL_MAGIC, 32);
    writer.writeBits(HUFFMAN_CANONICAL_VERSION, 8);
    writer.writeBits(input.size(), 64);

    uint16_t numSymbols = 0;
    for (int i = 0; i < 256; i++) {
        if (codeLengths[i] > 0) numSymbols++;
    }
    writer.writeBits(numSymbols, 16);

    for (int i = 0; i < 256; i++) {
        if (codeLengths[i] > 0) {
            writer.writeBits(i, 8);
            writer.writeBits(codeLengths[i], 8);
        }
    }

    for (uint8_t byte : input) {
        const CanonicalCode& code = codes[byte];
        writer.writeBits(code.code, code.length);
    }

    writer.flush();
    return writer.getData();
}

vector<uint8_t> HuffmanCanonical::decodeCanonical(const vector<uint8_t>& input, size_t originalSize, const vector<uint8_t>& codeLengths) {
    BitReader reader(input);
    vector<uint8_t> output;
    output.reserve(originalSize);

    vector<pair<uint8_t, uint8_t>> symbolsWithLength;
    for (int i = 0; i < 256; i++) {
        if (codeLengths[i] > 0) {
            symbolsWithLength.push_back({ static_cast<uint8_t>(i), codeLengths[i] });
        }
    }

    sort(symbolsWithLength.begin(), symbolsWithLength.end(),
        [](const pair<uint8_t, uint8_t>& a, const pair<uint8_t, uint8_t>& b) {
            if (a.second != b.second) return a.second < b.second;
            return a.first < b.first;
        });

    vector<pair<uint32_t, uint8_t>> codeToSymbol;
    uint32_t currentCode = 0;
    uint8_t prevLength = 0;

    for (const auto& symLen : symbolsWithLength) {
        uint8_t symbol = symLen.first;
        uint8_t length = symLen.second;

        if (length > prevLength) {
            currentCode <<= (length - prevLength);
        }

        codeToSymbol.push_back({ currentCode, symbol });

        currentCode++;
        prevLength = length;
    }

    while (output.size() < originalSize && reader.hasMore()) {
        uint32_t currentValue = 0;
        int bitsRead = 0;
        bool found = false;

        while (!found && reader.hasMore()) {
            currentValue = (currentValue << 1) | reader.readBit();
            bitsRead++;

            for (const auto& pair : codeToSymbol) {
                if (pair.first == currentValue) {
                    uint8_t length = 0;
                    for (const auto& sl : symbolsWithLength) {
                        if (sl.first == pair.second) {
                            length = sl.second;
                            break;
                        }
                    }
                    if (bitsRead == length) {
                        output.push_back(pair.second);
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!found) break;
    }

    return output;
}

vector<uint8_t> HuffmanCanonical::encode(const vector<uint8_t>& input) {
    if (input.empty()) {
        vector<uint8_t> result(sizeof(CanonicalHuffmanHeader));
        CanonicalHuffmanHeader header;
        header.magicNumber = HUFFMAN_CANONICAL_MAGIC;
        header.version = HUFFMAN_CANONICAL_VERSION;
        header.originalSize = 0;
        header.compressedSize = 0;
        header.checksum = 0;
        header.numSymbols = 0;
        memcpy(result.data(), &header, sizeof(CanonicalHuffmanHeader));
        return result;
    }

    vector<uint8_t> compressedData = encodeCanonical(input);

    CanonicalHuffmanHeader header;
    header.magicNumber = HUFFMAN_CANONICAL_MAGIC;
    header.version = HUFFMAN_CANONICAL_VERSION;
    header.originalSize = input.size();
    header.compressedSize = compressedData.size();
    header.checksum = calculateChecksum(compressedData);

    uint16_t numSymbols = 0;
    map<uint8_t, uint64_t> freqs = calculateFrequencies(input);
    for (const auto& p : freqs) {
        if (p.second > 0) numSymbols++;
    }
    header.numSymbols = numSymbols;

    vector<uint8_t> result(sizeof(CanonicalHuffmanHeader) + compressedData.size());
    memcpy(result.data(), &header, sizeof(CanonicalHuffmanHeader));
    memcpy(result.data() + sizeof(CanonicalHuffmanHeader), compressedData.data(), compressedData.size());

    return result;
}

vector<uint8_t> HuffmanCanonical::decode(const vector<uint8_t>& input) {
    if (input.size() < sizeof(CanonicalHuffmanHeader)) {
        return vector<uint8_t>();
    }

    CanonicalHuffmanHeader header;
    memcpy(&header, input.data(), sizeof(CanonicalHuffmanHeader));

    if (header.magicNumber != HUFFMAN_CANONICAL_MAGIC) {
        return vector<uint8_t>();
    }

    if (header.version != HUFFMAN_CANONICAL_VERSION) {
        return vector<uint8_t>();
    }

    if (header.originalSize == 0) {
        return vector<uint8_t>();
    }

    if (input.size() < sizeof(CanonicalHuffmanHeader) + header.compressedSize) {
        return vector<uint8_t>();
    }

    vector<uint8_t> compressedData(input.begin() + sizeof(CanonicalHuffmanHeader),
        input.begin() + sizeof(CanonicalHuffmanHeader) + header.compressedSize);

    uint32_t checksum = calculateChecksum(compressedData);
    if (checksum != header.checksum) {
        return vector<uint8_t>();
    }

    BitReader reader(compressedData);

    uint32_t magic = static_cast<uint32_t>(reader.readBits(32));
    uint8_t version = static_cast<uint8_t>(reader.readBits(8));
    uint64_t originalSize = reader.readBits(64);
    uint16_t numSymbols = static_cast<uint16_t>(reader.readBits(16));

    vector<uint8_t> codeLengths(256, 0);
    for (int i = 0; i < numSymbols; i++) {
        uint8_t symbol = static_cast<uint8_t>(reader.readBits(8));
        uint8_t length = static_cast<uint8_t>(reader.readBits(8));
        codeLengths[symbol] = length;
    }

    vector<uint8_t> remainingData;
    while (reader.hasMore()) {
        remainingData.push_back(static_cast<uint8_t>(reader.readBits(8)));
    }

    return decodeCanonical(remainingData, originalSize, codeLengths);
}

bool HuffmanCanonical::encodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
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

bool HuffmanCanonical::decodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    vector<uint8_t> decodedData = decode(inputData);

    if (decodedData.empty()) {
        errorMsg = "Failed to decode data";
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

bool HuffmanCanonical::verifyCycle(const string& inputPath, const string& encodedPath, const string& decodedPath, string& errorMsg) {
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