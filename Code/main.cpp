#include "raw_convert.h"
#include "compressors.h"
#include "analysis.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <limits>
#include <functional>
#include <vector>

using namespace std;

namespace fs = std::filesystem;

void printMainMenu() {
    cout << "\n========================================\n";
    cout << "Compression & Analysis Tool\n";
    cout << "========================================\n";
    cout << "1. Debug Mode (Algorithm Analysis)\n";
    cout << "2. Compressors Mode (Test All Compressors)\n";
    cout << "3. Exit\n";
    cout << "========================================\n";
    cout << "Choose mode (1-3): ";
}

void printCompressorsMenu() {
    cout << "\n========================================\n";
    cout << "Compressors Mode\n";
    cout << "========================================\n";
    cout << "1. Test all compressors on all files\n";
    cout << "2. Test specific compressor\n";
    cout << "3. Back to main menu\n";
    cout << "========================================\n";
    cout << "Choose option (1-3): ";
}

void printCompressorList() {
    cout << "\nAvailable compressors:\n";
    cout << "1. HA (Canonical Huffman)\n";
    cout << "2. RLE (Run-Length Encoding)\n";
    cout << "3. BWT + RLE\n";
    cout << "4. BWT + MTF + HA\n";
    cout << "5. BWT + MTF + RLE + HA\n";
    cout << "6. LZSS\n";
    cout << "7. LZSS + HA\n";
    cout << "8. LZW\n";
    cout << "9. LZW + HA\n";
    cout << "Enter choice (1-9): ";
}

void runCompressorsOnAllFiles(const string& inputDir, const string& outputDir) {
    vector<string> files = { "BW.raw", "Gray.raw", "RGB.raw", "enwik7.txt", "RusText.txt", "BIN.bin" };
    vector<CompressionResult> allResults;

    vector<pair<string, function<CompressionResult(const string&, const string&)>>> compressors;
    compressors.push_back({ "HA", Compressors::HA });
    compressors.push_back({ "RLE", Compressors::RLE });
    compressors.push_back({ "BWT+RLE", Compressors::BWTRLE });
    compressors.push_back({ "BWT+MTF+HA", Compressors::BWTMTFHA });
    compressors.push_back({ "BWT+MTF+RLE+HA", Compressors::BWTMTFRLEHA });
    compressors.push_back({ "LZSS", Compressors::LZSS });
    compressors.push_back({ "LZSS+HA", Compressors::LZSSHA });
    compressors.push_back({ "LZW", Compressors::LZW });
    compressors.push_back({ "LZW+HA", Compressors::LZWHA });

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;
        if (!fs::exists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n";
            continue;
        }

        cout << "\n========================================\n";
        cout << "Testing on: " << filename << "\n";
        cout << "========================================\n";

        string basePath = outputDir + "\\" + filename;

        for (size_t i = 0; i < compressors.size(); i++) {
            cout << "\n" << compressors[i].first << ":\n";
            cout << "----------------------------------------\n";
            CompressionResult result = compressors[i].second(inputPath, basePath + "_" + compressors[i].first);
            allResults.push_back(result);
            Compressors::printResult(result);
        }
    }

    Compressors::printSummary(allResults);
}

void runSpecificCompressor(const string& inputDir, const string& outputDir) {
    printCompressorList();
    int compChoice;
    cin >> compChoice;

    if (compChoice < 1 || compChoice > 9) {
        cout << "Invalid choice\n";
        return;
    }

    Analysis::printFileSelectionMenu();
    int fileChoice;
    cin >> fileChoice;

    if (fileChoice == 8) {
        cout << "Operation cancelled\n";
        return;
    }

    vector<string> allFiles = { "BW.raw", "Gray.raw", "RGB.raw", "enwik7.txt", "RusText.txt", "BIN.bin" };
    vector<string> files;

    if (fileChoice == 7) {
        files = allFiles;
    }
    else if (fileChoice >= 1 && fileChoice <= 6) {
        files.push_back(allFiles[fileChoice - 1]);
    }
    else {
        cout << "Invalid choice\n";
        return;
    }

    vector<pair<string, function<CompressionResult(const string&, const string&)>>> compressors;
    compressors.push_back({ "HA", Compressors::HA });
    compressors.push_back({ "RLE", Compressors::RLE });
    compressors.push_back({ "BWT+RLE", Compressors::BWTRLE });
    compressors.push_back({ "BWT+MTF+HA", Compressors::BWTMTFHA });
    compressors.push_back({ "BWT+MTF+RLE+HA", Compressors::BWTMTFRLEHA });
    compressors.push_back({ "LZSS", Compressors::LZSS });
    compressors.push_back({ "LZSS+HA", Compressors::LZSSHA });
    compressors.push_back({ "LZW", Compressors::LZW });
    compressors.push_back({ "LZW+HA", Compressors::LZWHA });

    auto comp = compressors[compChoice - 1];
    vector<CompressionResult> results;

    for (const auto& filename : files) {
        string inputPath = inputDir + "\\" + filename;
        if (!fs::exists(inputPath)) {
            cout << "File not found: " << filename << " - skipping\n";
            continue;
        }

        cout << "\n========================================\n";
        cout << comp.first << " on " << filename << "\n";
        cout << "========================================\n";

        string basePath = outputDir + "\\" + filename + "_" + comp.first;
        CompressionResult result = comp.second(inputPath, basePath);
        results.push_back(result);
        Compressors::printResult(result);
    }

    Compressors::printSummary(results);
}

