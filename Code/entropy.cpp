#include "entropy.h"
#include <cmath>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

EntropyResult EntropyCalculator::calculate(const vector<uint8_t>& data, uint8_t Ms) {
    EntropyResult result;
    result.totalSymbols = 0;
    result.uniqueSymbols = 0;
    result.entropy = 0.0;

    if (data.empty() || Ms == 0) {
        return result;
    }

    map<vector<uint8_t>, uint64_t> frequencies;

    size_t numSymbols = data.size() / Ms;

    for (size_t i = 0; i < numSymbols; i++) {
        vector<uint8_t> symbol;
        for (uint8_t k = 0; k < Ms; k++) {
            symbol.push_back(data[i * Ms + k]);
        }
        frequencies[symbol]++;
        result.totalSymbols++;
    }

    if (data.size() % Ms != 0) {
        vector<uint8_t> lastSymbol;
        for (size_t i = numSymbols * Ms; i < data.size(); i++) {
            lastSymbol.push_back(data[i]);
        }
        while (lastSymbol.size() < Ms) {
            lastSymbol.push_back(0);
        }
        frequencies[lastSymbol]++;
        result.totalSymbols++;
    }

    result.frequencies = frequencies;
    result.uniqueSymbols = frequencies.size();

    result.entropy = calculateShannonEntropy(frequencies, result.totalSymbols);

    return result;
}

double EntropyCalculator::calculateShannonEntropy(const map<vector<uint8_t>, uint64_t>& frequencies,
    uint64_t totalSymbols) {
    if (totalSymbols == 0) return 0.0;

    double entropy = 0.0;

    for (const auto& pair : frequencies) {
        double probability = (double)pair.second / totalSymbols;
        entropy -= probability * log2(probability);
    }

    return entropy;
}