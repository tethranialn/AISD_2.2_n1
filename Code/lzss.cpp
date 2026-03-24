#include "lzss.h"
#include "bitstream.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <iomanip>

using namespace std;

// Константы
const uint32_t LZSS_MAGIC = 0x4C5A5353; // "LZSS"
const uint8_t LZSS_VERSION = 1;

uint32_t LZSS::calculateChecksum(const vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 5) - checksum + byte;
    }
    return checksum;
}

pair<uint16_t, uint16_t> LZSS::findLongestMatch(
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

vector<uint8_t> LZSS::encodeTokens(const vector<uint8_t>& input,
    uint32_t windowSize,
    uint32_t lookaheadSize) {
    BitWriter writer;

    size_t pos = 0;
    size_t n = input.size();

    while (pos < n) {
        auto [offset, length] = findLongestMatch(input, pos, windowSize, lookaheadSize);

        // В LZSS используем совпадение только если длина >= 3
        if (length >= 3) {
            // Флаг 0 означает ссылку
            writer.writeBit(0);
            writer.writeBits(offset, 16);
            writer.writeBits(length, 16);
            pos += length;
        }
        else {
            // Флаг 1 означает литерал
            writer.writeBit(1);
            writer.writeBits(input[pos], 8);
            pos++;
        }
    }

    writer.flush();
    return writer.getData();
}

vector<uint8_t> LZSS::decodeTokens(const vector<uint8_t>& compressed, size_t originalSize) {
    BitReader reader(compressed);
    vector<uint8_t> output;
    output.reserve(originalSize);

    while (output.size() < originalSize && reader.hasMore()) {
        uint8_t flag = reader.readBit();

        if (flag == 1) {
            // Литерал
            if (!reader.hasMore()) break;
            uint8_t literal = static_cast<uint8_t>(reader.readBits(8));
            output.push_back(literal);
        }
        else {
            // Ссылка (offset, length)
            if (!reader.hasMore()) break;
            uint16_t offset = static_cast<uint16_t>(reader.readBits(16));
            if (!reader.hasMore()) break;
            uint16_t length = static_cast<uint16_t>(reader.readBits(16));

            size_t startPos = output.size() - offset;
            for (uint16_t i = 0; i < length && output.size() < originalSize; i++) {
                output.push_back(output[startPos + i]);
            }
        }
    }

    return output;
}

vector<uint8_t> LZSS::encode(const vector<uint8_t>& input,
    uint32_t windowSize,
    uint32_t lookaheadSize) {
    if (input.empty()) {
        vector<uint8_t> result(sizeof(LZSSHeader));
        LZSSHeader header;
        header.magicNumber = LZSS_MAGIC;
        header.version = LZSS_VERSION;
        header.originalSize = 0;
        header.compressedSize = 0;
        header.windowSize = windowSize;
        header.lookaheadSize = lookaheadSize;
        header.checksum = 0;
        memcpy(result.data(), &header, sizeof(LZSSHeader));
        return result;
    }

    vector<uint8_t> compressedData = encodeTokens(input, windowSize, lookaheadSize);

    LZSSHeader header;
    header.magicNumber = LZSS_MAGIC;
    header.version = LZSS_VERSION;
    header.originalSize = input.size();
    header.compressedSize = compressedData.size();
    header.windowSize = windowSize;
    header.lookaheadSize = lookaheadSize;
    header.checksum = calculateChecksum(compressedData);

    vector<uint8_t> result(sizeof(LZSSHeader) + compressedData.size());
    memcpy(result.data(), &header, sizeof(LZSSHeader));
    memcpy(result.data() + sizeof(LZSSHeader), compressedData.data(), compressedData.size());

    return result;
}

vector<uint8_t> LZSS::decode(const vector<uint8_t>& input) {
    if (input.size() < sizeof(LZSSHeader)) {
        return vector<uint8_t>();
    }

    LZSSHeader header;
    memcpy(&header, input.data(), sizeof(LZSSHeader));

    if (header.magicNumber != LZSS_MAGIC) {
        return vector<uint8_t>();
    }

    if (header.originalSize == 0) {
        return vector<uint8_t>();
    }

    vector<uint8_t> compressedData(input.begin() + sizeof(LZSSHeader), input.end());

    uint32_t checksum = calculateChecksum(compressedData);
    if (checksum != header.checksum) {
        return vector<uint8_t>();
    }

    return decodeTokens(compressedData, header.originalSize);
}

bool LZSS::encodeFile(const string& inputPath,
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

bool LZSS::decodeFile(const string& inputPath,
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

LZSSStats LZSS::compressFile(const string& inputPath,
    const string& outputPath,
    uint32_t windowSize,
    uint32_t lookaheadSize,
    string& errorMsg) {
    LZSSStats stats;
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

    // Кодируем данные
    vector<uint8_t> encodedData = encode(inputData, windowSize, lookaheadSize);
    stats.compressedSize = encodedData.size();
    stats.compressionRatio = (double)stats.compressedSize / stats.originalSize * 100.0;
    stats.compressionFactor = (double)stats.originalSize / stats.compressedSize;
    stats.savingsPercent = 100.0 - stats.compressionRatio;

    // Подсчитываем статистику по токенам
    stats.numLiterals = 0;
    stats.numMatches = 0;

    size_t pos = 0;
    size_t n = inputData.size();
    double totalMatchLength = 0;
    double totalOffset = 0;

    while (pos < n) {
        auto [offset, length] = findLongestMatch(inputData, pos, windowSize, lookaheadSize);

        if (length >= 3) {
            stats.numMatches++;
            totalMatchLength += length;
            totalOffset += offset;
            pos += length;
        }
        else {
            stats.numLiterals++;
            pos++;
        }
    }

    if (stats.numMatches > 0) {
        stats.averageMatchLength = totalMatchLength / stats.numMatches;
        stats.averageOffset = totalOffset / stats.numMatches;
    }

    // Сохраняем сжатые данные
    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return stats;
    }

    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return stats;
}

bool LZSS::verifyCycle(const string& inputPath,
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

void LZSS::printStats(const LZSSStats& stats) {
    cout << "  Original: " << stats.originalSize << " bytes\n";
    cout << "  Compressed: " << stats.compressedSize << " bytes\n";
    cout << "  Ratio: " << fixed << setprecision(2)
        << stats.compressionRatio << "% of original\n";
    cout << "  Factor: " << fixed << setprecision(2)
        << stats.compressionFactor << ":1\n";
}