#include "lz77.h"
#include "bitstream.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <chrono>

using namespace std;


const uint32_t LZ77_MAGIC = 0x4C5A3737; 
const uint8_t LZ77_VERSION = 1;

uint32_t LZ77::calculateChecksum(const vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 5) - checksum + byte;
    }
    return checksum;
}

pair<uint16_t, uint16_t> LZ77::findLongestMatch(
    const vector<uint8_t>& data,
    size_t currentPos,
    uint32_t windowSize,
    uint32_t lookaheadSize) {

    uint16_t bestOffset = 0;
    uint16_t bestLength = 0;

    size_t searchStart = (currentPos > windowSize) ? currentPos - windowSize : 0;
    size_t maxLength = min(lookaheadSize, static_cast<uint32_t>(data.size() - currentPos));

    for (size_t offset = 1; offset <= currentPos - searchStart && offset <= 65535; offset++) {
        size_t matchPos = currentPos - offset;
        uint16_t length = 0;

        while (length < maxLength &&
            matchPos + length < data.size() &&
            data[matchPos + length] == data[currentPos + length]) {
            length++;
        }

        if (length > bestLength) {
            bestLength = length;
            bestOffset = static_cast<uint16_t>(offset);

            if (bestLength == maxLength) {
                break;
            }
        }
    }

    return { bestOffset, bestLength };
}

vector<LZ77Token> LZ77::compressTokens(const vector<uint8_t>& input,
    uint32_t windowSize,
    uint32_t lookaheadSize) {
    vector<LZ77Token> tokens;
    size_t pos = 0;
    size_t n = input.size();

    while (pos < n) {
        auto [offset, length] = findLongestMatch(input, pos, windowSize, lookaheadSize);

        if (length >= 3) {
            uint8_t nextChar = (pos + length < n) ? input[pos + length] : 0;
            tokens.push_back({ offset, static_cast<uint16_t>(length), nextChar });
            pos += length + 1;
        }
        else {
            tokens.push_back({ 0, 0, input[pos] });
            pos++;
        }
    }

    return tokens;
}

vector<uint8_t> LZ77::decompressTokens(const vector<LZ77Token>& tokens, size_t originalSize) {
    vector<uint8_t> output;
    output.reserve(originalSize);

    for (const auto& token : tokens) {
        if (token.offset == 0 && token.length == 0) {
            output.push_back(token.nextChar);
        }
        else {
            size_t startPos = output.size() - token.offset;
            for (uint16_t i = 0; i < token.length; i++) {
                output.push_back(output[startPos + i]);
            }
            if (token.nextChar != 0 || output.size() < originalSize) {
                output.push_back(token.nextChar);
            }
        }
    }

    if (output.size() > originalSize) {
        output.resize(originalSize);
    }

    return output;
}

vector<uint8_t> LZ77::tokensToBytes(const vector<LZ77Token>& tokens) {
    BitWriter writer;

    for (const auto& token : tokens) {
        if (token.offset == 0 && token.length == 0) {
            writer.writeBit(1);
            writer.writeBits(token.nextChar, 8);
        }
        else {
            writer.writeBit(0);
            writer.writeBits(token.offset, 16);
            writer.writeBits(token.length, 16);
            writer.writeBits(token.nextChar, 8);
        }
    }

    writer.flush();
    return writer.getData();
}

vector<LZ77Token> LZ77::bytesToTokens(const vector<uint8_t>& bytes) {
    BitReader reader(bytes);
    vector<LZ77Token> tokens;

    while (reader.hasMore()) {
        uint8_t flag = reader.readBit();

        if (flag == 1) {
            if (!reader.hasMore()) break;
            uint8_t nextChar = static_cast<uint8_t>(reader.readBits(8));
            tokens.push_back({ 0, 0, nextChar });
        }
        else {
            if (!reader.hasMore()) break;
            uint16_t offset = static_cast<uint16_t>(reader.readBits(16));
            if (!reader.hasMore()) break;
            uint16_t length = static_cast<uint16_t>(reader.readBits(16));
            if (!reader.hasMore()) break;
            uint8_t nextChar = static_cast<uint8_t>(reader.readBits(8));
            tokens.push_back({ offset, length, nextChar });
        }
    }

    return tokens;
}

