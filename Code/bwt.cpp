#include "bwt.h"
#include "rle.h"
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/stat.h>
#include "raw_convert.h"

using namespace std;
using namespace chrono;

bool fileExistsBWT(const string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

vector<vector<uint8_t>> BWT::buildRotationMatrix(const vector<uint8_t>& input) {
    size_t n = input.size();
    vector<vector<uint8_t>> matrix(n, vector<uint8_t>(n));

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            matrix[i][j] = input[(i + j) % n];
        }
    }

    return matrix;
}

vector<size_t> BWT::getSortedIndices(const vector<vector<uint8_t>>& matrix) {
    size_t n = matrix.size();
    vector<size_t> indices(n);

    for (size_t i = 0; i < n; i++) {
        indices[i] = i;
    }

    sort(indices.begin(), indices.end(), [&matrix](size_t a, size_t b) {
        return matrix[a] < matrix[b];
        });

    return indices;
}

vector<uint8_t> BWT::getLastColumn(const vector<vector<uint8_t>>& matrix,
    const vector<size_t>& indices) {
    size_t n = matrix.size();
    vector<uint8_t> lastColumn(n);

    for (size_t i = 0; i < n; i++) {
        lastColumn[i] = matrix[indices[i]][n - 1];
    }

    return lastColumn;
}

BWTResult BWT::encode(const vector<uint8_t>& input) {
    BWTResult result;

    if (input.empty()) {
        result.description = "Empty input";
        return result;
    }

    size_t n = input.size();

    vector<vector<uint8_t>> matrix = buildRotationMatrix(input);

    vector<size_t> sortedIndices = getSortedIndices(matrix);

    result.transformed = getLastColumn(matrix, sortedIndices);

    for (size_t i = 0; i < n; i++) {
        if (sortedIndices[i] == 0) {
            result.primaryIndex = i;
            break;
        }
    }

    result.description = "BWT encoding successful";

    return result;
}

vector<uint8_t> BWT::reconstructUsingLFMapping(const vector<uint8_t>& lastColumn,
    size_t primaryIndex) {
    size_t n = lastColumn.size();
    vector<uint8_t> result(n);

    vector<pair<uint8_t, size_t>> column(n);
    for (size_t i = 0; i < n; i++) {
        column[i] = { lastColumn[i], i };
    }

    sort(column.begin(), column.end());

    size_t current = primaryIndex;
    for (size_t i = 0; i < n; i++) {
        result[i] = column[current].first;
        current = column[current].second;
    }

    return result;
}

vector<uint8_t> BWT::reconstructUsingPermutation(const vector<uint8_t>& lastColumn,
    size_t primaryIndex) {
    size_t n = lastColumn.size();
    vector<uint8_t> result(n);

    const int ALPHABET_SIZE = 256;
    vector<int> count(ALPHABET_SIZE, 0);

    for (size_t i = 0; i < n; i++) {
        count[lastColumn[i]]++;
    }

    vector<int> cumulativeCount(ALPHABET_SIZE, 0);
    for (int i = 1; i < ALPHABET_SIZE; i++) {
        cumulativeCount[i] = cumulativeCount[i - 1] + count[i - 1];
    }

    vector<size_t> firstColumnIndices(n);
    vector<int> tempCount = cumulativeCount;

    for (size_t i = 0; i < n; i++) {
        uint8_t symbol = lastColumn[i];
        firstColumnIndices[tempCount[symbol]] = i;
        tempCount[symbol]++;
    }

    vector<size_t> nextIndices(n);
    for (size_t i = 0; i < n; i++) {
        nextIndices[firstColumnIndices[i]] = i;
    }

    size_t current = primaryIndex;
    for (size_t i = 0; i < n; i++) {
        result[n - 1 - i] = lastColumn[current];
        current = nextIndices[current];
    }

    return result;
}

vector<uint8_t> BWT::decode(const vector<uint8_t>& input, size_t primaryIndex) {
    return reconstructUsingPermutation(input, primaryIndex);
}

vector<uint8_t> BWT::decodeWithCountingSort(const vector<uint8_t>& input, size_t primaryIndex) {
    return reconstructUsingPermutation(input, primaryIndex);
}

