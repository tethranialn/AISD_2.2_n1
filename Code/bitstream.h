#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <vector>
#include <cstdint>
#include <string>

class BitWriter {
private:
    std::vector<uint8_t> buffer;
    uint8_t currentByte;
    int bitPos;

public:
    BitWriter();
    ~BitWriter();

    void writeBit(uint8_t bit);
    void writeBits(uint64_t value, int numBits);
    void flush();
    std::vector<uint8_t> getData() const;
    void clear();
    size_t size() const;
};

class BitReader {
private:
    const std::vector<uint8_t>& data;
    size_t bytePos;
    int bitPos;

public:
    BitReader(const std::vector<uint8_t>& input);

    uint8_t readBit();
    uint64_t readBits(int numBits);
    bool hasMore() const;
    size_t getPosition() const;
};

#endif