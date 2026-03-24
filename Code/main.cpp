#include "raw_convert.h"
#include "rle.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace std;

namespace fs = std::filesystem;

int main() {
    const string BASE_PATH = "C:\\Users\\tetramania\\Documents\\.Study\\AISD_2.2_n1";
    const string INPUT_DIR = BASE_PATH + "\\Input files";
    const string ENCODED_DIR = BASE_PATH + "\\Encoded files";
    const string DECODED_DIR = BASE_PATH + "\\Decoded files";

    fs::create_directories(INPUT_DIR);
    fs::create_directories(ENCODED_DIR);
    fs::create_directories(DECODED_DIR);

    cout << "========================================\n";
    cout << "RLE Compression Analysis Tool\n";
    cout << "========================================\n\n";

    cout << "Converting images to RAW format...\n";
    cout << "----------------------------------------\n";
    if (!convertImagesToRaw(INPUT_DIR, INPUT_DIR)) {
        cout << "Error: Failed to convert images\n";
        return 1;
    }
    cout << "Images converted successfully!\n\n";

    struct TestConfig {
        string filename;
        uint8_t Ms;
        uint8_t Mc;
        string description;
        bool isRaw;
    };

    vector<TestConfig> tests = {
        {"BW.raw", 1, 1, "Black & White", true},
        {"BW.raw", 1, 2, "Black & White", true},
        {"Gray.raw", 1, 1, "Grayscale", true},
        {"Gray.raw", 1, 2, "Grayscale", true},
        {"RGB.raw", 3, 1, "Color RGB", true},
        {"RGB.raw", 3, 2, "Color RGB", true},
        {"enwik7.txt", 1, 1, "Text (English)", false},
        {"RusText.txt", 1, 1, "Text (Russian)", false},
        {"BIN.bin", 1, 1, "Binary", false}
    };

    for (const auto& test : tests) {
        string inputPath = INPUT_DIR + "\\" + test.filename;

        if (!fs::exists(inputPath)) {
            cout << "File not found: " << test.filename << " - skipping\n\n";
            continue;
        }

        cout << "File: " << test.filename << " (" << test.description << ")\n";
        cout << "Parameters: Ms=" << (int)test.Ms << ", Mc=" << (int)test.Mc << "\n";
        cout << "----------------------------------------\n";

        string encodedPath = ENCODED_DIR + "\\" + test.filename + ".rle";
        string decodedPath = DECODED_DIR + "\\" + test.filename;

        string errorMsg;

        if (test.isRaw) {
            uint8_t type = 0;
            uint32_t pixels = 0;
            vector<uint8_t> pixelData;

            if (!extractPixelData(inputPath, pixelData, type, pixels)) {
                cout << "Error: Cannot extract pixel data\n\n";
                continue;
            }

            vector<uint8_t> encodedPixelData = RLE::encode(pixelData, test.Ms, test.Mc);

            RLEHeader header;
            header.Ms = test.Ms;
            header.Mc = test.Mc;
            header.originalSize = pixelData.size();
            header.encodedSize = encodedPixelData.size();

            size_t encodedTotalSize = sizeof(header) + encodedPixelData.size();

            ofstream outFile(encodedPath, ios::binary);
            if (outFile.is_open()) {
                outFile.write(reinterpret_cast<const char*>(&header.Ms), sizeof(header.Ms));
                outFile.write(reinterpret_cast<const char*>(&header.Mc), sizeof(header.Mc));
                outFile.write(reinterpret_cast<const char*>(&header.originalSize), sizeof(header.originalSize));
                outFile.write(reinterpret_cast<const char*>(&header.encodedSize), sizeof(header.encodedSize));
                outFile.write(reinterpret_cast<const char*>(encodedPixelData.data()), encodedPixelData.size());
                outFile.close();
            }

            vector<uint8_t> decodedPixelData = RLE::decode(encodedPixelData, test.Ms, test.Mc);

            ofstream decFile(decodedPath, ios::binary);
            if (decFile.is_open()) {
                decFile.put(type);
                decFile.write((char*)&pixels, 4);
                decFile.write((char*)decodedPixelData.data(), decodedPixelData.size());
                decFile.close();
            }

            if (pixelData.size() == decodedPixelData.size() &&
                memcmp(pixelData.data(), decodedPixelData.data(), pixelData.size()) == 0) {
                double ratio = (double)encodedTotalSize / pixelData.size() * 100.0;
                double factor = (double)pixelData.size() / encodedTotalSize;
                cout << "  Pixel data: " << pixelData.size() << " bytes\n";
                cout << "  Encoded:    " << encodedTotalSize << " bytes\n";
                cout << "  Ratio:      " << fixed << setprecision(2) << ratio << "% of pixel data\n";
                cout << "  Factor:     " << fixed << setprecision(2) << factor << ":1\n";
                cout << "  Status:     PASSED\n\n";
            }
            else {
                cout << "  Status:     FAILED\n\n";
            }
        }
        else {
            CompressionStats actual = RLE::compressFile(inputPath, encodedPath, test.Ms, test.Mc, errorMsg);
            if (!errorMsg.empty()) {
                cout << "Error: " << errorMsg << "\n\n";
                continue;
            }

            bool verified = RLE::verifyCycle(inputPath, encodedPath, decodedPath, test.Ms, test.Mc, errorMsg);

            cout << "  Original: " << actual.originalSize << " bytes\n";
            cout << "  Encoded:  " << actual.encodedSize << " bytes\n";
            cout << "  Ratio:    " << fixed << setprecision(2) << actual.compressionRatio << "% of original\n";
            cout << "  Factor:   " << fixed << setprecision(2) << actual.compressionFactor << ":1\n";
            cout << "  Status:   " << (verified ? "PASSED" : "FAILED") << "\n\n";
        }
    }

    return 0;
}