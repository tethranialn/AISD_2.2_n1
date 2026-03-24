#include "lzw.h"
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

// Константы
const uint32_t LZW_MAGIC = 0x4C5A57; // "LZW"
const uint8_t LZW_VERSION = 1;

uint32_t LZW::calculateChecksum(const vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 5) - checksum + byte;
    }
    return checksum;
}

uint8_t LZW::getCodeBitSize(uint32_t maxDictSize) {
    if (maxDictSize == 0) return 8;
    return static_cast<uint8_t>(ceil(log2(maxDictSize)));
}

map<vector<uint8_t>, uint32_t> LZW::buildInitialDictionary() {
    map<vector<uint8_t>, uint32_t> dictionary;

    // Инициализируем словарь всеми возможными байтами (0-255)
    for (int i = 0; i < 256; i++) {
        vector<uint8_t> entry;
        entry.push_back(static_cast<uint8_t>(i));
        dictionary[entry] = i;
    }

    return dictionary;
}

vector<uint8_t> LZW::encodeTokens(const vector<uint8_t>& input, uint32_t maxDictSize) {
    BitWriter writer;

    if (input.empty()) {
        return writer.getData();
    }

    // Инициализируем словарь всеми возможными байтами
    map<vector<uint8_t>, uint32_t> dictionary = buildInitialDictionary();
    uint32_t nextCode = 256; // Следующий доступный код
    uint8_t codeBitSize = getCodeBitSize(maxDictSize);

    vector<uint8_t> current;
    current.push_back(input[0]);

    size_t pos = 1;
    size_t n = input.size();

    while (pos < n) {
        vector<uint8_t> next = current;
        next.push_back(input[pos]);

        // Если строка есть в словаре, продолжаем
        if (dictionary.find(next) != dictionary.end()) {
            current = next;
            pos++;
            continue;
        }

        // Кодируем текущую строку
        uint32_t code = dictionary[current];
        writer.writeBits(code, codeBitSize);

        // Добавляем новую строку в словарь, если не превышен лимит
        if (nextCode < maxDictSize) {
            dictionary[next] = nextCode++;
        }

        // Начинаем новую строку с текущего символа
        current.clear();
        current.push_back(input[pos]);
        pos++;
    }

    // Кодируем последнюю строку
    if (!current.empty()) {
        uint32_t code = dictionary[current];
        writer.writeBits(code, codeBitSize);
    }

    writer.flush();
    return writer.getData();
}

vector<uint8_t> LZW::decodeTokens(const vector<uint8_t>& compressed,
    size_t originalSize,
    uint32_t maxDictSize) {
    BitReader reader(compressed);
    vector<uint8_t> output;
    output.reserve(originalSize);

    if (compressed.empty()) {
        return output;
    }

    // Для декодирования LZW не нужно сохранять начальный словарь отдельно,
    // так как он всегда одинаковый для всех данных (0-255)
    // Словарь восстанавливается в процессе декодирования
    map<uint32_t, vector<uint8_t>> dictionary;

    // Инициализируем словарь всеми возможными байтами
    for (int i = 0; i < 256; i++) {
        vector<uint8_t> entry;
        entry.push_back(static_cast<uint8_t>(i));
        dictionary[i] = entry;
    }

    uint32_t nextCode = 256;
    uint8_t codeBitSize = getCodeBitSize(maxDictSize);

    // Читаем первый код
    if (!reader.hasMore()) {
        return output;
    }

    uint32_t prevCode = static_cast<uint32_t>(reader.readBits(codeBitSize));
    vector<uint8_t> prevString = dictionary[prevCode];
    output.insert(output.end(), prevString.begin(), prevString.end());

    while (output.size() < originalSize && reader.hasMore()) {
        uint32_t currentCode = static_cast<uint32_t>(reader.readBits(codeBitSize));

        vector<uint8_t> currentString;

        // Если код есть в словаре
        if (dictionary.find(currentCode) != dictionary.end()) {
            currentString = dictionary[currentCode];
        }
        // Специальный случай: код равен следующему доступному
        else if (currentCode == nextCode) {
            currentString = prevString;
            currentString.push_back(prevString[0]);
        }
        else {
            // Ошибка: неизвестный код
            return vector<uint8_t>();
        }

        // Выводим строку
        output.insert(output.end(), currentString.begin(), currentString.end());

        // Добавляем новую строку в словарь
        if (nextCode < maxDictSize) {
            vector<uint8_t> newString = prevString;
            newString.push_back(currentString[0]);
            dictionary[nextCode++] = newString;
        }

        prevString = currentString;
        prevCode = currentCode;
    }

    return output;
}

