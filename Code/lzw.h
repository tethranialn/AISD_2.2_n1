#ifndef LZW_H
#define LZW_H

#include <vector>
#include <cstdint>
#include <string>
#include <map>

// Структура заголовка файла LZW
struct LZWHeader {
    uint32_t magicNumber;     // Магическое число для идентификации формата (0x4C5A57)
    uint8_t version;          // Версия формата
    uint64_t originalSize;    // Исходный размер данных
    uint64_t compressedSize;  // Размер сжатых данных
    uint32_t maxDictSize;     // Максимальный размер словаря (обычно 4096 или 65536)
    uint32_t checksum;        // Контрольная сумма для проверки целостности
};

// Структура для статистики сжатия LZW
struct LZWStats {
    std::string filename;
    size_t originalSize;
    size_t compressedSize;
    size_t numCodes;
    size_t dictSize;
    double compressionRatio;
    double compressionFactor;
    double averageCodeLength;
    double savingsPercent;
};

class LZW {
public:
    // Основные функции кодирования/декодирования
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input,
        uint32_t maxDictSize = 4096);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);

    // Функции для работы с файлами
    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        uint32_t maxDictSize,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    // Функции для анализа и статистики
    static LZWStats compressFile(const std::string& inputPath,
        const std::string& outputPath,
        uint32_t maxDictSize,
        std::string& errorMsg);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        uint32_t maxDictSize,
        std::string& errorMsg);

    static void printStats(const LZWStats& stats);

private:
    // Вспомогательные функции
    static uint32_t calculateChecksum(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> encodeTokens(const std::vector<uint8_t>& input,
        uint32_t maxDictSize);
    static std::vector<uint8_t> decodeTokens(const std::vector<uint8_t>& compressed,
        size_t originalSize,
        uint32_t maxDictSize);
    static uint8_t getCodeBitSize(uint32_t maxDictSize);
    static std::map<std::vector<uint8_t>, uint32_t> buildInitialDictionary();
};

#endif