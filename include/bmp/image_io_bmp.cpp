#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "image.h"

Image loadImageBmp(std::string path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file) return {};

    char magic[2];
    int filesize;
    uint16_t reserved1;
    uint16_t reserved2;
    int offbits;
    int dibsize;
    int width;
    int height;
    uint16_t planes;
    uint16_t bpp;
    int compression;
    int imagesize;
    int xppm;
    int yppm;
    int colorsused;
    int importantcolors;

    file.read(magic, 2);
    file.read(reinterpret_cast<char*>(&filesize), 4);
    file.read(reinterpret_cast<char*>(&reserved1), 2);
    file.read(reinterpret_cast<char*>(&reserved2), 2);
    file.read(reinterpret_cast<char*>(&offbits), 4);
    file.read(reinterpret_cast<char*>(&dibsize), 4);
    file.read(reinterpret_cast<char*>(&width), 4);
    file.read(reinterpret_cast<char*>(&height), 4);
    file.read(reinterpret_cast<char*>(&planes), 2);
    file.read(reinterpret_cast<char*>(&bpp), 2);
    file.read(reinterpret_cast<char*>(&compression), 4);
    file.read(reinterpret_cast<char*>(&imagesize), 4);
    file.read(reinterpret_cast<char*>(&xppm), 4);
    file.read(reinterpret_cast<char*>(&yppm), 4);
    file.read(reinterpret_cast<char*>(&colorsused), 4);
    file.read(reinterpret_cast<char*>(&importantcolors), 4);

    if (magic[0] != 'B' || magic[1] != 'M')
    {
        std::cerr << "Error: Not a BMP file." << std::endl;
        return {};
    }
    if (compression != 0)
    {
        std::cerr << "Error: Compressed BMPs are not supported." << std::endl;
        return {};
    }

    int rowSize = ((bpp * width + 31) / 32) * 4;
    int padding = rowSize - (width * (bpp / 8));

    file.seekg(offbits);

    Image img{width, height, 255};

    if (bpp == 32) img.ALPHA = true;
    else if (bpp == 24) img.ALPHA = false;

    img.TYPE = BMP;

    img.bmpdata.filesize = filesize;
    img.bmpdata.reserved1 = reserved1;
    img.bmpdata.reserved2 = reserved2;
    img.bmpdata.offbits = offbits;
    img.bmpdata.dibsize = dibsize;
    img.bmpdata.planes = planes;
    img.bmpdata.bpp = bpp;
    img.bmpdata.compression = compression;
    img.bmpdata.imagesize = imagesize;
    img.bmpdata.xppm = xppm;
    img.bmpdata.yppm = yppm;
    img.bmpdata.colorsused = colorsused;
    img.bmpdata.importantcolors = importantcolors;

    img.data.resize(width * height);

    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < width; x++)
        {
            Pixel& pixel = img.data[y * width + x];

            uint8_t b, g, r, a = 255;

            file.read(reinterpret_cast<char*>(&b), 1);
            file.read(reinterpret_cast<char*>(&g), 1);
            file.read(reinterpret_cast<char*>(&r), 1);

            if (bpp == 32)
            {
                file.read(reinterpret_cast<char*>(&a), 1);
            }

            pixel.r = r;
            pixel.g = g;
            pixel.b = b;
            pixel.a = a;
        }
        file.ignore(padding);
    }

    return img;
}

int writeImageBmp(std::string path, Image& img)
{
    std::ofstream file(path, std::ios::binary);

    if (!file) return 1;

    int file_size;
    int offbits = 54;
    int dibsize = 40;
    int compression = 0;
    int rowSize;

    if (!img.ALPHA) img.bmpdata.bpp = 24;
    else img.bmpdata.bpp = 32;

    file_size = 54 + ((img.bmpdata.bpp * img.WIDTH + 31) / 32) * 4 * img.HEIGHT;
    rowSize = ((img.bmpdata.bpp * img.WIDTH + 31) / 32) * 4;
    img.bmpdata.filesize = file_size;
    img.bmpdata.reserved1 = 0;
    img.bmpdata.reserved2 = 0;
    img.bmpdata.offbits = offbits;
    img.bmpdata.dibsize = dibsize;
    img.bmpdata.planes = 1;
    img.bmpdata.compression = 0;
    img.bmpdata.imagesize = rowSize * img.HEIGHT;
    img.bmpdata.xppm = 0;
    img.bmpdata.yppm = 0;
    img.bmpdata.colorsused = 0;
    img.bmpdata.importantcolors = 0;

    int padding = rowSize - (img.WIDTH * (img.bmpdata.bpp / 8));

    file.write("BM", 2);
    file.write(reinterpret_cast<const char*>(&file_size), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.reserved1), 2);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.reserved2), 2);
    file.write(reinterpret_cast<const char*>(&offbits), 4);
    file.write(reinterpret_cast<const char*>(&dibsize), 4);
    file.write(reinterpret_cast<const char*>(&img.WIDTH), 4);
    file.write(reinterpret_cast<const char*>(&img.HEIGHT), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.planes), 2);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.bpp), 2);
    file.write(reinterpret_cast<const char*>(&compression), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.imagesize), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.xppm), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.yppm), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.colorsused), 4);
    file.write(reinterpret_cast<const char*>(&img.bmpdata.importantcolors), 4);

    for (int y = img.HEIGHT - 1; y >= 0; y--)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            Pixel pixel = img.data[y * img.WIDTH + x];

            pixel.r = scaleMaxTo8(pixel.r, 255);
            pixel.g = scaleMaxTo8(pixel.g, 255);
            pixel.b = scaleMaxTo8(pixel.b, 255);

            file.write(reinterpret_cast<const char*>(&pixel.b), 1);
            file.write(reinterpret_cast<const char*>(&pixel.g), 1);
            file.write(reinterpret_cast<const char*>(&pixel.r), 1);

            if (img.bmpdata.bpp == 32)
            {
                pixel.a = scale8ToMax(pixel.a, 255);
                file.write(reinterpret_cast<const char*>(&pixel.a), 1);
            }
        }
        file.write(reinterpret_cast<const char*>("\0\0\0"), padding);
    }
    return 0;
}
