#include "rle.h"
#include "bitstream.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iomanip>

using namespace std;

vector<uint8_t> RLE::encode(const vector<uint8_t>& input, uint8_t Ms, uint8_t Mc) {
    BitWriter writer;

    if (input.empty() || Ms == 0 || Mc == 0) {
        return writer.getData();
    }

    size_t i = 0;
    size_t dataSize = input.size();

    while (i < dataSize) {
        size_t remaining = dataSize - i;

        if (remaining >= Ms) {
            size_t runLen = 1;
            uint64_t maxRun = (1ULL << (Mc * 8)) - 1;

            while (i + (runLen + 1) * Ms <= dataSize && runLen < maxRun) {
                bool match = true;
                for (uint8_t k = 0; k < Ms && match; k++) {
                    if (input[i + k] != input[i + runLen * Ms + k]) {
                        match = false;
                    }
                }
                if (!match) break;
                runLen++;
            }

            if (runLen >= 2) {
                writer.writeBit(0);
                writer.writeBits(runLen, Mc * 8);
                for (uint8_t k = 0; k < Ms; k++) {
                    writer.writeBits(input[i + k], 8);
                }
                i += runLen * Ms;
                continue;
            }
        }

        uint64_t maxLiteral = (1ULL << (Mc * 8)) - 1;
        uint64_t literalLen = 1;

        while (i + (literalLen + 1) * Ms <= dataSize && literalLen < maxLiteral) {
            if (remaining >= (literalLen + 2) * Ms) {
                bool nextIsRun = true;
                for (uint8_t k = 0; k < Ms; k++) {
                    if (input[i + literalLen * Ms + k] !=
                        input[i + (literalLen + 1) * Ms + k]) {
                        nextIsRun = false;
                        break;
                    }
                }
                if (nextIsRun) break;
            }
            literalLen++;
        }

        writer.writeBit(1);
        writer.writeBits(literalLen, Mc * 8);
        for (uint64_t j = 0; j < literalLen; j++) {
            for (uint8_t k = 0; k < Ms; k++) {
                writer.writeBits(input[i + j * Ms + k], 8);
            }
        }
        i += literalLen * Ms;
    }

    writer.flush();
    return writer.getData();
}

vector<uint8_t> RLE::decode(const vector<uint8_t>& input, uint8_t Ms, uint8_t Mc) {
    BitReader reader(input);
    vector<uint8_t> output;

    if (input.empty() || Ms == 0 || Mc == 0) {
        return output;
    }

    while (reader.hasMore()) {
        uint8_t controlBit = reader.readBit();
        uint64_t length = reader.readBits(Mc * 8);

        if (controlBit == 1) {
            for (uint64_t j = 0; j < length; j++) {
                for (uint8_t k = 0; k < Ms; k++) {
                    if (!reader.hasMore()) break;
                    output.push_back(static_cast<uint8_t>(reader.readBits(8)));
                }
            }
        }
        else {
            vector<uint8_t> symbol;
            for (uint8_t k = 0; k < Ms; k++) {
                if (!reader.hasMore()) break;
                symbol.push_back(static_cast<uint8_t>(reader.readBits(8)));
            }
            for (uint64_t j = 0; j < length; j++) {
                output.insert(output.end(), symbol.begin(), symbol.end());
            }
        }
    }

    return output;
}

bool RLE::encodeFile(const string& inputPath,
    const string& outputPath,
    uint8_t Ms,
    uint8_t Mc,
    string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
    inFile.close();

    size_t originalSize = inputData.size();

    vector<uint8_t> encodedData = encode(inputData, Ms, Mc);

    RLEHeader header;
    header.Ms = Ms;
    header.Mc = Mc;
    header.originalSize = originalSize;
    header.encodedSize = encodedData.size();

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(&header.Ms), sizeof(header.Ms));
    outFile.write(reinterpret_cast<const char*>(&header.Mc), sizeof(header.Mc));
    outFile.write(reinterpret_cast<const char*>(&header.originalSize), sizeof(header.originalSize));
    outFile.write(reinterpret_cast<const char*>(&header.encodedSize), sizeof(header.encodedSize));
    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return true;
}

