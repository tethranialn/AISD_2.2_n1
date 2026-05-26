#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <vector>
#include <cstdint>
#include <string>
#include <map>

struct ArithmeticResult {
    double code;
    double low;
    double high;
    size_t dataLength;
    std::map<uint8_t, double> probabilities;
};

class Arithmetic {
public:
    static ArithmeticResult encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decode(double code, size_t length, const std::map<uint8_t, double>& probabilities);
    static void testPrecisionLimit();
};

#endif