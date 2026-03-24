#ifndef BWT_H
#define BWT_H

#include <vector>
#include <cstdint>
#include <string>

struct BWTResult {
    std::vector<uint8_t> transformed;
    size_t primaryIndex;
    std::string description;
};

struct BWTBlockResult {
    std::vector<uint8_t> transformed;
    std::vector<size_t> primaryIndices;
    size_t blockSize;
    size_t originalSize;
};

class BWT {
public:
    static BWTResult encode(const std::vector<uint8_t>& input);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input, size_t primaryIndex);

    static std::vector<uint8_t> decodeWithCountingSort(const std::vector<uint8_t>& input, size_t primaryIndex);

    static BWTBlockResult encodeBlocked(const std::vector<uint8_t>& input, size_t blockSize);

    static std::vector<uint8_t> decodeBlocked(const BWTBlockResult& result);

    static bool encodeFileBlocked(const std::string& inputPath,
        const std::string& outputPath,
        size_t blockSize,
        std::string& errorMsg);

    static bool decodeFileBlocked(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool decodeFileWithCountingSort(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        std::string& errorMsg);

    static void testOnBanana();

    static void testBWTAndRLE(const std::string& inputDir, const std::vector<std::string>& files);

    // Новая функция: преобразование суффиксного массива в последний столбец матрицы BWT
    static std::vector<uint8_t> suffixArrayToLastColumn(
        const std::vector<uint8_t>& input,
        const std::vector<int32_t>& suffixArray);

private:
    static std::vector<std::vector<uint8_t>> buildRotationMatrix(const std::vector<uint8_t>& input);

    static std::vector<size_t> getSortedIndices(const std::vector<std::vector<uint8_t>>& matrix);

    static std::vector<uint8_t> getLastColumn(const std::vector<std::vector<uint8_t>>& matrix,
        const std::vector<size_t>& indices);

    static std::vector<uint8_t> reconstructUsingLFMapping(const std::vector<uint8_t>& lastColumn,
        size_t primaryIndex);

    static std::vector<uint8_t> reconstructUsingPermutation(const std::vector<uint8_t>& lastColumn,
        size_t primaryIndex);

    static void countingSortForIndices(const std::vector<uint8_t>& lastColumn,
        std::vector<size_t>& indices,
        std::vector<size_t>& nextIndices);
};

#endif