#include "analysis.h"
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
#include <iomanip>
#include <cstring>
#include <sys/stat.h>
#include <chrono>
#include <functional>
#include <map>

using namespace std;
using namespace chrono;

bool Analysis::fileExists(const string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void Analysis::printFileSelectionMenu() {
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

vector<string> Analysis::getSelectedFiles(const string& inputDir, int choice) {
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

void Analysis::rleAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
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

void Analysis::entropyAnalysis(const string& inputDir) {
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

void Analysis::mtfAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
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

        string encodedPath = encodedDir + "\\" + filename + ".mtf";
        ofstream encFile(encodedPath, ios::binary);
        encFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
        encFile.close();

        cout << "MTF encoded size: " << encodedData.size() << " bytes\n";

        vector<uint8_t> decodedData = MTF::decode(encodedData);

        string decodedPath = decodedDir + "\\" + filename;
        ofstream decFile(decodedPath, ios::binary);
        decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
        decFile.close();

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

void Analysis::huffmanAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
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

void Analysis::huffmanCanonicalAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
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

        auto start = high_resolution_clock::now();
        bool success = HuffmanCanonical::encodeFile(inputPath, encodedPath, errorMsg);
        auto end = high_resolution_clock::now();
        auto encodeTime = duration_cast<milliseconds>(end - start).count();

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
        cout << "Encoding time: " << encodeTime << " ms\n";

        start = high_resolution_clock::now();
        bool verified = HuffmanCanonical::verifyCycle(inputPath, encodedPath, decodedPath, errorMsg);
        end = high_resolution_clock::now();
        auto decodeTime = duration_cast<milliseconds>(end - start).count();

        cout << "Decoding time: " << decodeTime << " ms\n";
        cout << "Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void Analysis::suffixArrayToBWTDemo(const string& inputDir) {
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

void Analysis::testBWTAndRLE(const string& inputDir) {
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
    BWT::testBWTAndRLE(inputDir, files);
}

void Analysis::testEfficientBWT(const string& inputDir) {
    BWT::testEfficientBWT(inputDir, getSelectedFiles(inputDir, 7));
}

void Analysis::testOnBanana() {
    BWT::testOnBanana();
}

void Analysis::lz77Analysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZ77 Compression Analysis\n";
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

        string encodedPath = encodedDir + "\\" + filename + ".lz77";
        string decodedPath = decodedDir + "\\" + filename;

        LZ77Stats stats = LZ77::compressFile(inputPath, encodedPath, 4096, 16, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZ77::printStats(stats);

        bool verified = LZ77::verifyCycle(inputPath, encodedPath, decodedPath, 4096, 16, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void Analysis::lzssAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZSS Compression Analysis\n";
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

        string encodedPath = encodedDir + "\\" + filename + ".lzss";
        string decodedPath = decodedDir + "\\" + filename;

        LZSSStats stats = LZSS::compressFile(inputPath, encodedPath, 4096, 16, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZSS::printStats(stats);

        bool verified = LZSS::verifyCycle(inputPath, encodedPath, decodedPath, 4096, 16, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void Analysis::lz78Analysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZ78 Compression Analysis\n";
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

        string encodedPath = encodedDir + "\\" + filename + ".lz78";
        string decodedPath = decodedDir + "\\" + filename;

        LZ78Stats stats = LZ78::compressFile(inputPath, encodedPath, 4096, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZ78::printStats(stats);

        bool verified = LZ78::verifyCycle(inputPath, encodedPath, decodedPath, 4096, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void Analysis::lzwAnalysis(const string& inputDir, const string& encodedDir, const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "LZW Compression Analysis\n";
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

        string encodedPath = encodedDir + "\\" + filename + ".lzw";
        string decodedPath = decodedDir + "\\" + filename;

        LZWStats stats = LZW::compressFile(inputPath, encodedPath, 4096, errorMsg);

        if (!errorMsg.empty()) {
            cout << "Error: " << errorMsg << "\n\n";
            continue;
        }

        LZW::printStats(stats);

        bool verified = LZW::verifyCycle(inputPath, encodedPath, decodedPath, 4096, errorMsg);

        cout << "  Status: " << (verified ? "PASSED" : "FAILED") << "\n\n";
    }
}

void Analysis::runSingleAnalysis(const string& algorithm,
    const string& inputDir,
    const string& encodedDir,
    const string& decodedDir,
    vector<AlgorithmResult>& results) {
    vector<string> files = { "BW.raw", "Gray.raw", "RGB.raw", "enwik7.txt", "RusText.txt", "BIN.bin" };

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;
        if (!fileExists(inputPath)) {
            AlgorithmResult res;
            res.algorithm = algorithm;
            res.filename = filename;
            res.success = false;
            res.details = "File not found";
            results.push_back(res);
            continue;
        }

        AlgorithmResult res;
        res.algorithm = algorithm;
        res.filename = filename;

        auto start = high_resolution_clock::now();

        try {
            if (algorithm == "RLE") {
                string encodedPath = encodedDir + "\\" + filename + ".rle";
                string decodedPath = decodedDir + "\\" + filename;
                string errorMsg;

                CompressionStats stats = RLE::compressFile(inputPath, encodedPath, 1, 2, errorMsg);
                if (errorMsg.empty()) {
                    bool verified = RLE::verifyCycle(inputPath, encodedPath, decodedPath, 1, 2, errorMsg);
                    res.success = verified;
                    res.ratio = stats.compressionRatio;
                    res.originalSize = stats.originalSize;
                    res.processedSize = stats.encodedSize;
                    res.details = verified ? "PASSED" : "FAILED";
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "MTF") {
                ifstream inFile(inputPath, ios::binary);
                vector<uint8_t> originalData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
                inFile.close();

                vector<uint8_t> encodedData = MTF::encode(originalData);

                string encodedPath = encodedDir + "\\" + filename + ".mtf";
                ofstream encFile(encodedPath, ios::binary);
                encFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
                encFile.close();

                vector<uint8_t> decodedData = MTF::decode(encodedData);

                string decodedPath = decodedDir + "\\" + filename;
                ofstream decFile(decodedPath, ios::binary);
                decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
                decFile.close();

                res.success = (originalData.size() == decodedData.size() &&
                    memcmp(originalData.data(), decodedData.data(), originalData.size()) == 0);
                res.originalSize = originalData.size();
                res.processedSize = encodedData.size();
                res.ratio = (double)encodedData.size() / originalData.size() * 100.0;
                res.details = res.success ? "PASSED" : "FAILED";
            }
            else if (algorithm == "Huffman") {
                string errorMsg;
                CompressionStatsHuffman stats = Huffman::analyzeFile(inputPath, errorMsg);
                if (errorMsg.empty()) {
                    string encodedPath = encodedDir + "\\" + filename + ".huf";
                    string decodedPath = decodedDir + "\\" + filename;

                    if (Huffman::encodeFileWithStats(inputPath, encodedPath, stats, errorMsg)) {
                        bool verified = Huffman::decodeFileWithVerify(encodedPath, decodedPath, errorMsg);
                        res.success = verified;
                        res.ratio = stats.compressionRatio;
                        res.originalSize = stats.originalSize;
                        res.processedSize = stats.encodedSize;
                        res.details = verified ? "PASSED" : "FAILED";
                    }
                    else {
                        res.success = false;
                        res.details = errorMsg;
                    }
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "Canonical Huffman") {
                string encodedPath = encodedDir + "\\" + filename + ".huf_canonical";
                string decodedPath = decodedDir + "\\" + filename;
                string errorMsg;

                ifstream inFile(inputPath, ios::binary);
                vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
                inFile.close();
                res.originalSize = inputData.size();

                bool success = HuffmanCanonical::encodeFile(inputPath, encodedPath, errorMsg);

                if (success) {
                    ifstream encodedFile(encodedPath, ios::binary);
                    vector<uint8_t> encodedData((istreambuf_iterator<char>(encodedFile)), istreambuf_iterator<char>());
                    encodedFile.close();
                    res.processedSize = encodedData.size();
                    res.ratio = (double)encodedData.size() / res.originalSize * 100.0;

                    bool verified = HuffmanCanonical::verifyCycle(inputPath, encodedPath, decodedPath, errorMsg);
                    res.success = verified;
                    res.details = verified ? "PASSED" : "FAILED";
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "LZ77") {
                string encodedPath = encodedDir + "\\" + filename + ".lz77";
                string decodedPath = decodedDir + "\\" + filename;
                string errorMsg;

                LZ77Stats stats = LZ77::compressFile(inputPath, encodedPath, 4096, 16, errorMsg);
                if (errorMsg.empty()) {
                    bool verified = LZ77::verifyCycle(inputPath, encodedPath, decodedPath, 4096, 16, errorMsg);
                    res.success = verified;
                    res.ratio = stats.compressionRatio;
                    res.originalSize = stats.originalSize;
                    res.processedSize = stats.compressedSize;
                    res.details = verified ? "PASSED" : "FAILED";
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "LZSS") {
                string encodedPath = encodedDir + "\\" + filename + ".lzss";
                string decodedPath = decodedDir + "\\" + filename;
                string errorMsg;

                LZSSStats stats = LZSS::compressFile(inputPath, encodedPath, 4096, 16, errorMsg);
                if (errorMsg.empty()) {
                    bool verified = LZSS::verifyCycle(inputPath, encodedPath, decodedPath, 4096, 16, errorMsg);
                    res.success = verified;
                    res.ratio = stats.compressionRatio;
                    res.originalSize = stats.originalSize;
                    res.processedSize = stats.compressedSize;
                    res.details = verified ? "PASSED" : "FAILED";
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "LZ78") {
                string encodedPath = encodedDir + "\\" + filename + ".lz78";
                string decodedPath = decodedDir + "\\" + filename;
                string errorMsg;

                LZ78Stats stats = LZ78::compressFile(inputPath, encodedPath, 4096, errorMsg);
                if (errorMsg.empty()) {
                    bool verified = LZ78::verifyCycle(inputPath, encodedPath, decodedPath, 4096, errorMsg);
                    res.success = verified;
                    res.ratio = stats.compressionRatio;
                    res.originalSize = stats.originalSize;
                    res.processedSize = stats.compressedSize;
                    res.details = verified ? "PASSED" : "FAILED";
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "LZW") {
                string encodedPath = encodedDir + "\\" + filename + ".lzw";
                string decodedPath = decodedDir + "\\" + filename;
                string errorMsg;

                LZWStats stats = LZW::compressFile(inputPath, encodedPath, 4096, errorMsg);
                if (errorMsg.empty()) {
                    bool verified = LZW::verifyCycle(inputPath, encodedPath, decodedPath, 4096, errorMsg);
                    res.success = verified;
                    res.ratio = stats.compressionRatio;
                    res.originalSize = stats.originalSize;
                    res.processedSize = stats.compressedSize;
                    res.details = verified ? "PASSED" : "FAILED";
                }
                else {
                    res.success = false;
                    res.details = errorMsg;
                }
            }
            else if (algorithm == "BWT+RLE") {
                ifstream inFile(inputPath, ios::binary);
                vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
                inFile.close();
                res.originalSize = inputData.size();

                BWTResult bwtResult = BWT::encodeEfficient(inputData);
                vector<uint8_t> rleEncoded = RLE::encode(bwtResult.transformed, 1, 2);

                string bwtPath = encodedDir + "\\" + filename + "_bwt.bin";
                string rlePath = encodedDir + "\\" + filename + "_bwt.rle";
                string decodedPath = decodedDir + "\\" + filename;

                ofstream bwtFile(bwtPath, ios::binary);
                uint64_t primaryIdx = bwtResult.primaryIndex;
                uint64_t dataSize = bwtResult.transformed.size();
                bwtFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
                bwtFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
                bwtFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
                bwtFile.close();

                ofstream rleFile(rlePath, ios::binary);
                rleFile.write(reinterpret_cast<const char*>(rleEncoded.data()), rleEncoded.size());
                rleFile.close();

                res.processedSize = rleEncoded.size();
                res.ratio = (double)res.processedSize / res.originalSize * 100.0;

                string errorMsg;
                string tempDecode = encodedDir + "\\" + filename + "_temp.bin";
                RLE::decodeFile(rlePath, tempDecode, errorMsg);

                ifstream tempFile(tempDecode, ios::binary);
                uint64_t dataSize2;
                uint64_t primaryIdx2;
                tempFile.read(reinterpret_cast<char*>(&dataSize2), sizeof(dataSize2));
                tempFile.read(reinterpret_cast<char*>(&primaryIdx2), sizeof(primaryIdx2));
                vector<uint8_t> bwtData(dataSize2);
                tempFile.read(reinterpret_cast<char*>(bwtData.data()), dataSize2);
                tempFile.close();

                vector<uint8_t> decodedData = BWT::decode(bwtData, primaryIdx2);

                ofstream decFile(decodedPath, ios::binary);
                decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
                decFile.close();

                res.success = (decodedData.size() == inputData.size() &&
                    memcmp(decodedData.data(), inputData.data(), inputData.size()) == 0);
                res.details = res.success ? "PASSED" : "FAILED";
            }
            else if (algorithm == "BWT+MTF+HA") {
                ifstream inFile(inputPath, ios::binary);
                vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
                inFile.close();
                res.originalSize = inputData.size();

                BWTResult bwtResult = BWT::encodeEfficient(inputData);
                vector<uint8_t> mtfEncoded = MTF::encode(bwtResult.transformed);
                vector<uint8_t> haEncoded = HuffmanCanonical::encode(mtfEncoded);

                string bwtPath = encodedDir + "\\" + filename + "_bwt.bin";
                string mtfPath = encodedDir + "\\" + filename + "_mtf.bin";
                string haPath = encodedDir + "\\" + filename + ".ha";
                string decodedPath = decodedDir + "\\" + filename;

                ofstream bwtFile(bwtPath, ios::binary);
                uint64_t primaryIdx = bwtResult.primaryIndex;
                uint64_t dataSize = bwtResult.transformed.size();
                bwtFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
                bwtFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
                bwtFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
                bwtFile.close();

                ofstream mtfFile(mtfPath, ios::binary);
                mtfFile.write(reinterpret_cast<const char*>(mtfEncoded.data()), mtfEncoded.size());
                mtfFile.close();

                ofstream haFile(haPath, ios::binary);
                haFile.write(reinterpret_cast<const char*>(haEncoded.data()), haEncoded.size());
                haFile.close();

                res.processedSize = haEncoded.size();
                res.ratio = (double)res.processedSize / res.originalSize * 100.0;

                string errorMsg;
                string tempDecode = encodedDir + "\\" + filename + "_temp.bin";
                HuffmanCanonical::decodeFile(haPath, tempDecode, errorMsg);

                ifstream tempFile(tempDecode, ios::binary);
                vector<uint8_t> mtfData((istreambuf_iterator<char>(tempFile)), istreambuf_iterator<char>());
                tempFile.close();

                vector<uint8_t> mtfDecoded = MTF::decode(mtfData);

                ifstream bwtInFile(bwtPath, ios::binary);
                uint64_t dataSize2;
                uint64_t primaryIdx2;
                bwtInFile.read(reinterpret_cast<char*>(&dataSize2), sizeof(dataSize2));
                bwtInFile.read(reinterpret_cast<char*>(&primaryIdx2), sizeof(primaryIdx2));
                vector<uint8_t> bwtData(dataSize2);
                bwtInFile.read(reinterpret_cast<char*>(bwtData.data()), dataSize2);
                bwtInFile.close();

                vector<uint8_t> decodedData = BWT::decode(mtfDecoded, primaryIdx2);

                ofstream decFile(decodedPath, ios::binary);
                decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
                decFile.close();

                res.success = (decodedData.size() == inputData.size() &&
                    memcmp(decodedData.data(), inputData.data(), inputData.size()) == 0);
                res.details = res.success ? "PASSED" : "FAILED";
            }
            else if (algorithm == "BWT+MTF+RLE+HA") {
                ifstream inFile(inputPath, ios::binary);
                vector<uint8_t> inputData((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
                inFile.close();
                res.originalSize = inputData.size();

                BWTResult bwtResult = BWT::encodeEfficient(inputData);
                vector<uint8_t> mtfEncoded = MTF::encode(bwtResult.transformed);
                vector<uint8_t> rleEncoded = RLE::encode(mtfEncoded, 1, 2);
                vector<uint8_t> haEncoded = HuffmanCanonical::encode(rleEncoded);

                string bwtPath = encodedDir + "\\" + filename + "_bwt.bin";
                string mtfPath = encodedDir + "\\" + filename + "_mtf.bin";
                string rlePath = encodedDir + "\\" + filename + "_bwt.rle";
                string haPath = encodedDir + "\\" + filename + ".ha";
                string decodedPath = decodedDir + "\\" + filename;

                ofstream bwtFile(bwtPath, ios::binary);
                uint64_t primaryIdx = bwtResult.primaryIndex;
                uint64_t dataSize = bwtResult.transformed.size();
                bwtFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
                bwtFile.write(reinterpret_cast<const char*>(&primaryIdx), sizeof(primaryIdx));
                bwtFile.write(reinterpret_cast<const char*>(bwtResult.transformed.data()), dataSize);
                bwtFile.close();

                ofstream mtfFile(mtfPath, ios::binary);
                mtfFile.write(reinterpret_cast<const char*>(mtfEncoded.data()), mtfEncoded.size());
                mtfFile.close();

                ofstream rleFile(rlePath, ios::binary);
                rleFile.write(reinterpret_cast<const char*>(rleEncoded.data()), rleEncoded.size());
                rleFile.close();

                ofstream haFile(haPath, ios::binary);
                haFile.write(reinterpret_cast<const char*>(haEncoded.data()), haEncoded.size());
                haFile.close();

                res.processedSize = haEncoded.size();
                res.ratio = (double)res.processedSize / res.originalSize * 100.0;

                string errorMsg;
                string tempDecode1 = encodedDir + "\\" + filename + "_temp1.bin";
                string tempDecode2 = encodedDir + "\\" + filename + "_temp2.bin";
                HuffmanCanonical::decodeFile(haPath, tempDecode1, errorMsg);
                RLE::decodeFile(tempDecode1, tempDecode2, errorMsg);

                ifstream tempFile(tempDecode2, ios::binary);
                vector<uint8_t> mtfData((istreambuf_iterator<char>(tempFile)), istreambuf_iterator<char>());
                tempFile.close();

                vector<uint8_t> mtfDecoded = MTF::decode(mtfData);

                ifstream bwtInFile(bwtPath, ios::binary);
                uint64_t dataSize2;
                uint64_t primaryIdx2;
                bwtInFile.read(reinterpret_cast<char*>(&dataSize2), sizeof(dataSize2));
                bwtInFile.read(reinterpret_cast<char*>(&primaryIdx2), sizeof(primaryIdx2));
                vector<uint8_t> bwtData(dataSize2);
                bwtInFile.read(reinterpret_cast<char*>(bwtData.data()), dataSize2);
                bwtInFile.close();

                vector<uint8_t> decodedData = BWT::decode(mtfDecoded, primaryIdx2);

                ofstream decFile(decodedPath, ios::binary);
                decFile.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
                decFile.close();

                res.success = (decodedData.size() == inputData.size() &&
                    memcmp(decodedData.data(), inputData.data(), inputData.size()) == 0);
                res.details = res.success ? "PASSED" : "FAILED";
            }
            else if (algorithm == "Entropy") {
                ifstream inFile(inputPath, ios::binary);
                vector<uint8_t> data((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
                inFile.close();
                res.originalSize = data.size();
                res.success = true;

                EntropyResult entropy = EntropyCalculator::calculate(data, 1);
                res.ratio = entropy.entropy;
                res.details = "Entropy: " + to_string(entropy.entropy) + " bits/byte";
            }

        }
        catch (const exception& e) {
            res.success = false;
            res.details = string("Exception: ") + e.what();
        }

        auto end = high_resolution_clock::now();
        res.timeMs = duration_cast<milliseconds>(end - start).count();

        results.push_back(res);
    }
}

void Analysis::runAllAnalyses(const string& inputDir,
    const string& encodedDir,
    const string& decodedDir) {
    cout << "\n========================================\n";
    cout << "Running All Analyses on All Files\n";
    cout << "========================================\n\n";

    vector<string> algorithms = {
        "RLE", "MTF", "Huffman", "Canonical Huffman", "LZ77", "LZSS", "LZ78", "LZW",
        "BWT+RLE", "BWT+MTF+HA", "BWT+MTF+RLE+HA", "Entropy"
    };

    vector<AlgorithmResult> allResults;

    for (const auto& algorithm : algorithms) {
        cout << "\n========================================\n";
        cout << "Testing: " << algorithm << "\n";
        cout << "========================================\n";

        vector<AlgorithmResult> results;
        runSingleAnalysis(algorithm, inputDir, encodedDir, decodedDir, results);

        for (const auto& res : results) {
            cout << "  " << res.filename << ": ";
            if (res.success) {
                cout << "PASSED";
                if (algorithm == "Entropy") {
                    cout << " (" << fixed << setprecision(4) << res.ratio << " bits/byte)";
                }
                else if (res.processedSize > 0 && res.originalSize > 0) {
                    cout << " (" << fixed << setprecision(2) << res.ratio << "%, "
                        << res.processedSize << "/" << res.originalSize << " bytes, "
                        << res.timeMs << " ms)";
                }
                else {
                    cout << " (" << res.timeMs << " ms)";
                }
            }
            else {
                cout << "FAILED - " << res.details;
            }
            cout << "\n";
        }

        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    printAnalysisSummary(allResults);
}

void Analysis::printAnalysisSummary(const vector<AlgorithmResult>& results) {
    cout << "\n========================================\n";
    cout << "Analysis Summary\n";
    cout << "========================================\n\n";

    map<string, map<string, AlgorithmResult>> grouped;
    for (const auto& res : results) {
        grouped[res.algorithm][res.filename] = res;
    }

    cout << left << setw(25) << "Algorithm";
    vector<string> files = { "BW.raw", "Gray.raw", "RGB.raw", "enwik7.txt", "RusText.txt", "BIN.bin" };
    for (const auto& file : files) {
        cout << setw(15) << file.substr(0, 8);
    }
    cout << "\n";
    cout << string(25 + 15 * files.size(), '-') << "\n";

    for (const auto& algo : grouped) {
        cout << left << setw(25) << algo.first;
        for (const auto& file : files) {
            auto it = algo.second.find(file);
            if (it != algo.second.end() && it->second.success) {
                if (algo.first == "Entropy") {
                    cout << setw(15) << fixed << setprecision(2) << it->second.ratio;
                }
                else if (it->second.ratio > 0) {
                    cout << setw(15) << fixed << setprecision(1) << it->second.ratio << "%";
                }
                else {
                    cout << setw(15) << "PASS";
                }
            }
            else {
                cout << setw(15) << "FAIL";
            }
        }
        cout << "\n";
    }

    cout << "\n========================================\n";
    cout << "Success Rate by Algorithm\n";
    cout << "========================================\n\n";

    for (const auto& algo : grouped) {
        int success = 0;
        int total = 0;
        for (const auto& res : algo.second) {
            total++;
            if (res.second.success) success++;
        }
        cout << left << setw(25) << algo.first << ": " << success << "/" << total
            << " (" << fixed << setprecision(1) << (100.0 * success / total) << "%)\n";
    }

    cout << "\n========================================\n";
    cout << "Success Rate by File\n";
    cout << "========================================\n\n";

    map<string, int> fileSuccess, fileTotal;
    for (const auto& res : results) {
        fileTotal[res.filename]++;
        if (res.success) fileSuccess[res.filename]++;
    }

    for (const auto& file : files) {
        cout << left << setw(20) << file << ": " << fileSuccess[file] << "/" << fileTotal[file]
            << " (" << fixed << setprecision(1) << (100.0 * fileSuccess[file] / fileTotal[file]) << "%)\n";
    }

    cout << "\n";
}