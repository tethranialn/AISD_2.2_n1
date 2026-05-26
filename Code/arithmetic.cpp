#include "arithmetic.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

ArithmeticResult Arithmetic::encode(const vector<uint8_t>& data) {
    ArithmeticResult result;

    if (data.empty()) {
        result.low = 0.0;
        result.high = 1.0;
        result.code = 0.0;
        result.dataLength = 0;
        return result;
    }

    map<uint8_t, size_t> freq;
    for (uint8_t byte : data) {
        freq[byte]++;
    }

    map<uint8_t, double> prob;
    map<uint8_t, double> cumProb;
    double cum = 0.0;
    for (auto& p : freq) {
        double pVal = (double)p.second / data.size();
        prob[p.first] = pVal;
        cumProb[p.first] = cum;
        cum += pVal;
    }
    result.probabilities = prob;

    double low = 0.0;
    double high = 1.0;

    for (uint8_t byte : data) {
        double range = high - low;
        double probLow = cumProb[byte];
        double probHigh = probLow + prob[byte];

        low = low + range * probLow;
        high = low + range * (probHigh - probLow);
    }

    result.low = low;
    result.high = high;
    result.code = (low + high) / 2.0;
    result.dataLength = data.size();

    return result;
}

vector<uint8_t> Arithmetic::decode(double code, size_t length, const map<uint8_t, double>& probabilities) {
    vector<uint8_t> output;

    if (length == 0 || probabilities.empty()) {
        return output;
    }

    map<uint8_t, double> cumProb;
    double cum = 0.0;
    for (auto& p : probabilities) {
        cumProb[p.first] = cum;
        cum += p.second;
    }

    double low = 0.0;
    double high = 1.0;

    for (size_t i = 0; i < length; i++) {
        double value = (code - low) / (high - low);

        uint8_t foundSymbol = 0;
        bool found = false;
        for (auto& p : probabilities) {
            if (value >= cumProb[p.first] && value < cumProb[p.first] + p.second) {
                foundSymbol = p.first;
                found = true;
                break;
            }
        }

        if (!found) {
            for (auto& p : probabilities) {
                if (fabs(value - cumProb[p.first]) < 1e-12) {
                    foundSymbol = p.first;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            break;
        }

        output.push_back(foundSymbol);

        double range = high - low;
        double probLow = cumProb[foundSymbol];
        double probHigh = probLow + probabilities.at(foundSymbol);

        low = low + range * probLow;
        high = low + range * (probHigh - probLow);
    }

    return output;
}

void Arithmetic::testPrecisionLimit() {
    cout << "\n========================================\n";
    cout << "ARITHMETIC CODING PRECISION TEST\n";
    cout << "========================================\n\n";

    cout << "Length,Low,High,Difference,DecodeMatch\n";

    for (int len = 1; len <= 50; len++) {
        vector<uint8_t> data;
        data.push_back('a');
        for (int i = 1; i < len; i++) {
            data.push_back('a' + (i % 2));
        }

        ArithmeticResult result = Arithmetic::encode(data);

        double diff = result.high - result.low;

        vector<uint8_t> decoded = Arithmetic::decode(result.code, result.dataLength, result.probabilities);

        bool decodeMatch = (decoded.size() == data.size());
        for (size_t i = 0; i < decoded.size() && decodeMatch; i++) {
            if (decoded[i] != data[i]) decodeMatch = false;
        }

        cout << len << ","
            << scientific << setprecision(10) << result.low << ","
            << scientific << setprecision(10) << result.high << ","
            << scientific << setprecision(10) << diff << ","
            << (decodeMatch ? "YES" : "NO") << "\n";

        if (diff < 1e-15 || result.low == result.high) {
            cout << "\n!!! BOUNDARIES COINCIDED AT LENGTH " << len << " !!!\n";
            break;
        }
    }
}