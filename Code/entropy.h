#ifndef ENTROPY_H
#define ENTROPY_H

#include <vector>
#include <cstdint>
#include <string>
#include <map>

struct EntropyResult {
    double entropy;
    uint64_t totalSymbols;
    uint64_t uniqueSymbols;
    std::map<std::vector<uint8_t>, uint64_t> frequencies;
};

class EntropyCalculator {
public:
    static EntropyResult calculate(const std::vector<uint8_t>& data, uint8_t Ms);

    static double calculateShannonEntropy(const std::map<std::vector<uint8_t>, uint64_t>& frequencies,
        uint64_t totalSymbols);
};

#endif