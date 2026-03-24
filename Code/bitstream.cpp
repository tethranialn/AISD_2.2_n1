#include "bitstream.h"
#include <iostream>

using namespace std;

BitWriter::BitWriter() : currentByte(0), bitPos(0) {}

BitWriter::~BitWriter() {
    flush();
}

void BitWriter::writeBit(uint8_t bit) {
    currentByte |= (bit & 1) << (7 - bitPos);
    bitPos++;

    if (bitPos == 8) {
        buffer.push_back(currentByte);
        currentByte = 0;
        bitPos = 0;
    }
}

void BitWriter::writeBits(uint64_t value, int numBits) {
    for (int i = numBits - 1; i >= 0; i--) {
        writeBit((value >> i) & 1);
    }
}

void BitWriter::flush() {
    if (bitPos > 0) {
        buffer.push_back(currentByte);
        currentByte = 0;
        bitPos = 0;
    }
}

vector<uint8_t> BitWriter::getData() const {
    return buffer;
}

void BitWriter::clear() {
    buffer.clear();
    currentByte = 0;
    bitPos = 0;
}

size_t BitWriter::size() const {
    return buffer.size();
}

BitReader::BitReader(const vector<uint8_t>& input)
    : data(input), bytePos(0), bitPos(0) {
}

uint8_t BitReader::readBit() {
    if (bytePos >= data.size()) return 0;

    uint8_t bit = (data[bytePos] >> (7 - bitPos)) & 1;
    bitPos++;

    if (bitPos == 8) {
        bytePos++;
        bitPos = 0;
    }

    return bit;
}

uint64_t BitReader::readBits(int numBits) {
    uint64_t value = 0;
    for (int i = 0; i < numBits; i++) {
        value = (value << 1) | readBit();
    }
    return value;
}

bool BitReader::hasMore() const {
    return bytePos < data.size();
}

size_t BitReader::getPosition() const {
    return bytePos * 8 + bitPos;
}