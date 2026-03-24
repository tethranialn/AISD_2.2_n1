#include "raw_convert.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

bool jpgToRaw(const string& inputFile, RawResult& out) {
    struct stat st;
    if (stat(inputFile.c_str(), &st) != 0) {
        return false;
    }
    out.jpgSize = st.st_size;

    int w, h, c;
    unsigned char* img = stbi_load(inputFile.c_str(), &w, &h, &c, 0);
    if (!img) {
        return false;
    }

    out.width = w;
    out.height = h;
    out.pixels = w * h;

    if (c == 1) {
        bool isBlackWhite = true;
        for (uint32_t i = 0; i < out.pixels; i++) {
            uint8_t val = img[i];
            if (val > 10 && val < 245) {
                isBlackWhite = false;
                break;
            }
        }

        if (isBlackWhite) {
            out.type = 1;
            out.data.resize(out.pixels);
            for (uint32_t i = 0; i < out.pixels; i++) {
                out.data[i] = (img[i] > 127) ? 255 : 0;
            }
        }
        else {
            out.type = 2;
            out.data.assign(img, img + out.pixels);
        }
    }
    else {
        vector<uint8_t> gray(out.pixels);
        for (uint32_t i = 0; i < out.pixels; i++) {
            gray[i] = (img[i * 3] + img[i * 3 + 1] + img[i * 3 + 2]) / 3;
        }

        bool isBlackWhite = true;
        int blackCount = 0, whiteCount = 0;

        for (uint32_t i = 0; i < out.pixels; i++) {
            if (gray[i] < 30) blackCount++;
            else if (gray[i] > 225) whiteCount++;
            else {
                isBlackWhite = false;
                break;
            }
        }

        if (isBlackWhite && (blackCount + whiteCount) > out.pixels * 0.95) {
            out.type = 1;
            out.data.resize(out.pixels);
            for (uint32_t i = 0; i < out.pixels; i++) {
                out.data[i] = (gray[i] > 127) ? 255 : 0;
            }
        }
        else {
            bool isGrayscale = true;
            for (uint32_t i = 0; i < out.pixels && isGrayscale; i++) {
                uint8_t r = img[i * 3], g = img[i * 3 + 1], b = img[i * 3 + 2];
                if (r != g || g != b) {
                    isGrayscale = false;
                }
            }

            if (isGrayscale) {
                out.type = 2;
                out.data = gray;
            }
            else {
                out.type = 3;
                out.data.assign(img, img + out.pixels * 3);
            }
        }
    }

    out.rawSize = 5 + out.data.size();

    stbi_image_free(img);
    return true;
}

bool convertImagesToRaw(const string& inputDir, const string& outputDir) {
    const char* imageNames[] = { "BW.jpg", "Gray.jpg", "RGB.jpg" };
    const char* imageTypes[] = { "Black & White", "Grayscale", "Color" };

    for (int i = 0; i < 3; i++) {
        string inputFile = inputDir + "\\" + imageNames[i];
        string outputFile = outputDir + "\\" + string(imageNames[i]).substr(0, string(imageNames[i]).find_last_of('.')) + ".raw";

        RawResult result;
        if (jpgToRaw(inputFile, result)) {
            ofstream f(outputFile, ios::binary);
            if (f.is_open()) {
                f.put(result.type);
                f.write((char*)&result.pixels, 4);
                f.write((char*)result.data.data(), result.data.size());
                f.close();

                cout << imageNames[i] << " -> " << imageTypes[i] << "\n";
                cout << "  Resolution: " << result.width << "x" << result.height << "\n";
                cout << "  JPG size: " << result.jpgSize << " bytes\n";
                cout << "  RAW size: " << result.rawSize << " bytes (header + pixel data)\n";
                cout << "  Pixel data: " << result.data.size() << " bytes\n";
                cout << "  JPG is " << (float)result.jpgSize / result.rawSize * 100 << "% of RAW\n";
                cout << "  JPG compression ratio: " << (float)result.rawSize / result.jpgSize << ":1\n\n";
            }
        }
        else {
            cout << "Failed to convert " << imageNames[i] << "\n";
            return false;
        }
    }

    return true;
}

bool extractPixelData(const string& rawPath, vector<uint8_t>& pixelData, uint8_t& type, uint32_t& pixels) {
    ifstream f(rawPath, ios::binary);
    if (!f.is_open()) {
        return false;
    }

    f.read((char*)&type, 1);
    f.read((char*)&pixels, 4);

    pixelData.clear();
    pixelData.assign((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close();

    return true;
}