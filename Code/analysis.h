#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <string>
#include <vector>

struct AlgorithmResult {
    std::string algorithm;
    std::string filename;
    bool success;
    std::string details;
    double ratio;
    size_t originalSize;
    size_t processedSize;
    long long timeMs;
};

class Analysis {
public:
    static void rleAnalysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void entropyAnalysis(const std::string& inputDir);

    static void mtfAnalysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void huffmanAnalysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void huffmanCanonicalAnalysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void suffixArrayToBWTDemo(const std::string& inputDir);

    static void testBWTAndRLE(const std::string& inputDir);

    static void testEfficientBWT(const std::string& inputDir);

    static void testOnBanana();

    static void lz77Analysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void lzssAnalysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void lz78Analysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void lzwAnalysis(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void printFileSelectionMenu();

    static void runAllAnalyses(const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir);

    static void printAnalysisSummary(const std::vector<AlgorithmResult>& results);

private:
    static bool fileExists(const std::string& path);
    static std::vector<std::string> getSelectedFiles(const std::string& inputDir, int choice);
    static void runSingleAnalysis(const std::string& algorithm,
        const std::string& inputDir,
        const std::string& encodedDir,
        const std::string& decodedDir,
        std::vector<AlgorithmResult>& results);
};

#endif