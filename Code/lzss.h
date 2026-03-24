#ifndef LZSS_H
#define LZSS_H

#include <vector>
#include <cstdint>
#include <string>

// Структура заголовка файла LZSS
struct LZSSHeader {
    uint32_t magicNumber;     // Магическое число для идентификации формата
    uint8_t version;          // Версия формата
    uint64_t originalSize;    // Исходный размер данных
    uint64_t compressedSize;  // Размер сжатых данных
    uint32_t windowSize;      // Размер скользящего окна (буфера поиска)
    uint32_t lookaheadSize;   // Размер буфера просмотра вперед
    uint32_t checksum;        // Контрольная сумма для проверки целостности
};

// Структура для статистики сжатия LZSS
struct LZSSStats {
    std::string filename;
    size_t originalSize;
    size_t compressedSize;
    size_t numLiterals;
    size_t numMatches;
    double compressionRatio;
    double compressionFactor;
    double averageMatchLength;
    double averageOffset;
    double savingsPercent;
};

class LZSS {
public:
    // Основные функции кодирования/декодирования
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input,
        uint32_t windowSize,
        uint32_t lookaheadSize);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);

    // Функции для работы с файлами
    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        uint32_t windowSize,
        uint32_t lookaheadSize,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    // Функции для анализа и статистики
    static LZSSStats compressFile(const std::string& inputPath,
        const std::string& outputPath,
        uint32_t windowSize,
        uint32_t lookaheadSize,
        std::string& errorMsg);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        uint32_t windowSize,
        uint32_t lookaheadSize,
        std::string& errorMsg);

    static void printStats(const LZSSStats& stats);

private:
    // Вспомогательные функции
    static uint32_t calculateChecksum(const std::vector<uint8_t>& data);
    static std::pair<uint16_t, uint16_t> findLongestMatch(
        const std::vector<uint8_t>& data,
        size_t currentPos,
        uint32_t windowSize,
        uint32_t lookaheadSize);
    static std::vector<uint8_t> encodeTokens(const std::vector<uint8_t>& input,
        uint32_t windowSize,
        uint32_t lookaheadSize);
    static std::vector<uint8_t> decodeTokens(const std::vector<uint8_t>& compressed,
        size_t originalSize);
};

#endif