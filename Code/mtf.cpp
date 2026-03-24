#include "mtf.h"
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdint>

using namespace std;

vector<uint8_t> MTF::encode(const vector<uint8_t>& input) {
    vector<uint8_t> output;

    if (input.empty()) {
        return output;
    }

    vector<uint8_t> symbolList(256);
    for (int i = 0; i < 256; i++) {
        symbolList[i] = i;
    }

    for (size_t i = 0; i < input.size(); i++) {
        uint8_t symbol = input[i];

        size_t pos = 0;
        while (pos < symbolList.size() && symbolList[pos] != symbol) {
            pos++;
        }

        output.push_back(static_cast<uint8_t>(pos));

        for (size_t j = pos; j > 0; j--) {
            symbolList[j] = symbolList[j - 1];
        }
        symbolList[0] = symbol;
    }

    return output;
}

vector<uint8_t> MTF::decode(const vector<uint8_t>& input) {
    vector<uint8_t> output;

    if (input.empty()) {
        return output;
    }

    vector<uint8_t> symbolList(256);
    for (int i = 0; i < 256; i++) {
        symbolList[i] = i;
    }

    for (size_t i = 0; i < input.size(); i++) {
        uint8_t index = input[i];

        if (index >= symbolList.size()) {
            return output;
        }

        uint8_t symbol = symbolList[index];
        output.push_back(symbol);

        for (size_t j = index; j > 0; j--) {
            symbolList[j] = symbolList[j - 1];
        }
        symbolList[0] = symbol;
    }

    return output;
}

bool MTF::encodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open input file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
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

bool MTF::decodeFile(const string& inputPath, const string& outputPath, string& errorMsg) {
    ifstream inFile(inputPath, ios::binary);
    if (!inFile.is_open()) {
        errorMsg = "Cannot open encoded file: " + inputPath;
        return false;
    }

    vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)),
        istreambuf_iterator<char>());
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

bool MTF::verifyCycle(const string& inputPath, const string& encodedPath,
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