void printDebugMenu() {
    cout << "\n========================================\n";
    cout << "Debug Mode - Algorithm Analysis\n";
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
    cout << "14. Run ALL Analyses on ALL Files\n";
    cout << "15. Back to main menu\n";
    cout << "========================================\n";
    cout << "Choose algorithm (1-15): ";
}

int main() {
    const string BASE_PATH = "C:\\Users\\chivo\\Documents\\AISD_2.2_n1";
    const string INPUT_DIR = BASE_PATH + "\\Input files";
    const string ENCODED_DIR = BASE_PATH + "\\Encoded files";
    const string DECODED_DIR = BASE_PATH + "\\Decoded files";
    const string COMPRESSOR_DIR = BASE_PATH + "\\Compressor results";

    fs::create_directories(INPUT_DIR);
    fs::create_directories(ENCODED_DIR);
    fs::create_directories(DECODED_DIR);
    fs::create_directories(COMPRESSOR_DIR);

    cout << "Converting images to RAW format...\n";
    cout << "----------------------------------------\n";
    if (!convertImagesToRaw(INPUT_DIR, INPUT_DIR)) {
        cout << "Error: Failed to convert images\n";
        return 1;
    }
    cout << "Images converted successfully!\n";
    cout << "\n========================================\n";
    cout << "File Storage Locations:\n";
    cout << "========================================\n";
    cout << "Input files:     " << INPUT_DIR << "\n";
    cout << "Encoded files:   " << ENCODED_DIR << "\n";
    cout << "Decoded files:   " << DECODED_DIR << "\n";
    cout << "Compressor results: " << COMPRESSOR_DIR << "\n";
    cout << "========================================\n";
    cout << "Note: All encoded and decoded files are saved permanently\n";
    cout << "      in the respective directories for later analysis.\n\n";

    while (true) {
        printMainMenu();
        int mode;
        cin >> mode;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        if (mode == 3) {
            cout << "Exiting program...\n";
            break;
        }

        if (mode == 2) {
            while (true) {
                printCompressorsMenu();
                int compMode;
                cin >> compMode;

                if (compMode == 3) break;

                if (compMode == 1) {
                    runCompressorsOnAllFiles(INPUT_DIR, COMPRESSOR_DIR);
                }
                else if (compMode == 2) {
                    runSpecificCompressor(INPUT_DIR, COMPRESSOR_DIR);
                }
                else {
                    cout << "Invalid choice\n";
                }
            }
        }
        else if (mode == 1) {
            while (true) {
                printDebugMenu();
                int debugChoice;
                cin >> debugChoice;

                if (debugChoice == 15) break;

                if (debugChoice == 14) {
                    Analysis::runAllAnalyses(INPUT_DIR, ENCODED_DIR, DECODED_DIR);
                }
                else {
                    switch (debugChoice) {
                    case 1: Analysis::rleAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 2: Analysis::entropyAnalysis(INPUT_DIR); break;
                    case 3: Analysis::mtfAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 4: Analysis::huffmanAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 5: Analysis::huffmanCanonicalAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 6: Analysis::testOnBanana(); break;
                    case 7: Analysis::testBWTAndRLE(INPUT_DIR); break;
                    case 8: Analysis::suffixArrayToBWTDemo(INPUT_DIR); break;
                    case 9: Analysis::testEfficientBWT(INPUT_DIR); break;
                    case 10: Analysis::lz77Analysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 11: Analysis::lzssAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 12: Analysis::lz78Analysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    case 13: Analysis::lzwAnalysis(INPUT_DIR, ENCODED_DIR, DECODED_DIR); break;
                    default: cout << "Invalid choice\n"; break;
                    }
                }
            }
        }
        else {
            cout << "Invalid choice. Please select 1-3.\n";
        }
    }

    return 0;
}