vector<uint8_t> LZ77::encode(const vector<uint8_t>& input,
    uint32_t windowSize,
    uint32_t lookaheadSize) {
    if (input.empty()) {
        vector<uint8_t> result(sizeof(LZ77Header));
        LZ77Header header;
        header.magicNumber = LZ77_MAGIC;
        header.version = LZ77_VERSION;
        header.originalSize = 0;
        header.compressedSize = 0;
        header.windowSize = windowSize;
        header.lookaheadSize = lookaheadSize;
        header.checksum = 0;
        memcpy(result.data(), &header, sizeof(LZ77Header));
        return result;
    }

    vector<LZ77Token> tokens = compressTokens(input, windowSize, lookaheadSize);
    vector<uint8_t> compressedData = tokensToBytes(tokens);

    LZ77Header header;
    header.magicNumber = LZ77_MAGIC;
    header.version = LZ77_VERSION;
    header.originalSize = input.size();
    header.compressedSize = compressedData.size();
    header.windowSize = windowSize;
    header.lookaheadSize = lookaheadSize;
    header.checksum = calculateChecksum(compressedData);

    vector<uint8_t> result(sizeof(LZ77Header) + compressedData.size());
    memcpy(result.data(), &header, sizeof(LZ77Header));
    memcpy(result.data() + sizeof(LZ77Header), compressedData.data(), compressedData.size());

    return result;
}

vector<uint8_t> LZ77::decode(const vector<uint8_t>& input) {
    if (input.size() < sizeof(LZ77Header)) {
        return vector<uint8_t>();
    }

    LZ77Header header;
    memcpy(&header, input.data(), sizeof(LZ77Header));

    if (header.magicNumber != LZ77_MAGIC) {
        return vector<uint8_t>();
    }

    if (header.originalSize == 0) {
        return vector<uint8_t>();
    }

    vector<uint8_t> compressedData(input.begin() + sizeof(LZ77Header), input.end());

    uint32_t checksum = calculateChecksum(compressedData);
    if (checksum != header.checksum) {
        return vector<uint8_t>();
    }

    vector<LZ77Token> tokens = bytesToTokens(compressedData);

    return decompressTokens(tokens, header.originalSize);
}

bool LZ77::encodeFile(const string& inputPath,
    const string& outputPath,
    uint32_t windowSize,
    uint32_t lookaheadSize,
    string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
    inFile.close();

    vector<uint8_t> encodedData = encode(inputData, windowSize, lookaheadSize);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return true;
}

bool LZ77::decodeFile(const string& inputPath,
    const string& outputPath,
    string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
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

LZ77Stats LZ77::compressFile(const string& inputPath,
    const string& outputPath,
    uint32_t windowSize,
    uint32_t lookaheadSize,
    string& errorMsg) {
    LZ77Stats stats;
    stats.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open file: " + inputPath;
        return stats;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
    inFile.close();

    stats.originalSize = inputData.size();

    vector<uint8_t> encodedData = encode(inputData, windowSize, lookaheadSize);
    stats.compressedSize = encodedData.size();
    stats.compressionRatio = (double)stats.compressedSize / stats.originalSize * 100.0;
    stats.compressionFactor = (double)stats.originalSize / stats.compressedSize;
    stats.savingsPercent = 100.0 - stats.compressionRatio;

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return stats;
    }

    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return stats;
}

bool LZ77::verifyCycle(const string& inputPath,
    const string& encodedPath,
    const string& decodedPath,
    uint32_t windowSize,
    uint32_t lookaheadSize,
    string& errorMsg) {
    if (!encodeFile(inputPath, encodedPath, windowSize, lookaheadSize, errorMsg)) {
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

    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)),
        istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)),
        istreambuf_iterator<char>());

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

void LZ77::printStats(const LZ77Stats& stats) {
    cout << "  Original: " << stats.originalSize << " bytes\n";
    cout << "  Compressed: " << stats.compressedSize << " bytes\n";
    cout << "  Ratio: " << fixed << setprecision(2)
        << stats.compressionRatio << "% of original\n";
    cout << "  Factor: " << fixed << setprecision(2)
        << stats.compressionFactor << ":1\n";
}