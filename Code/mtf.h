#ifndef MTF_H
#define MTF_H

#include <vector>
#include <cstdint>
#include <string>

class MTF {
public:
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input);

    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);

    static bool encodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool decodeFile(const std::string& inputPath,
        const std::string& outputPath,
        std::string& errorMsg);

    static bool verifyCycle(const std::string& inputPath,
        const std::string& encodedPath,
        const std::string& decodedPath,
        std::string& errorMsg);
};

#endif
