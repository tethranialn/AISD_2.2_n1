#include "lz78.h"
#include "bitstream.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>
#include <iomanip>
#include <cmath>

using namespace std;


const uint32_t LZ78_MAGIC = 0x4C5A3738; 
const uint8_t LZ78_VERSION = 1;

uint32_t LZ78::calculateChecksum(const vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 5) - checksum + byte;
    }
    return checksum;
}

uint8_t LZ78::getIndexBitSize(uint32_t maxDictSize) {
    if (maxDictSize == 0) return 1;
    return static_cast<uint8_t>(ceil(log2(maxDictSize + 1)));
}

vector<uint8_t> LZ78::encodeTokens(const vector<uint8_t>& input, uint32_t maxDictSize) {
    BitWriter writer;

    
    map<vector<uint8_t>, uint32_t> dictionary;

 
    vector<uint8_t> empty;
    dictionary[empty] = 0;

    uint32_t nextIndex = 1;
    uint8_t indexBitSize = getIndexBitSize(maxDictSize);

    vector<uint8_t> current;

    size_t pos = 0;
    size_t n = input.size();

    while (pos < n) {
        current.push_back(input[pos]);

        
        if (dictionary.find(current) != dictionary.end() && pos + 1 < n) {
            pos++;
            continue;
        }

        
        vector<uint8_t> prefix(current.begin(), current.end() - 1);
        uint8_t lastChar = current.back();

        uint32_t prefixIndex = dictionary[prefix];

        
        writer.writeBits(prefixIndex, indexBitSize);
        
        writer.writeBits(lastChar, 8);

        
        if (nextIndex < maxDictSize) {
            dictionary[current] = nextIndex++;
        }

        
        current.clear();
        pos++;
    }

    writer.flush();
    return writer.getData();
}

vector<uint8_t> LZ78::decodeTokens(const vector<uint8_t>& compressed, size_t originalSize, uint32_t maxDictSize) {
    BitReader reader(compressed);
    vector<uint8_t> output;
    output.reserve(originalSize);

    
    map<uint32_t, vector<uint8_t>> dictionary;
    dictionary[0] = vector<uint8_t>();

    uint32_t nextIndex = 1;
    uint8_t indexBitSize = getIndexBitSize(maxDictSize);

    while (output.size() < originalSize && reader.hasMore()) {
   
        if (!reader.hasMore()) break;
        uint32_t prefixIndex = static_cast<uint32_t>(reader.readBits(indexBitSize));

        
        if (!reader.hasMore()) break;
        uint8_t nextChar = static_cast<uint8_t>(reader.readBits(8));

        
        vector<uint8_t> phrase;

        auto it = dictionary.find(prefixIndex);
        if (it != dictionary.end()) {
            phrase = it->second;
        }

        phrase.push_back(nextChar);

        
        for (uint8_t c : phrase) {
            if (output.size() < originalSize) {
                output.push_back(c);
            }
        }

       
        if (nextIndex < maxDictSize) {
            dictionary[nextIndex++] = phrase;
        }
    }

    return output;
}

vector<uint8_t> LZ78::encode(const vector<uint8_t>& input, uint32_t maxDictSize) {
    if (input.empty()) {
        vector<uint8_t> result(sizeof(LZ78Header));
        LZ78Header header;
        header.magicNumber = LZ78_MAGIC;
        header.version = LZ78_VERSION;
        header.originalSize = 0;
        header.compressedSize = 0;
        header.maxDictSize = maxDictSize;
        header.checksum = 0;
        memcpy(result.data(), &header, sizeof(LZ78Header));
        return result;
    }

    vector<uint8_t> compressedData = encodeTokens(input, maxDictSize);

    LZ78Header header;
    header.magicNumber = LZ78_MAGIC;
    header.version = LZ78_VERSION;
    header.originalSize = input.size();
    header.compressedSize = compressedData.size();
    header.maxDictSize = maxDictSize;
    header.checksum = calculateChecksum(compressedData);

    vector<uint8_t> result(sizeof(LZ78Header) + compressedData.size());
    memcpy(result.data(), &header, sizeof(LZ78Header));
    memcpy(result.data() + sizeof(LZ78Header), compressedData.data(), compressedData.size());

    return result;
}

