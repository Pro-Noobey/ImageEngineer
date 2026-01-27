#include <iostream>
#include <fstream>
#include <string>
#include "bmp/image_io_bmp.h"
#include "netpbm/image_io_pbm.h"
#include "image.h"

std::string input(std::string prompt)
{
    std::string response;
    std::cout << prompt;
    std::getline(std::cin, response);
    return response;
}

Image loadImage(std::string path)
{
    std::ifstream file(path, std::ios::binary);

    char magic[2];
    file.read(magic, 2);

    if (magic[0] == 'B' && magic[1] == 'M')
        return loadImageBmp(path);

    int type = magic[1] - '0'; // IMPORTANT: convert ASCII to number

    if (type == PBM_ASCII) return loadImagePBM(path, false);
    else if (type == PBM_BINARY) return loadImagePBM(path, true);

    else if (type == PGM_ASCII) return loadImagePGM(path, false);
    else if (type == PGM_BINARY) return loadImagePGM(path, true);

    else if (type == PPM_ASCII) return loadImagePPM(path, false);
    else if (type == PPM_BINARY) return loadImagePPM(path, true);

    throw std::runtime_error("Unknown image format");
}


int writeImage(std::string path, Image& img, int type)
{
    if (type == PBM_ASCII) return writeImagePBM(path, img, false);
    else if (type == PBM_BINARY) return writeImagePBM(path, img, true);

    else if (type == PGM_ASCII) return writeImagePGM(path, img, false);
    else if (type == PGM_BINARY) return writeImagePGM(path, img, true);

    else if (type == PPM_ASCII) return writeImagePPM(path, img, false);
    else if (type == PPM_BINARY) return writeImagePPM(path, img, true);

    else if (type == BMP) return writeImageBmp(path, img);

    else return 2;
}


int main()
{
    std::string path = input("Enter image path\n>>");
    Image img = loadImage(path);
    std::string output_path = input("Enter dst path\n>>");
    int write_code = writeImage(output_path, img, BMP);

    return write_code;
}