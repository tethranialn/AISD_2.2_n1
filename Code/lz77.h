#ifndef LZ77_H
#define LZ77_H

#include <vector>
#include <cstdint>
#include <string>

// Структура для хранения LZ77 токена
struct LZ77Token {
    uint16_t offset;      // Смещение назад в буфере (0 - если литерал)
    uint16_t length;      // Длина совпадения (0 - если литерал)
    uint8_t nextChar;     // Следующий символ после совпадения или литерал
};

// Структура заголовка файла LZ77
struct LZ77Header {
    uint32_t magicNumber;     // Магическое число для идентификации формата
    uint8_t version;          // Версия формата
    uint64_t originalSize;    // Исходный размер данных
    uint64_t compressedSize;  // Размер сжатых данных
    uint32_t windowSize;      // Размер скользящего окна (буфера поиска)
    uint32_t lookaheadSize;   // Размер буфера просмотра вперед
    uint32_t checksum;        // Контрольная сумма для проверки целостности
};

// Структура для статистики сжатия LZ77
struct LZ77Stats {
    std::string filename;
    size_t originalSize;
    size_t compressedSize;
    size_t numLiterals;       // Количество литеральных токенов
    size_t numMatches;        // Количество токенов с совпадениями
    double compressionRatio;
    double compressionFactor;
    double averageMatchLength;
    double averageOffset;
    double savingsPercent;
};

class LZ77 {
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
    static LZ77Stats analyzeFile(const std::string& inputPath,
        uint32_t windowSize,
        uint32_t lookaheadSize,
        std::string& errorMsg);

    static LZ77Stats compressFile(const std::string& inputPath,
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

    static void printStats(const LZ77Stats& stats);

    // Утилиты для работы с битовыми потоками токенов
    static std::vector<uint8_t> tokensToBytes(const std::vector<LZ77Token>& tokens);
    static std::vector<LZ77Token> bytesToTokens(const std::vector<uint8_t>& bytes);

private:
    // Вспомогательные функции
    static uint32_t calculateChecksum(const std::vector<uint8_t>& data);
    static std::pair<uint16_t, uint16_t> findLongestMatch(
        const std::vector<uint8_t>& data,
        size_t currentPos,
        uint32_t windowSize,
        uint32_t lookaheadSize);
    static std::vector<LZ77Token> compressTokens(const std::vector<uint8_t>& input,
        uint32_t windowSize,
        uint32_t lookaheadSize);
    static std::vector<uint8_t> decompressTokens(const std::vector<LZ77Token>& tokens,
        size_t originalSize);
};

#endif