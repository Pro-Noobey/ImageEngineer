#include <iostream>
#include <fstream>
#include <string>
#include "bmp/image_io_bmp.h"
#include "netpbm/image_io_pbm.h"
#include "image.h"

// IMPORTANT NOTICE!!
/*
THIS CODE IS NOT TESTED IF IT WORKS AS A SAMPLE
DUE TO NEW STRUCTURAL IMPROVEMENTS
*/
std::string input(std::string prompt)
{
    std::string response;
    std::cout << prompt;
    std::getline(std::cin, response);
    return response;
}

Image<RGBA16BitPixel> loadImage(std::string path)
{
    std::ifstream file(path, std::ios::binary);

    char magic[2];
    file.read(magic, 2);

    if (magic[0] == 'B' && magic[1] == 'M')
        return loadImageBmp(path);

    int type = magic[1] - '0'; // IMPORTANT: convert ASCII to number

    if (type == PBM_ASCII) return convertImage<RGBA16BitPixel, RGB16BitPixel>(loadImagePbm(path, false));
    else if (type == PBM_BINARY) return convertImage<RGBA16BitPixel, RGB16BitPixel>(loadImagePbm(path, true));

    else if (type == PGM_ASCII) return convertImage<RGBA16BitPixel, RGB16BitPixel>(loadImagePgm(path, false));
    else if (type == PGM_BINARY) return convertImage<RGBA16BitPixel, RGB16BitPixel>(loadImagePgm(path, true));

    else if (type == PPM_ASCII) return convertImage<RGBA16BitPixel, RGB16BitPixel>(loadImagePpm(path, false));
    else if (type == PPM_BINARY) return convertImage<RGBA16BitPixel, RGB16BitPixel>(loadImagePpm(path, true));

    else if (type == BMP) return loadImageBmp(path);
    throw std::runtime_error("Unknown image format");
}

template<typename Pixel>
int writeImage(std::string path, Image<Pixel>& img, int type, int bmpcompression=BITMAP_RLE8)
{
    
    if (type == PBM_ASCII) return writeImagePbm(path, img, false);
    else if (type == PBM_BINARY) return writeImagePbm(path, img, true);

    else if (type == PGM_ASCII) return writeImagePgm(path, img, false);
    else if (type == PGM_BINARY) return writeImagePgm(path, img, true);

    else if (type == PPM_ASCII) return writeImagePpm(path, img, false);
    else if (type == PPM_BINARY) return writeImagePpm(path, img, true);
    
    else if (type == BMP) return writeImageBmp(path, img, 8, bmpcompression);

    else return 2;
}


int main()
{
    std::string path = input("Enter image path\n>>");
    auto img = loadImage(path);
    std::string output_path = input("Enter dst path\n>>");
    int type = std::stoi(input("Enter type (1 = PBM_ASCII, 4 = PBM_BINARY, 2 = PGM_ASCII, 5 = PGM_BINARY, 3 = PPM_ASCII, 6 = PPM_BINARY, 7 = BMP)\n>>"));
    int write_code = writeImage(output_path, img, type);

    return write_code;
}