vector<uint8_t> LZW::encode(const vector<uint8_t>& input, uint32_t maxDictSize) {
    if (input.empty()) {
        vector<uint8_t> result(sizeof(LZWHeader));
        LZWHeader header;
        header.magicNumber = LZW_MAGIC;
        header.version = LZW_VERSION;
        header.originalSize = 0;
        header.compressedSize = 0;
        header.maxDictSize = maxDictSize;
        header.checksum = 0;
        memcpy(result.data(), &header, sizeof(LZWHeader));
        return result;
    }

    vector<uint8_t> compressedData = encodeTokens(input, maxDictSize);

    LZWHeader header;
    header.magicNumber = LZW_MAGIC;
    header.version = LZW_VERSION;
    header.originalSize = input.size();
    header.compressedSize = compressedData.size();
    header.maxDictSize = maxDictSize;
    header.checksum = calculateChecksum(compressedData);

    vector<uint8_t> result(sizeof(LZWHeader) + compressedData.size());
    memcpy(result.data(), &header, sizeof(LZWHeader));
    memcpy(result.data() + sizeof(LZWHeader), compressedData.data(), compressedData.size());

    return result;
}

vector<uint8_t> LZW::decode(const vector<uint8_t>& input) {
    if (input.size() < sizeof(LZWHeader)) {
        return vector<uint8_t>();
    }

    LZWHeader header;
    memcpy(&header, input.data(), sizeof(LZWHeader));

    if (header.magicNumber != LZW_MAGIC) {
        return vector<uint8_t>();
    }

    if (header.version != LZW_VERSION) {
        return vector<uint8_t>();
    }

    if (header.originalSize == 0) {
        return vector<uint8_t>();
    }

    if (input.size() < sizeof(LZWHeader) + header.compressedSize) {
        return vector<uint8_t>();
    }

    vector<uint8_t> compressedData(input.begin() + sizeof(LZWHeader),
        input.begin() + sizeof(LZWHeader) + header.compressedSize);

    uint32_t checksum = calculateChecksum(compressedData);
    if (checksum != header.checksum) {
        return vector<uint8_t>();
    }

    return decodeTokens(compressedData, header.originalSize, header.maxDictSize);
}

bool LZW::encodeFile(const string& inputPath,
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

bool LZW::decodeFile(const string& inputPath,
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

LZWStats LZW::compressFile(const string& inputPath,
    const string& outputPath,
    uint32_t maxDictSize,
    string& errorMsg) {
    LZWStats stats;
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

    // Подсчитываем статистику
    map<vector<uint8_t>, uint32_t> dictionary = buildInitialDictionary();
    uint32_t nextCode = 256;

    vector<uint8_t> current;
    current.push_back(inputData[0]);

    size_t pos = 1;
    size_t n = inputData.size();
    stats.numCodes = 1; // Первый код

    while (pos < n) {
        vector<uint8_t> next = current;
        next.push_back(inputData[pos]);

        if (dictionary.find(next) != dictionary.end()) {
            current = next;
            pos++;
            continue;
        }

        // Добавляем новый код
        stats.numCodes++;
        if (nextCode < maxDictSize) {
            dictionary[next] = nextCode++;
        }

        current.clear();
        current.push_back(inputData[pos]);
        pos++;
    }

    stats.dictSize = min(nextCode, maxDictSize);
    uint8_t codeBitSize = getCodeBitSize(maxDictSize);
    stats.averageCodeLength = codeBitSize;

    // Кодируем данные
    vector<uint8_t> encodedData = encode(inputData, maxDictSize);
    stats.compressedSize = encodedData.size();
    stats.compressionRatio = (double)stats.compressedSize / stats.originalSize * 100.0;
    stats.compressionFactor = (double)stats.originalSize / stats.compressedSize;
    stats.savingsPercent = 100.0 - stats.compressionRatio;

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

bool LZW::verifyCycle(const string& inputPath,
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

void LZW::printStats(const LZWStats& stats) {
    cout << "  Original: " << stats.originalSize << " bytes\n";
    cout << "  Compressed: " << stats.compressedSize << " bytes\n";
    cout << "  Ratio: " << fixed << setprecision(2)
        << stats.compressionRatio << "% of original\n";
    cout << "  Factor: " << fixed << setprecision(2)
        << stats.compressionFactor << ":1\n";
}