BWTBlockResult BWT::encodeBlocked(const vector<uint8_t>& input, size_t blockSize) {
    BWTBlockResult result;
    result.blockSize = blockSize;
    result.originalSize = input.size();

    if (input.empty()) {
        return result;
    }

    size_t numBlocks = (input.size() + blockSize - 1) / blockSize;
    result.transformed.reserve(input.size());
    result.primaryIndices.reserve(numBlocks);

    for (size_t blockIdx = 0; blockIdx < numBlocks; blockIdx++) {
        size_t start = blockIdx * blockSize;
        size_t end = min(start + blockSize, input.size());

        vector<uint8_t> block(input.begin() + start, input.begin() + end);

        if (block.size() < blockSize) {
            block.resize(blockSize, 0);
        }

        BWTResult bwtResult = encode(block);

        result.transformed.insert(result.transformed.end(),
            bwtResult.transformed.begin(),
            bwtResult.transformed.end());
        result.primaryIndices.push_back(bwtResult.primaryIndex);
    }

    return result;
}

vector<uint8_t> BWT::decodeBlocked(const BWTBlockResult& result) {
    vector<uint8_t> output;
    output.reserve(result.originalSize);

    size_t numBlocks = result.primaryIndices.size();
    size_t blockDataSize = result.blockSize;

    for (size_t blockIdx = 0; blockIdx < numBlocks; blockIdx++) {
        size_t start = blockIdx * blockDataSize;
        size_t end = min(start + blockDataSize, result.transformed.size());

        vector<uint8_t> blockData(result.transformed.begin() + start,
            result.transformed.begin() + end);

        vector<uint8_t> decodedBlock = decode(blockData, result.primaryIndices[blockIdx]);

        size_t remaining = result.originalSize - output.size();
        if (decodedBlock.size() > remaining) {
            decodedBlock.resize(remaining);
        }

        output.insert(output.end(), decodedBlock.begin(), decodedBlock.end());
    }

    return output;
}

bool BWT::encodeFileBlocked(const string& inputPath, const string& outputPath,
    size_t blockSize, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    BWTBlockResult bwtResult = encodeBlocked(inputData, blockSize);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    uint64_t magicNumber = 0x42575442;
    uint8_t version = 1;
    uint64_t originalSize = bwtResult.originalSize;
    uint64_t blockSize64 = bwtResult.blockSize;
    uint64_t numBlocks = bwtResult.primaryIndices.size();
    uint64_t dataSize = bwtResult.transformed.size();

    outFile.write(reinterpret_cast<const char*>(&magicNumber), sizeof(magicNumber));
    outFile.write(reinterpret_cast<const char*>(&version), sizeof(version));
    outFile.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));
    outFile.write(reinterpret_cast<const char*>(&blockSize64), sizeof(blockSize64));
    outFile.write(reinterpret_cast<const char*>(&numBlocks), sizeof(numBlocks));
    outFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

    outFile.write(reinterpret_cast<const char*>(bwtResult.primaryIndices.data()),
        numBlocks * sizeof(size_t));
    outFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
    outFile.close();

    return true;
}

bool BWT::decodeFileBlocked(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    uint64_t magicNumber;
    uint8_t version;
    uint64_t originalSize;
    uint64_t blockSize;
    uint64_t numBlocks;
    uint64_t dataSize;

    inFile.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));
    if (magicNumber != 0x42575442) {
        errorMsg = "Invalid file format";
        return false;
    }

    inFile.read(reinterpret_cast<char*>(&version), sizeof(version));
    inFile.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));
    inFile.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
    inFile.read(reinterpret_cast<char*>(&numBlocks), sizeof(numBlocks));
    inFile.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

    vector<size_t> primaryIndices(numBlocks);
    inFile.read(reinterpret_cast<char*>(primaryIndices.data()), numBlocks * sizeof(size_t));

    vector<uint8_t> transformedData(dataSize);
    inFile.read(reinterpret_cast<char*>(transformedData.data()), dataSize);
    inFile.close();

    BWTBlockResult bwtResult;
    bwtResult.originalSize = originalSize;
    bwtResult.blockSize = blockSize;
    bwtResult.primaryIndices = primaryIndices;
    bwtResult.transformed = transformedData;

    vector<uint8_t> decodedData = decodeBlocked(bwtResult);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create decoded file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    outFile.close();

    return true;
}

bool BWT::encodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    BWTResult result = encode(inputData);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create output file: " + outputPath;
        return false;
    }

    uint64_t primaryIdx = result.primaryIndex;
    uint64_t dataSize = result.transformed.size();

    outFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    outFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
    outFile.write(reinterpret_cast<const char*>(result.transformed.data()), result.transformed.size());
    outFile.close();

    return true;
}

