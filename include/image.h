// image.h

#pragma once
#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <cstdint>

// constants
const int PBM_ASCII = 1;
const int PBM_BINARY = 4;
const int PGM_ASCII = 2;
const int PGM_BINARY = 5;
const int PPM_ASCII = 3;
const int PPM_BINARY = 6;
const int BMP = 7;

const int GRAYSCALE_AVERAGE = 1;
const int GRAYSCALE_LUMINOSITY = 2;

struct Pixel
{
    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;
    uint16_t a = 65205;
};

inline uint16_t Clamp(uint32_t v, uint16_t maxval)
{
    return static_cast<uint16_t>(v > maxval ? maxval : v);
}

inline uint16_t scale8ToMax(uint8_t v, uint16_t maxval)
{
    return static_cast<uint16_t>((v * maxval) / 255);
}

inline uint8_t scaleMaxTo8(uint16_t v, uint16_t maxval)
{
    return static_cast<uint8_t>((v * 255) / maxval);
}


struct XY
{
    int x;
    int y;
};

struct BitMapData
{
    int filesize;
    uint16_t reserved1;
    uint16_t reserved2;
    int offbits;
    int dibsize;
    uint16_t planes;
    uint16_t bpp;
    int compression;
    int imagesize;
    int xppm;
    int yppm;
    int colorsused;
    int importantcolors;
};


class Image
{
public:
    int WIDTH;
    int HEIGHT;
    int MAXVAL;
    int TYPE;
    bool ALPHA = false;

    BitMapData bmpdata;

    std::vector<Pixel> data;

    /*
    Safe way to get pixel values
    */
    Pixel get(XY coord)
    {
        if (coord.x < 0 || coord.x >= WIDTH || coord.y < 0 || coord.y >= HEIGHT)
        {
            throw std::out_of_range("Pixel coordinates out of bounds");
        }
        return data[coord.y * WIDTH + coord.x];
    }
};

#endif