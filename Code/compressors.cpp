#include "compressors.h"
#include "rle.h"
#include "huffman_canonical.h"
#include "bwt.h"
#include "mtf.h"
#include "lzss.h"
#include "lzw.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <functional>

using namespace std;
using namespace chrono;

CompressionResult Compressors::HA(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string encodedPath = outputPath + ".huf";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();
    string errorMsg;
    bool success = HuffmanCanonical::encodeFile(inputPath, encodedPath, errorMsg);
    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream encodedFile(encodedPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(encodedFile)), istreambuf_iterator<char>());
    encodedFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();
    success = HuffmanCanonical::decodeFile(encodedPath, decodedPath, errorMsg);
    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);
    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    result.verified = (origData.size() == decData.size() &&
        memcmp(origData.data(), decData.data(), origData.size()) == 0);

    return result;
}

CompressionResult Compressors::RLE(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string encodedPath = outputPath + ".rle";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();
    string errorMsg;
    bool success = RLE::encodeFile(inputPath, encodedPath, 1, 2, errorMsg);
    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream encodedFile(encodedPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(encodedFile)), istreambuf_iterator<char>());
    encodedFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();
    success = RLE::decodeFile(encodedPath, decodedPath, errorMsg);
    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);
    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    result.verified = (origData.size() == decData.size() &&
        memcmp(origData.data(), decData.data(), origData.size()) == 0);

    return result;
}

CompressionResult Compressors::BWTRLE(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string bwtPath = outputPath + "_bwt.bin";
    string rlePath = outputPath + ".rle";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();

    BWTResult bwtResult = BWT::encodeEfficient(inputData);

    ofstream bwtFile(bwtPath, ios::binary);
    uint64_t primaryIdx = bwtResult.primaryIndex;
    uint64_t dataSize = bwtResult.transformed.size();
    bwtFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    bwtFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
    bwtFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
    bwtFile.close();

    string errorMsg;
    RLE::encodeFile(bwtPath, rlePath, 1, 2, errorMsg);

    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream rleFile(rlePath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(rleFile)), istreambuf_iterator<char>());
    rleFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();

    string tempDecode = outputPath + "_temp.bin";
    RLE::decodeFile(rlePath, tempDecode, errorMsg);

    ifstream tempFile(tempDecode, ios::binary);
    uint64_t dataSize2;
    uint64_t primaryIdx2;
    tempFile.read(reinterpret_cast<char*>(&dataSize2), sizeof(dataSize2));
    tempFile.read(reinterpret_cast<char*>(&primaryIdx2), sizeof(primaryIdx2));
    vector<uint8_t> bwtData(dataSize2);
    tempFile.read(reinterpret_cast<char*>(bwtData.data()), dataSize2);
    tempFile.close();

    vector<uint8_t> decodedData = BWT::decode(bwtData, primaryIdx2);

    ofstream decFile(decodedPath, ios::binary);
    decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    decFile.close();

    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    result.verified = (decodedData.size() == inputData.size() &&
        memcmp(decodedData.data(), inputData.data(), inputData.size()) == 0);

    return result;
}

CompressionResult Compressors::BWTMTFHA(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string bwtPath = outputPath + "_bwt.bin";
    string mtfPath = outputPath + "_mtf.bin";
    string haPath = outputPath + ".ha";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();

    BWTResult bwtResult = BWT::encodeEfficient(inputData);

    ofstream bwtFile(bwtPath, ios::binary);
    uint64_t primaryIdx = bwtResult.primaryIndex;
    uint64_t dataSize = bwtResult.transformed.size();
    bwtFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    bwtFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
    bwtFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
    bwtFile.close();

    vector<uint8_t> mtfEncoded = MTF::encode(bwtResult.transformed);

    ofstream mtfFile(mtfPath, ios::binary);
    mtfFile.write(reinterpret_cast<const char*>(mtfEncoded.data()), mtfEncoded.size());
    mtfFile.close();

    string errorMsg;
    HuffmanCanonical::encodeFile(mtfPath, haPath, errorMsg);

    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream haFile(haPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(haFile)), istreambuf_iterator<char>());
    haFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();

    string tempDecode = outputPath + "_temp.bin";
    HuffmanCanonical::decodeFile(haPath, tempDecode, errorMsg);

    ifstream tempFile(tempDecode, ios::binary);
    vector<uint8_t> mtfData((istreambuf_iterator<char>(tempFile)), istreambuf_iterator<char>());
    tempFile.close();

    vector<uint8_t> mtfDecoded = MTF::decode(mtfData);

    ifstream bwtInFile(bwtPath, ios::binary);
    uint64_t dataSize2;
    uint64_t primaryIdx2;
    bwtInFile.read(reinterpret_cast<char*>(&dataSize2), sizeof(dataSize2));
    bwtInFile.read(reinterpret_cast<char*>(&primaryIdx2), sizeof(primaryIdx2));
    vector<uint8_t> bwtData(dataSize2);
    bwtInFile.read(reinterpret_cast<char*>(bwtData.data()), dataSize2);
    bwtInFile.close();

    vector<uint8_t> decodedData = BWT::decode(mtfDecoded, primaryIdx2);

    ofstream decFile(decodedPath, ios::binary);
    decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    decFile.close();

    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    result.verified = (decodedData.size() == inputData.size() &&
        memcmp(decodedData.data(), inputData.data(), inputData.size()) == 0);

    return result;
}

