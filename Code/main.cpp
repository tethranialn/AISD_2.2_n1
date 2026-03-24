#include "raw_convert.h"
#include "rle.h"
#include "entropy.h"
#include "mtf.h"
#include "huffman.h"
#include "bwt.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <cstring>
#include <limits>
#include <sys/stat.h>

using namespace std;

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
    cout << "5. BWT Transform Test (banana)\n";
    cout << "6. BWT + RLE Combined Compression\n";
    cout << "7. Exit\n";
    cout << "========================================\n";
    cout << "Choose algorithm (1-7): ";
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
            BWT::testOnBanana();
            break;
        case 6: {
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
        case 7:
            cout << "Exiting program...\n";
            return 0;
        default:
            cout << "Invalid choice. Please select 1-7.\n";
            break;
        }
    }

    return 0;
}