bool RLE::decodeFile(const string& inputPath,
    const string& outputPath,
    string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    RLEHeader header;
    inFile.read(reinterpret_cast<char*>(&header.Ms), sizeof(header.Ms));
    inFile.read(reinterpret_cast<char*>(&header.Mc), sizeof(header.Mc));
    inFile.read(reinterpret_cast<char*>(&header.originalSize), sizeof(header.originalSize));
    inFile.read(reinterpret_cast<char*>(&header.encodedSize), sizeof(header.encodedSize));

    if (inFile.fail()) {
        errorMsg = "Failed to read header";
        return false;
    }

    vector<uint8_t> encodedData(header.encodedSize);
    inFile.read(reinterpret_cast<char*>(encodedData.data()), header.encodedSize);
    inFile.close();

    vector<uint8_t> decodedData = decode(encodedData, header.Ms, header.Mc);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create decoded file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    outFile.close();

    return true;
}

CompressionStats RLE::analyzeFile(const string& inputPath, uint8_t Ms, uint8_t Mc, string& errorMsg) {
    CompressionStats stats;
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

    size_t totalRuns = 0;
    size_t totalRunLength = 0;
    size_t currentRun = 1;

    for (size_t i = Ms; i < inputData.size(); i += Ms) {
        bool same = true;
        for (uint8_t k = 0; k < Ms && same; k++) {
            if (inputData[i - Ms + k] != inputData[i + k]) {
                same = false;
            }
        }

        if (same) {
            currentRun++;
        }
        else {
            if (currentRun > 1) {
                totalRuns++;
                totalRunLength += currentRun;
            }
            currentRun = 1;
        }
    }

    if (currentRun > 1) {
        totalRuns++;
        totalRunLength += currentRun;
    }

    size_t totalSymbols = inputData.size() / Ms;

    if (totalRuns > 0) {
        double encodedEstimate = totalRuns * (Mc + Ms);
        double literalEstimate = (totalSymbols - totalRunLength) * (1 + Ms);
        double totalEstimate = encodedEstimate + literalEstimate;
        stats.compressionRatio = totalEstimate / inputData.size() * 100.0;
    }
    else {
        stats.compressionRatio = 100.0;
    }

    stats.compressionFactor = 100.0 / stats.compressionRatio;
    stats.savingsPercent = 100.0 - stats.compressionRatio;

    return stats;
}

CompressionStats RLE::compressFile(const string& inputPath,
    const string& outputPath,
    uint8_t Ms,
    uint8_t Mc,
    string& errorMsg) {
    CompressionStats stats;
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

    vector<uint8_t> encodedData = encode(inputData, Ms, Mc);

    RLEHeader header;
    header.Ms = Ms;
    header.Mc = Mc;
    header.originalSize = stats.originalSize;
    header.encodedSize = encodedData.size();

    stats.encodedSize = sizeof(header) + encodedData.size();
    stats.compressionRatio = (double)stats.encodedSize / stats.originalSize * 100.0;
    stats.compressionFactor = (double)stats.originalSize / stats.encodedSize;
    stats.savingsPercent = 100.0 - stats.compressionRatio;

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return stats;
    }

    outFile.write(reinterpret_cast<const char*>(&header.Ms), sizeof(header.Ms));
    outFile.write(reinterpret_cast<const char*>(&header.Mc), sizeof(header.Mc));
    outFile.write(reinterpret_cast<const char*>(&header.originalSize), sizeof(header.originalSize));
    outFile.write(reinterpret_cast<const char*>(&header.encodedSize), sizeof(header.encodedSize));
    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    outFile.close();

    return stats;
}

double RLE::getCompressionRatio(size_t originalSize, size_t encodedSize) {
    if (originalSize == 0) return 0.0;
    return (double)encodedSize / originalSize * 100.0;
}

bool RLE::verifyCycle(const string& inputPath,
    const string& encodedPath,
    const string& decodedPath,
    uint8_t Ms,
    uint8_t Mc,
    string& errorMsg) {
    if (!encodeFile(inputPath, encodedPath, Ms, Mc, errorMsg)) {
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

void RLE::printStats(const CompressionStats& stats) {
    cout << "  Original: " << stats.originalSize << " bytes\n";
    cout << "  Encoded:  " << stats.encodedSize << " bytes\n";
    cout << "  Ratio:    " << fixed << setprecision(2) << stats.compressionRatio << "% of original\n";
    cout << "  Factor:   " << fixed << setprecision(2) << stats.compressionFactor << ":1\n";
}