CompressionResult Compressors::BWTMTFRLEHA(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string bwtPath = outputPath + "_bwt.bin";
    string mtfPath = outputPath + "_mtf.bin";
    string rlePath = outputPath + ".rle";
    string haPath = outputPath + ".ha";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();

    BWTResult bwtResult = BWT::encodeEfficient(inputData);

    ofstream bwtFile(bwtPath, ios::binary);
    uint64_t primaryIdx = bwtResult.primaryIndex;
    uint64_t dataSize = bwtResult.transformed.size();
    bwtFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    bwtFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
    bwtFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
    bwtFile.close();

    vector<uint8_t> mtfEncoded = MTF::encode(bwtResult.transformed);

    ofstream mtfFile(mtfPath, ios::binary);
    mtfFile.write(reinterpret_cast<const char*>(mtfEncoded.data()), mtfEncoded.size());
    mtfFile.close();

    string errorMsg;
    RLE::encodeFile(mtfPath, rlePath, 1, 2, errorMsg);
    HuffmanCanonical::encodeFile(rlePath, haPath, errorMsg);

    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream haFile(haPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(haFile)), istreambuf_iterator<char>());
    haFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();

    string tempDecode1 = outputPath + "_temp1.bin";
    string tempDecode2 = outputPath + "_temp2.bin";
    HuffmanCanonical::decodeFile(haPath, tempDecode1, errorMsg);
    RLE::decodeFile(tempDecode1, tempDecode2, errorMsg);

    ifstream tempFile(tempDecode2, ios::binary);
    vector<uint8_t> mtfData((istreambuf_iterator<char>(tempFile)), istreambuf_iterator<char>());
    tempFile.close();

    vector<uint8_t> mtfDecoded = MTF::decode(mtfData);

    ifstream bwtInFile(bwtPath, ios::binary);
    uint64_t dataSize2;
    uint64_t primaryIdx2;
    bwtInFile.read(reinterpret_cast<char*>(&dataSize2), sizeof(dataSize2));
    bwtInFile.read(reinterpret_cast<char*>(&primaryIdx2), sizeof(primaryIdx2));
    vector<uint8_t> bwtData(dataSize2);
    bwtInFile.read(reinterpret_cast<char*>(bwtData.data()), dataSize2);
    bwtInFile.close();

    vector<uint8_t> decodedData = BWT::decode(mtfDecoded, primaryIdx2);

    ofstream decFile(decodedPath, ios::binary);
    decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    decFile.close();

    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    result.verified = (decodedData.size() == inputData.size() &&
        memcmp(decodedData.data(), inputData.data(), inputData.size()) == 0);

    return result;
}

CompressionResult Compressors::LZSS(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string encodedPath = outputPath + ".lzss";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();
    string errorMsg;
    bool success = LZSS::encodeFile(inputPath, encodedPath, 4096, 16, errorMsg);
    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream encodedFile(encodedPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(encodedFile)), istreambuf_iterator<char>());
    encodedFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();
    success = LZSS::decodeFile(encodedPath, decodedPath, errorMsg);
    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);
    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    result.verified = (origData.size() == decData.size() &&
        memcmp(origData.data(), decData.data(), origData.size()) == 0);

    return result;
}

