#include "raw_convert.h"
#include "rle.h"
#include "entropy.h"
#include "mtf.h"
#include "huffman.h"
#include "huffman_canonical.h"
#include "bwt.h"
#include "lz77.h"
#include "lzss.h"
#include "lz78.h"
#include "lzw.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <cstring>
#include <limits>
#include <sys/stat.h>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace chrono;

namespace fs = std::filesystem;

bool fileExists(const string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void printMenu() {
    cout << "\n========================================\n";
    cout << "Compression & Analysis Tool\n";
    cout << "========================================\n";
    cout << "1. RLE Compression Analysis\n";
    cout << "2. Entropy Calculation\n";
    cout << "3. MTF Transform Analysis\n";
    cout << "4. Huffman Compression Analysis\n";
    cout << "5. Canonical Huffman Compression\n";
    cout << "6. BWT Transform Test (banana)\n";
    cout << "7. BWT + RLE Combined Compression\n";
    cout << "8. Suffix Array to BWT Last Column Demo\n";
    cout << "9. Efficient BWT (Suffix Array Based)\n";
    cout << "10. LZ77 Compression Analysis\n";
    cout << "11. LZSS Compression Analysis\n";
    cout << "12. LZ78 Compression Analysis\n";
    cout << "13. LZW Compression Analysis\n";
    cout << "14. Exit\n";
    cout << "========================================\n";
    cout << "Choose algorithm (1-14): ";
}

void printFileSelectionMenu() {
    cout << "\n----------------------------------------\n";
    cout << "Select files to process:\n";
    cout << "1. BW.raw (Black & White image)\n";
    cout << "2. Gray.raw (Grayscale image)\n";
    cout << "3. RGB.raw (Color RGB image)\n";
    cout << "4. enwik7.txt (English text)\n";
    cout << "5. RusText.txt (Russian text)\n";
    cout << "6. BIN.bin (Binary data)\n";
    cout << "7. All files\n";
    cout << "8. Cancel\n";
    cout << "Enter your choice (1-8): ";
}

vector<string> getSelectedFiles(const string& inputDir, int choice) {
    vector<string> allFiles = { "BW.raw", "Gray.raw", "RGB.raw", "enwik7.txt", "RusText.txt", "BIN.bin" };
    vector<string> selectedFiles;

    if (choice == 7) {
        return allFiles;
    }

    if (choice >= 1 && choice <= 6) {
        selectedFiles.push_back(allFiles[choice - 1]);
    }

    return selectedFiles;
}

void rleAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "RLE Compression Analysis\n";
    cout << "========================================\n\n";

    printFileSelectionMenu();
    int choice;
    cin >> choice;

    if (choice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, choice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    struct TestConfig {
        string filename;
        uint8_t Ms;
        uint8_t Mc;
        string description;
        bool isRaw;
    };

    vector<TestConfig> allTests = {
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

    for (const auto& filename : files) {
        for (const auto& test : allTests) {
            if (test.filename != filename) continue;

            string inputPath = inputDir + "\\" + test.filename;

            if (!fileExists(inputPath)) {
                cout << "File not found: " << test.filename << " - skipping\n\n";
                continue;
            }

            cout << "File: " << test.filename << " (" << test.description << ")\n";
            cout << "Parameters: Ms=" << (int)test.Ms << ", Mc=" << (int)test.Mc << "\n";
            cout << "----------------------------------------\n";

            string encodedPath = encodedDir + "\\" + test.filename + ".rle";
            string decodedPath = decodedDir + "\\" + test.filename;

            string errorMsg;

            if (test.isRaw) {
                uint8_t type = 0;
                uint32_t pixels = 0;
                vector<uint8_t> pixelData;

                ifstream rawFile(inputPath, ios::binary);
                if (rawFile.is_open()) {
                    rawFile.read((char*)&type, 1);
                    rawFile.read((char*)&pixels, 4);
                    pixelData.assign((istreambuf_iterator<char>(rawFile)), istreambuf_iterator<char>());
                    rawFile.close();
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
    }
}

void entropyAnalysis(const string& inputDir) {
    cout << "\n========================================\n";
    cout << "Entropy Calculation\n";
    cout << "========================================\n\n";

    printFileSelectionMenu();
    int choice;
    cin >> choice;

    if (choice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, choice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string filePath = inputDir + "\\" + filename;

        if (!fileExists(filePath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "File: " << filename << "\n";
        cout << "----------------------------------------\n";

        ifstream file(filePath, ios::binary);
        if (!file.is_open()) {
            cout << "Error: Cannot open file\n\n";
            continue;
        }

        vector<uint8_t> data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        if (data.empty()) {
            cout << "File is empty\n\n";
            continue;
        }

        cout << "File size: " << data.size() << " bytes\n\n";
        cout << "Entropy depending on symbol code length (Ms):\n";
        cout << "Ms (bytes) | Entropy H(X) (bits/symbol)\n";
        cout << "----------------------------------------\n";

        for (uint8_t Ms = 1; Ms <= 4; Ms++) {
            EntropyResult result = EntropyCalculator::calculate(data, Ms);

            cout << "   " << setw(2) << (int)Ms << "        |   "
                << fixed << setprecision(4) << result.entropy << "\n";
        }

        cout << "\n";
    }
}

void mtfAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "MTF Transform Analysis\n";
    cout << "========================================\n\n";

    printFileSelectionMenu();
    int choice;
    cin >> choice;

    if (choice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, choice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "File: " << filename << "\n";
        cout << "----------------------------------------\n";

        ifstream inFile(inputPath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Cannot open file\n\n";
            continue;
        }

        vector<uint8_t> originalData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
        inFile.close();

        cout << "Original size: " << originalData.size() << " bytes\n";

        vector<uint8_t> encodedData = MTF::encode(originalData);
        cout << "MTF encoded size: " << encodedData.size() << " bytes\n";

        vector<uint8_t> decodedData = MTF::decode(encodedData);

        if (originalData.size() == decodedData.size() &&
            memcmp(originalData.data(), decodedData.data(), originalData.size()) == 0) {
            cout << "Verification: PASSED\n";
        }
        else {
            cout << "Verification: FAILED\n";
        }

        cout << "\n";
    }
}

void huffmanAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "Huffman Compression Analysis\n";
    cout << "========================================\n\n";

    printFileSelectionMenu();
    int choice;
    cin >> choice;

    if (choice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, choice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        string errorMsg;
        CompressionStatsHuffman stats = Huffman::analyzeFile(inputPath, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error analyzing " << filename << ": " << errorMsg << "\n\n";
            continue;
        }

        Huffman::printStats(stats);

        string encodedPath = encodedDir + "\\" + filename + ".huf";
        string decodedPath = decodedDir + "\\" + filename;

        if (Huffman::encodeFileWithStats(inputPath, encodedPath, stats, errorMsg)) {
            cout << "Encoding successful\n";

            if (Huffman::decodeFileWithVerify(encodedPath, decodedPath, errorMsg)) {
                cout << "Decoding and verification: PASSED\n";
            }
            else {
                cout << "Decoding verification: FAILED - " << errorMsg << "\n";
            }
        }
        else {
            cout << "Encoding failed: " << errorMsg << "\n";
        }

        cout << "\n";
    }
}

void huffmanCanonicalAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "Canonical Huffman Compression\n";
    cout << "========================================\n\n";

    printFileSelectionMenu();
    int fileChoice;
    cin >> fileChoice;

    if (fileChoice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, fileChoice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "\nFile: " << filename << "\n";
        cout << "----------------------------------------\n";

        string errorMsg;

        string encodedPath = encodedDir + "\\" + filename + ".huf_canonical";
        string decodedPath = decodedDir + "\\" + filename;

        ifstream inFile(inputPath, ios::binary);
        vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
        inFile.close();

        cout << "Original size: " << inputData.size() << " bytes\n";

        auto start = chrono::high_resolution_clock::now();
        bool success = HuffmanCanonical::encodeFile(inputPath, encodedPath, errorMsg);
        auto end = chrono::high_resolution_clock::now();
        auto encodeTime = chrono::duration_cast<chrono::milliseconds>(end - start);

        if (!success) {
            cout << "Encoding failed: " << errorMsg << "\n\n";
            continue;
        }

        ifstream encodedFile(encodedPath, ios::binary);
        vector<uint8_t> encodedData((istreambuf_iterator<char>(encodedFile)), istreambuf_iterator<char>());
        encodedFile.close();

        cout << "Compressed size: " << encodedData.size() << " bytes\n";
        cout << "Compression ratio: " << fixed << setprecision(2)
            << (double)encodedData.size() / inputData.size() * 100.0 << "%\n";
        cout << "Encoding time: " << encodeTime.count() << " ms\n";

        start = chrono::high_resolution_clock::now();
        bool verified = HuffmanCanonical::verifyCycle(inputPath, encodedPath, decodedPath, errorMsg);
        end = chrono::high_resolution_clock::now();
        auto decodeTime = chrono::duration_cast<chrono::milliseconds>(end - start);

        cout << "Decoding time: " << decodeTime.count() << " ms\n";
        cout << "Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void suffixArrayToBWTDemo(const string& inputDir) {
    cout << "\n========================================\n";
    cout << "Suffix Array to BWT Last Column Demo\n";
    cout << "========================================\n\n";

    printFileSelectionMenu();
    int choice;
    cin >> choice;

    if (choice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, choice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string filePath = inputDir + "\\" + filename;

        if (!fileExists(filePath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "\nFile: " << filename << "\n";
        cout << "----------------------------------------\n";

        ifstream inFile(filePath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Cannot open file\n\n";
            continue;
        }

        vector<uint8_t> fileData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
        inFile.close();

        cout << "Size: " << fileData.size() << " bytes\n";

        size_t demoSize = min(fileData.size(), size_t(50));
        vector<uint8_t> demoData(fileData.begin(), fileData.begin() + demoSize);

        if (demoSize < fileData.size()) {
            cout << "Using first " << demoSize << " bytes\n";
        }

        cout << "\nInput data (first " << demoSize << " bytes):\n";
        for (size_t i = 0; i < demoSize; i++) {
            if (isprint(demoData[i])) {
                cout << (char)demoData[i];
            }
            else {
                cout << ".";
            }
        }
        cout << "\n\n";

        vector<int32_t> suffixArray = BWT::buildSuffixArrayDoubling(demoData);

        cout << "Suffix array (indices of cyclic rotations in sorted order):\n";
        for (size_t i = 0; i < suffixArray.size(); i++) {
            cout << "  [" << setw(2) << i << "] = " << suffixArray[i];
            cout << "  -> rotation: ";
            for (size_t j = 0; j < min(demoSize, size_t(20)); j++) {
                size_t pos = (suffixArray[i] + j) % demoSize;
                if (isprint(demoData[pos])) {
                    cout << (char)demoData[pos];
                }
                else {
                    cout << ".";
                }
            }
            if (demoSize > 20) cout << "...";
            cout << "\n";
        }

        vector<uint8_t> lastColumn = BWT::suffixArrayToLastColumn(demoData, suffixArray);

        cout << "\nBWT last column:\n";
        for (size_t i = 0; i < lastColumn.size(); i++) {
            if (isprint(lastColumn[i])) {
                cout << (char)lastColumn[i];
            }
            else {
                cout << "0x" << hex << setw(2) << setfill('0') << (int)lastColumn[i] << dec;
            }
        }
        cout << "\n";

        cout << "\nLast column bytes:\n";
        for (size_t i = 0; i < lastColumn.size(); i++) {
            cout << "0x" << hex << setw(2) << setfill('0') << (int)lastColumn[i] << " ";
            if ((i + 1) % 16 == 0) cout << "\n";
        }
        cout << dec << "\n\n";
    }
}

void lz77Analysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZ77 Compression Analysis\n";
    cout << "========================================\n\n";

    uint32_t windowSize = 4096;
    uint32_t lookaheadSize = 16;

    printFileSelectionMenu();
    int fileChoice;
    cin >> fileChoice;

    if (fileChoice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, fileChoice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "\nFile: " << filename << "\n";
        cout << "----------------------------------------\n";

        string errorMsg;

        string encodedPath = encodedDir + "\\" + filename + ".lz77";
        string decodedPath = decodedDir + "\\" + filename;

        LZ77Stats stats = LZ77::compressFile(inputPath, encodedPath,
            windowSize, lookaheadSize, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZ77::printStats(stats);

        bool verified = LZ77::verifyCycle(inputPath, encodedPath, decodedPath,
            windowSize, lookaheadSize, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void lzssAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZSS Compression Analysis\n";
    cout << "========================================\n\n";

    uint32_t windowSize = 4096;
    uint32_t lookaheadSize = 16;

    printFileSelectionMenu();
    int fileChoice;
    cin >> fileChoice;

    if (fileChoice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, fileChoice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "\nFile: " << filename << "\n";
        cout << "----------------------------------------\n";

        string errorMsg;

        string encodedPath = encodedDir + "\\" + filename + ".lzss";
        string decodedPath = decodedDir + "\\" + filename;

        LZSSStats stats = LZSS::compressFile(inputPath, encodedPath,
            windowSize, lookaheadSize, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZSS::printStats(stats);

        bool verified = LZSS::verifyCycle(inputPath, encodedPath, decodedPath,
            windowSize, lookaheadSize, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void lz78Analysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZ78 Compression Analysis\n";
    cout << "========================================\n\n";

    uint32_t maxDictSize = 4096;

    printFileSelectionMenu();
    int fileChoice;
    cin >> fileChoice;

    if (fileChoice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, fileChoice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "\nFile: " << filename << "\n";
        cout << "----------------------------------------\n";

        string errorMsg;

        string encodedPath = encodedDir + "\\" + filename + ".lz78";
        string decodedPath = decodedDir + "\\" + filename;

        LZ78Stats stats = LZ78::compressFile(inputPath, encodedPath,
            maxDictSize, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZ78::printStats(stats);

        bool verified = LZ78::verifyCycle(inputPath, encodedPath, decodedPath,
            maxDictSize, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void lzwAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZW Compression Analysis\n";
    cout << "========================================\n\n";

    uint32_t maxDictSize = 4096;

    printFileSelectionMenu();
    int fileChoice;
    cin >> fileChoice;

    if (fileChoice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> files = getSelectedFiles(inputDir, fileChoice);

    if (files.empty()) {
        cout << "Invalid choice\n";
        return;
    }

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;

        if (!fileExists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n\n";
            continue;
        }

        cout << "\nFile: " << filename << "\n";
        cout << "----------------------------------------\n";

        string errorMsg;

        string encodedPath = encodedDir + "\\" + filename + ".lzw";
        string decodedPath = decodedDir + "\\" + filename;

        LZWStats stats = LZW::compressFile(inputPath, encodedPath,
            maxDictSize, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZW::printStats(stats);

        bool verified = LZW::verifyCycle(inputPath, encodedPath, decodedPath,
            maxDictSize, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

int main() {
    const string BASE_PATH = "C:\\Users\\chivo\\Documents\\AISD_2.2_n1";
    const string INPUT_DIR = BASE_PATH + "\\Input files";
    const string ENCODED_DIR = BASE_PATH + "\\Encoded files";
    const string DECODED_DIR = BASE_PATH + "\\Decoded files";

    fs::create_directories(INPUT_DIR);
    fs::create_directories(ENCODED_DIR);
    fs::create_directories(DECODED_DIR);

    cout << "Converting images to RAW format...\n";
    cout << "----------------------------------------\n";
    if (!convertImagesToRaw(INPUT_DIR, INPUT_DIR)) {
        cout << "Error: Failed to convert images\n";
        return 1;
    }
    cout << "Images converted successfully!\n";

    int choice = 0;
    while (true) {
        printMenu();
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        switch (choice) {
        case 1:
            rleAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 2:
            entropyAnalysis(INPUT_DIR);
            break;
        case 3:
            mtfAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 4:
            huffmanAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 5:
            huffmanCanonicalAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 6:
            BWT::testOnBanana();
            break;
        case 7: {
            cout << "\n";
            cout << "========================================\n";
            cout << "BWT + RLE Combined Compression\n";
            cout << "========================================\n\n";
            printFileSelectionMenu();
            int fileChoice;
            cin >> fileChoice;
            if (fileChoice == 8) {
                cout << "Operation cancelled\n";
                break;
            }
            vector<string> files = getSelectedFiles(INPUT_DIR, fileChoice);
            if (files.empty()) {
                cout << "Invalid choice\n";
                break;
            }
            BWT::testBWTAndRLE(INPUT_DIR, files);
            break;
        }
        case 8:
            suffixArrayToBWTDemo(INPUT_DIR);
            break;
        case 9:
            BWT::testEfficientBWT(INPUT_DIR, getSelectedFiles(INPUT_DIR, 7));
            break;
        case 10:
            lz77Analysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 11:
            lzssAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 12:
            lz78Analysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 13:
            lzwAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
            break;
        case 14:
            cout << "Exiting program...\n";
            return 0;
        default:
            cout << "Invalid choice. Please select 1-14.\n";
            break;
        }
    }

    return 0;
}