vector<uint8_t> LZ78::decode(const vector<uint8_t>& input) {
    if (input.size() < sizeof(LZ78Header)) {
        return vector<uint8_t>();
    }

    LZ78Header header;
    memcpy(&header, input.data(), sizeof(LZ78Header));

    if (header.magicNumber != LZ78_MAGIC) {
        return vector<uint8_t>();
    }

    if (header.version != LZ78_VERSION) {
        return vector<uint8_t>();
    }

    if (header.originalSize == 0) {
        return vector<uint8_t>();
    }

    if (input.size() < sizeof(LZ78Header) + header.compressedSize) {
        return vector<uint8_t>();
    }

    vector<uint8_t> compressedData(input.begin() + sizeof(LZ78Header),
        input.begin() + sizeof(LZ78Header) + header.compressedSize);

    uint32_t checksum = calculateChecksum(compressedData);
    if (checksum != header.checksum) {
        return vector<uint8_t>();
    }

    return decodeTokens(compressedData, header.originalSize, header.maxDictSize);
}

bool LZ78::encodeFile(const string& inputPath,
    const string& outputPath,
    uint32_t maxDictSize,
    string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
    inFile.close();

    vector<uint8_t> encodedData = encode(inputData, maxDictSize);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return true;
}

bool LZ78::decodeFile(const string& inputPath,
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

LZ78Stats LZ78::compressFile(const string& inputPath,
    const string& outputPath,
    uint32_t maxDictSize,
    string& errorMsg) {
    LZ78Stats stats;
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

    map<vector<uint8_t>, uint32_t> dictionary;
    vector<uint8_t> empty;
    dictionary[empty] = 0;

    uint32_t nextIndex = 1;
    vector<uint8_t> current;

    size_t pos = 0;
    size_t n = inputData.size();

    stats.numLiterals = 0;
    stats.numPhrases = 0;
    double totalPhraseLength = 0;

    while (pos < n) {
        current.push_back(inputData[pos]);

        if (dictionary.find(current) != dictionary.end() && pos + 1 < n) {
            pos++;
            continue;
        }

        stats.numPhrases++;
        if (current.size() == 1) {
            stats.numLiterals++;
        }
        totalPhraseLength += current.size();

        if (nextIndex < maxDictSize) {
            dictionary[current] = nextIndex++;
        }

        current.clear();
        pos++;
    }

    if (stats.numPhrases > 0) {
        stats.averagePhraseLength = totalPhraseLength / stats.numPhrases;
    }
    stats.dictSize = min(nextIndex, maxDictSize);


    vector<uint8_t> encodedData = encode(inputData, maxDictSize);
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

bool LZ78::verifyCycle(const string& inputPath,
    const string& encodedPath,
    const string& decodedPath,
    uint32_t maxDictSize,
    string& errorMsg) {
    if (!encodeFile(inputPath, encodedPath, maxDictSize, errorMsg)) {
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
        errorMsg = "Size mismatch";
        return false;
    }

    for (size_t i = 0; i < origData.size(); i++) {
        if (origData[i] != decData[i]) {
            errorMsg = "Data mismatch at position " + to_string(i);
            return false;
        }
    }

    return true;
}

void LZ78::printStats(const LZ78Stats& stats) {
    cout << "  Original: " << stats.originalSize << " bytes\n";
    cout << "  Compressed: " << stats.compressedSize << " bytes\n";
    cout << "  Ratio: " << fixed << setprecision(2)
        << stats.compressionRatio << "% of original\n";
    cout << "  Factor: " << fixed << setprecision(2)
        << stats.compressionFactor << ":1\n";
}