CompressionResult Compressors::LZSSHA(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string lzssPath = outputPath + ".lzss";
    string haPath = outputPath + ".ha";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();
    string errorMsg;
    LZSS::encodeFile(inputPath, lzssPath, 4096, 16, errorMsg);
    HuffmanCanonical::encodeFile(lzssPath, haPath, errorMsg);
    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream haFile(haPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(haFile)), istreambuf_iterator<char>());
    haFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();

    string tempDecode = outputPath + "_temp.bin";
    HuffmanCanonical::decodeFile(haPath, tempDecode, errorMsg);
    LZSS::decodeFile(tempDecode, decodedPath, errorMsg);

    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);
    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    result.verified = (origData.size() == decData.size() &&
        memcmp(origData.data(), decData.data(), origData.size()) == 0);

    return result;
}

CompressionResult Compressors::LZW(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string encodedPath = outputPath + ".lzw";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();
    string errorMsg;
    bool success = LZW::encodeFile(inputPath, encodedPath, 4096, errorMsg);
    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream encodedFile(encodedPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(encodedFile)), istreambuf_iterator<char>());
    encodedFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();
    success = LZW::decodeFile(encodedPath, decodedPath, errorMsg);
    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    if (!success) {
        result.errorMsg = errorMsg;
        return result;
    }

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);
    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    result.verified = (origData.size() == decData.size() &&
        memcmp(origData.data(), decData.data(), origData.size()) == 0);

    return result;
}

CompressionResult Compressors::LZWHA(const string& inputPath, const string& outputPath) {
    CompressionResult result;
    result.filename = inputPath.substr(inputPath.find_last_of("/\\") + 1);

    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        result.errorMsg = "Cannot open input file";
        return result;
    }
    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    result.originalSize = inputData.size();

    string lzwPath = outputPath + ".lzw";
    string haPath = outputPath + ".ha";
    string decodedPath = outputPath + ".dec";

    auto start = high_resolution_clock::now();
    string errorMsg;
    LZW::encodeFile(inputPath, lzwPath, 4096, errorMsg);
    HuffmanCanonical::encodeFile(lzwPath, haPath, errorMsg);
    auto end = high_resolution_clock::now();
    result.encodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream haFile(haPath, ios::binary);
    vector<uint8_t> encodedData((istreambuf_iterator<char>(haFile)), istreambuf_iterator<char>());
    haFile.close();
    result.compressedSize = encodedData.size();
    result.compressionRatio = (double)result.compressedSize / result.originalSize * 100.0;
    result.compressionFactor = (double)result.originalSize / result.compressedSize;

    start = high_resolution_clock::now();

    string tempDecode = outputPath + "_temp.bin";
    HuffmanCanonical::decodeFile(haPath, tempDecode, errorMsg);
    LZW::decodeFile(tempDecode, decodedPath, errorMsg);

    end = high_resolution_clock::now();
    result.decodeTimeMs = duration_cast<milliseconds>(end - start).count();

    ifstream origFile(inputPath, ios::binary);
    ifstream decFile(decodedPath, ios::binary);
    vector<uint8_t> origData((istreambuf_iterator<char>(origFile)), istreambuf_iterator<char>());
    vector<uint8_t> decData((istreambuf_iterator<char>(decFile)), istreambuf_iterator<char>());

    result.verified = (origData.size() == decData.size() &&
        memcmp(origData.data(), decData.data(), origData.size()) == 0);

    return result;
}

void Compressors::printResult(const CompressionResult& result) {
    if (!result.errorMsg.empty()) {
        cout << "  Error: " << result.errorMsg << "\n";
        return;
    }

    cout << "  Original: " << result.originalSize << " bytes\n";
    cout << "  Compressed: " << result.compressedSize << " bytes\n";
    cout << "  Ratio: " << fixed << setprecision(2) << result.compressionRatio << "%\n";
    cout << "  Factor: " << fixed << setprecision(2) << result.compressionFactor << ":1\n";
    cout << "  Encode time: " << result.encodeTimeMs << " ms\n";
    cout << "  Decode time: " << result.decodeTimeMs << " ms\n";
    cout << "  Status: " << (result.verified ? "PASSED" : "FAILED") << "\n";
}

void Compressors::printSummary(const vector<CompressionResult>& results) {
    cout << "\n========================================\n";
    cout << "Compression Summary\n";
    cout << "========================================\n\n";

    for (const auto& result : results) {
        cout << result.filename << ":\n";
        printResult(result);
        cout << "\n";
    }
}