bool BWT::decodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    uint64_t dataSize;
    uint64_t primaryIndex;

    inFile.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    inFile.read(reinterpret_cast<char*>(&primaryIndex), sizeof(primaryIndex));

    vector<uint8_t> inputData(dataSize);
    inFile.read(reinterpret_cast<char*>(inputData.data()), dataSize);
    inFile.close();

    vector<uint8_t> decodedData = decode(inputData, primaryIndex);

    ofstream outFile(outputPath, ios::binary);
    if (!outFile.is_open()) {
        errorMsg = "Cannot create decoded file: " + outputPath;
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    outFile.close();

    return true;
}

bool BWT::decodeFileWithCountingSort(const string& inputPath, const string& outputPath, string& errorMsg) {
    return decodeFile(inputPath, outputPath, errorMsg);
}

bool BWT::verifyCycle(const string& inputPath, const string& encodedPath,
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

void BWT::testOnBanana() {
    cout << "\n========================================\n";
    cout << "BWT Transform Test on \"banana\"\n";
    cout << "========================================\n\n";

    string testStr = "banana";
    vector<uint8_t> input;
    for (char c : testStr) {
        input.push_back(static_cast<uint8_t>(c));
    }

    cout << "Original string: \"" << testStr << "\"\n";
    cout << "Original bytes: ";
    for (uint8_t b : input) {
        cout << "0x" << hex << setw(2) << setfill('0') << (int)b << " ";
    }
    cout << dec << "\n\n";

    BWTResult result = BWT::encode(input);

    cout << "BWT result:\n";
    cout << "  Last column: ";
    for (uint8_t b : result.transformed) {
        cout << (char)b;
    }
    cout << "\n";
    cout << "  Last column bytes: ";
    for (uint8_t b : result.transformed) {
        cout << "0x" << hex << setw(2) << setfill('0') << (int)b << " ";
    }
    cout << dec << "\n";
    cout << "  Primary index: " << result.primaryIndex << "\n\n";

    vector<uint8_t> decoded = BWT::decode(result.transformed, result.primaryIndex);

    string decodedStr;
    for (uint8_t b : decoded) {
        decodedStr += (char)b;
    }

    cout << "Decoded string: \"" << decodedStr << "\"\n";
    cout << "Decoded bytes: ";
    for (uint8_t b : decoded) {
        cout << "0x" << hex << setw(2) << setfill('0') << (int)b << " ";
    }
    cout << dec << "\n";
    cout << "Result: " << (testStr == decodedStr ? "PASSED" : "FAILED") << "\n\n";

    cout << "Applying RLE to BWT output:\n";
    vector<uint8_t> rleEncoded = RLE::encode(result.transformed, 1, 2);
    cout << "  BWT output size: " << result.transformed.size() << " bytes\n";
    cout << "  RLE compressed size: " << rleEncoded.size() << " bytes\n";
    cout << "  Compression ratio: " << fixed << setprecision(2)
        << (double)rleEncoded.size() / result.transformed.size() * 100.0 << "% of BWT output\n";
    cout << "  Overall compression (BWT+RLE): " << fixed << setprecision(2)
        << (double)rleEncoded.size() / input.size() * 100.0 << "% of original\n\n";
}

void BWT::testBWTAndRLE(const string& inputDir, const vector<string>& files) {
    size_t blockSize = 4096;

    for (const auto& filename : files) {
        string filePath = inputDir + "\\" + filename;

        if (!fileExistsBWT(filePath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "File: " << filename << "\n";
        cout << "----------------------------------------\n";

        ifstream inFile(filePath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Cannot open file\n\n";
            continue;
        }

        vector<uint8_t> originalData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
        inFile.close();

        cout << "Original size: " << originalData.size() << " bytes\n";
        cout << "Block size: " << blockSize << " bytes\n\n";

        BWTBlockResult bwtResult = encodeBlocked(originalData, blockSize);

        cout << "BWT transformed size: " << bwtResult.transformed.size() << " bytes\n";
        cout << "Number of blocks: " << bwtResult.primaryIndices.size() << "\n";

        vector<uint8_t> rleEncoded = RLE::encode(bwtResult.transformed, 1, 2);

        size_t headerSize = sizeof(uint64_t) * 6 + bwtResult.primaryIndices.size() * sizeof(size_t);
        size_t totalSize = headerSize + rleEncoded.size();

        double ratio = (double)totalSize / originalData.size() * 100.0;
        double factor = (double)originalData.size() / totalSize;

        cout << "BWT+RLE compressed size: " << totalSize << " bytes\n";
        cout << "Compression ratio: " << fixed << setprecision(2) << ratio << "% of original\n";
        cout << "Compression factor: " << fixed << setprecision(2) << factor << ":1\n\n";

        string tempFile = inputDir + "\\temp_bwt_rle_" + filename + ".bin";
        ofstream tempOut(tempFile, ios::binary);
        if (tempOut.is_open()) {
            uint64_t magicNumber = 0x42575452;
            uint8_t version = 1;
            uint64_t originalSize = bwtResult.originalSize;
            uint64_t blockSize64 = bwtResult.blockSize;
            uint64_t numBlocks = bwtResult.primaryIndices.size();
            uint64_t rleSize = rleEncoded.size();

            tempOut.write(reinterpret_cast<const char*>(&magicNumber), sizeof(magicNumber));
            tempOut.write(reinterpret_cast<const char*>(&version), sizeof(version));
            tempOut.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));
            tempOut.write(reinterpret_cast<const char*>(&blockSize64), sizeof(blockSize64));
            tempOut.write(reinterpret_cast<const char*>(&numBlocks), sizeof(numBlocks));
            tempOut.write(reinterpret_cast<const char*>(&rleSize), sizeof(rleSize));
            tempOut.write(reinterpret_cast<const char*>(bwtResult.primaryIndices.data()),
                numBlocks * sizeof(size_t));
            tempOut.write(reinterpret_cast<const char*>(rleEncoded.data()), rleSize);
            tempOut.close();
        }

        ifstream tempIn(tempFile, ios::binary);
        if (tempIn.is_open()) {
            uint64_t magic, origSize, blockSize64, numBlocks, rleSizeVal;
            uint8_t version;

            tempIn.read(reinterpret_cast<char*>(&magic), sizeof(magic));
            tempIn.read(reinterpret_cast<char*>(&version), sizeof(version));
            tempIn.read(reinterpret_cast<char*>(&origSize), sizeof(origSize));
            tempIn.read(reinterpret_cast<char*>(&blockSize64), sizeof(blockSize64));
            tempIn.read(reinterpret_cast<char*>(&numBlocks), sizeof(numBlocks));
            tempIn.read(reinterpret_cast<char*>(&rleSizeVal), sizeof(rleSizeVal));

            vector<size_t> primIndices(numBlocks);
            tempIn.read(reinterpret_cast<char*>(primIndices.data()), numBlocks * sizeof(size_t));

            vector<uint8_t> rleData(rleSizeVal);
            tempIn.read(reinterpret_cast<char*>(rleData.data()), rleSizeVal);
            tempIn.close();

            vector<uint8_t> bwtDecoded = RLE::decode(rleData, 1, 2);

            BWTBlockResult decodeResult;
            decodeResult.originalSize = origSize;
            decodeResult.blockSize = blockSize64;
            decodeResult.primaryIndices = primIndices;
            decodeResult.transformed = bwtDecoded;

            vector<uint8_t> finalDecoded = decodeBlocked(decodeResult);

            if (originalData.size() == finalDecoded.size() &&
                memcmp(originalData.data(), finalDecoded.data(), originalData.size()) == 0) {
                cout << "Verification: PASSED\n";
            }
            else {
                cout << "Verification: FAILED\n";
            }

            remove(tempFile.c_str());
        }

        cout << "\n";
    }
}

// Новая функция: преобразование суффиксного массива в последний столбец матрицы BWT
vector<uint8_t> BWT::suffixArrayToLastColumn(
    const vector<uint8_t>& input,
    const vector<int32_t>& suffixArray) {

    vector<uint8_t> lastColumn;

    if (input.empty() || suffixArray.empty()) {
        return lastColumn;
    }

    size_t n = input.size();
    lastColumn.reserve(n);

    // Для каждой циклической перестановки, заданной суффиксным массивом,
    // берем последний символ (символ перед началом перестановки)
    for (size_t i = 0; i < n; i++) {
        int32_t startPos = suffixArray[i];

        // Вычисляем позицию последнего символа в циклической перестановке
        // Если startPos == 0, последний символ находится в конце строки (n-1)
        // Иначе последний символ находится на позиции startPos - 1
        int32_t lastPos;
        if (startPos == 0) {
            lastPos = static_cast<int32_t>(n - 1);
        }
        else {
            lastPos = startPos - 1;
        }

        lastColumn.push_back(input[lastPos]);
    }

    return lastColumn;
}