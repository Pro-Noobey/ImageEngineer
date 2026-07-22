// ImageEngineer.h

#pragma once
#ifndef IMAGEENGINEER_H
#define IMAGEENGINEER_H

// IMPORTANT NOTICE!!!!!!
/*
THIS SINGLE HEADER IS NOW OUTDATED. PLEASE USE THE REGULAR!!
THIS IS KEPT FOR HISTORICAL PURPOSES!!
*/
/**
 * @file ImageEngineer.h
 * @brief Single-header aggregation of the project's image types, IO and ops.
 *
 * This header combines the previous `image.h`, `netpbm/image_io_pbm.h`,
 * `bmp/image_io_bmp.h` and `ops.h` into one convenient include for users of
 * the ImageEngineer library.
 */

#include <vector>
#include <cstdint>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <omp.h>

using namespace std::string_literals;

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


const int BITMAP_RLE8 = 1;
const int BITMAP_RLE4 = 2;

/** Pixel types */
struct Gray8BitPixel { uint8_t value = 0; };
struct Gray16BitPixel { uint16_t value = 0; };
struct BWPixel { bool value = false; };
struct RGB8BitPixel { uint8_t r = 0; uint8_t g = 0; uint8_t b = 0; };
struct RGB16BitPixel { uint16_t r = 0; uint16_t g = 0; uint16_t b = 0; };
struct RGBA8BitPixel { uint8_t r = 0; uint8_t g = 0; uint8_t b = 0; uint8_t a = 255; };
struct RGBA16BitPixel { uint16_t r = 0; uint16_t g = 0; uint16_t b = 0; uint16_t a = 65535; };

/** Utility helpers */
inline uint16_t Clamp(int v, int maxval) { return static_cast<uint16_t>(v > maxval ? maxval : v); }
inline uint16_t scale8ToMax(int v, int maxval) { return static_cast<uint16_t>((v * maxval) / 255); }
inline uint8_t scaleMaxTo8(int v, int maxval) { return static_cast<uint8_t>((v * 255) / maxval); }

/** Conversion helpers (declarations and definitions) */
template<typename Pixel> inline BWPixel toBW(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGB8BitPixel> || std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        return BWPixel{ (static_cast<uint16_t>(p.r) + p.g + p.b) / 3 >= 128 };
    }
    else if constexpr (std::is_same_v<Pixel, RGB16BitPixel> || std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        uint32_t avg = (static_cast<uint32_t>(p.r) + p.g + p.b) / 3;
        return BWPixel{ avg >= 32768 };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        return BWPixel{ p.value >= 128 };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        return BWPixel{ p.value >= 32768 };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        return p;
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline BWPixel toBW()\n");
}

template<typename Pixel> inline Gray16BitPixel toGray16Bit(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGB8BitPixel> || std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        uint16_t avg8 = static_cast<uint16_t>((static_cast<uint16_t>(p.r) + p.g + p.b) / 3);
        return Gray16BitPixel{ scale8ToMax(static_cast<uint8_t>(avg8), 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, RGB16BitPixel> || std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        uint32_t avg = (static_cast<uint32_t>(p.r) + p.g + p.b) / 3;
        return Gray16BitPixel{ Clamp(avg, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        return Gray16BitPixel{ p.value ? 65535u : 0u };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        return Gray16BitPixel{ scale8ToMax(p.value, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        return p;
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline Gray16BitPixel toGray16Bit()\n");
}

template<typename Pixel> inline RGB8BitPixel toRGB8Bit(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGB8BitPixel>)
    {
        return p;
    }
    else if constexpr (std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        return RGB8BitPixel{ p.r, p.g, p.b };
    }
    else if constexpr (std::is_same_v<Pixel, RGB16BitPixel>)
    {
        return RGB8BitPixel{ scaleMaxTo8(p.r, 65535), scaleMaxTo8(p.g, 65535), scaleMaxTo8(p.b, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        return RGB8BitPixel{ scaleMaxTo8(p.r, 65535), scaleMaxTo8(p.g, 65535), scaleMaxTo8(p.b, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        return RGB8BitPixel{ p.value, p.value, p.value };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        uint8_t v = scaleMaxTo8(p.value, 65535);
        return RGB8BitPixel{ v, v, v };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        uint8_t v = p.value ? 255 : 0;
        return RGB8BitPixel{ v, v, v };
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline RGB8BitPixel toRGB8Bit()\n");
}

template<typename Pixel> inline RGBA8BitPixel toRGBA8Bit(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        return p;
    }
    else if constexpr (std::is_same_v<Pixel, RGB8BitPixel>)
    {
        return RGBA8BitPixel{ p.r, p.g, p.b, 255 };
    }
    else if constexpr (std::is_same_v<Pixel, RGB16BitPixel>)
    {
        return RGBA8BitPixel{ scaleMaxTo8(p.r, 65535), scaleMaxTo8(p.g, 65535), scaleMaxTo8(p.b, 65535), 255 };
    }
    else if constexpr (std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        return RGBA8BitPixel{ scaleMaxTo8(p.r, 65535), scaleMaxTo8(p.g, 65535), scaleMaxTo8(p.b, 65535), scaleMaxTo8(p.a, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        return RGBA8BitPixel{ p.value, p.value, p.value, 255 };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        uint8_t v = scaleMaxTo8(p.value, 65535);
        return RGBA8BitPixel{ v, v, v, 255 };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        uint8_t v = p.value ? 255 : 0;
        return RGBA8BitPixel{ v, v, v, 255 };
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline RGBA8BitPixel toRGBA8Bit()\n");
}

template<typename Pixel> inline RGB16BitPixel toRGB16Bit(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGB16BitPixel>)
    {
        return p;
    }
    else if constexpr (std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        return RGB16BitPixel{ p.r, p.g, p.b };
    }
    else if constexpr (std::is_same_v<Pixel, RGB8BitPixel>)
    {
        return RGB16BitPixel{ scale8ToMax(p.r, 65535), scale8ToMax(p.g, 65535), scale8ToMax(p.b, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        return RGB16BitPixel{ scale8ToMax(p.r, 65535), scale8ToMax(p.g, 65535), scale8ToMax(p.b, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        uint16_t v = scale8ToMax(p.value, 65535);
        return RGB16BitPixel{ v, v, v };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        return RGB16BitPixel{ p.value, p.value, p.value };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        uint16_t v = p.value ? 65535 : 0;
        return RGB16BitPixel{ v, v, v };
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline RGB16BitPixel toRGB16Bit()\n");
}

template<typename Pixel> inline RGBA16BitPixel toRGBA16Bit(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        return p;
    }
    else if constexpr (std::is_same_v<Pixel, RGB16BitPixel>)
    {
        return RGBA16BitPixel{ p.r, p.g, p.b, 65535 };
    }
    else if constexpr (std::is_same_v<Pixel, RGB8BitPixel>)
    {
        return RGBA16BitPixel{ scale8ToMax(p.r, 65535), scale8ToMax(p.g, 65535), scale8ToMax(p.b, 65535), 65535 };
    }
    else if constexpr (std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        return RGBA16BitPixel{ scale8ToMax(p.r, 65535), scale8ToMax(p.g, 65535), scale8ToMax(p.b, 65535), scale8ToMax(p.a, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        uint16_t v = scale8ToMax(p.value, 65535);
        return RGBA16BitPixel{ v, v, v, 65535 };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        return RGBA16BitPixel{ p.value, p.value, p.value, 65535 };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        uint16_t v = p.value ? 65535 : 0;
        return RGBA16BitPixel{ v, v, v, 65535 };
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline RGBA16BitPixel toRGBA16Bit()\n");
}

template<typename Pixel> inline Gray8BitPixel toGray8Bit(Pixel p)
{
    if constexpr (std::is_same_v<Pixel, RGB8BitPixel> || std::is_same_v<Pixel, RGBA8BitPixel>)
    {
        uint16_t avg = static_cast<uint16_t>((static_cast<uint16_t>(p.r) + p.g + p.b) / 3);
        return Gray8BitPixel{ static_cast<uint8_t>(avg) };
    }
    else if constexpr (std::is_same_v<Pixel, RGB16BitPixel> || std::is_same_v<Pixel, RGBA16BitPixel>)
    {
        uint32_t avg = (static_cast<uint32_t>(p.r) + p.g + p.b) / 3;
        return Gray8BitPixel{ scaleMaxTo8(static_cast<uint16_t>(avg), 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, BWPixel>)
    {
        return Gray8BitPixel{ p.value ? 255 : 0 };
    }
    else if constexpr (std::is_same_v<Pixel, Gray16BitPixel>)
    {
        return Gray8BitPixel{ scaleMaxTo8(p.value, 65535) };
    }
    else if constexpr (std::is_same_v<Pixel, Gray8BitPixel>)
    {
        return p;
    }
    throw std::runtime_error("Type is not supported. Function\n>> inline Gray8BitPixel toGray8Bit()\n");
}

// Forward declare Image template and XY so they can be used by templates below
template<typename Pixel> class Image;

struct XY { int x; int y; };

template<typename PixelReturnType, typename PixelInputType>
PixelReturnType convertPixel(PixelInputType p)
{
    if constexpr (std::is_same_v<PixelReturnType, BWPixel>) return toBW(p);
    else if constexpr (std::is_same_v<PixelReturnType, Gray16BitPixel>) return toGray16Bit(p);
    else if constexpr (std::is_same_v<PixelReturnType, RGB8BitPixel>) return toRGB8Bit(p);
    else if constexpr (std::is_same_v<PixelReturnType, RGBA8BitPixel>) return toRGBA8Bit(p);
    else if constexpr (std::is_same_v<PixelReturnType, RGB16BitPixel>) return toRGB16Bit(p);
    else if constexpr (std::is_same_v<PixelReturnType, RGBA16BitPixel>) return toRGBA16Bit(p);
    else if constexpr (std::is_same_v<PixelReturnType, Gray8BitPixel>) return toGray8Bit(p);
    else throw std::runtime_error(std::string("Unsupported conversion from ") + typeid(PixelInputType).name() + " to " + typeid(PixelReturnType).name());
}

template<typename PixelReturnType, typename PixelInputType>
Image<PixelReturnType> convertImage(const Image<PixelInputType>& img)
{
    Image<PixelReturnType> new_img(img.WIDTH, img.HEIGHT, img.MAXVAL, img.TYPE, img.CHANNELS);
    for (int y = 0; y < img.HEIGHT; y++)
        for (int x = 0; x < img.WIDTH; x++)
            new_img.Set(XY{x, y}, convertPixel<PixelReturnType, PixelInputType>(img.Get(XY{x, y})));
    return new_img;
}

/**
 * @brief Generic image container.
 */
template<typename Pixel>
class Image
{
private:
    std::vector<Pixel> PixelData;
public:
    int WIDTH;
    int HEIGHT;
    int MAXVAL;
    int TYPE;
    int CHANNELS = 3;

    Image(int width, int height, int maxval, int type, int channels = 3)
    : WIDTH(width), HEIGHT(height), MAXVAL(maxval), TYPE(type), CHANNELS(channels) { Init(); }

    void Init() { PixelData.resize(WIDTH * HEIGHT); }
    Pixel Get(XY coord) const { if (coord.x < 0 || coord.x >= WIDTH || coord.y < 0 || coord.y >= HEIGHT) throw std::out_of_range("Pixel coordinates out of bounds"); return PixelData[coord.y * WIDTH + coord.x]; }
    void Set(XY coord, Pixel value) { if (coord.x < 0 || coord.x >= WIDTH || coord.y < 0 || coord.y >= HEIGHT) throw std::out_of_range("Pixel coordinates out of bounds"); PixelData[coord.y * WIDTH + coord.x] = value; }
    void DeleteImageBuffer() { PixelData.clear(); }
    void ResizeImage(int new_width, int new_height) { WIDTH = new_width; HEIGHT = new_height; Init(); }
};

// ---------------- Netpbm IO ----------------

/**
 * @brief Read ASCII file and remove comments (#... lines).
 */
std::string clean(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    std::string cleaned;
    std::string line;
    while (std::getline(file, line)) { if ((!line.empty() && line[0] == '#')) continue; cleaned += line; cleaned += "\n"; }
    return cleaned;
}

/** Extract 8 bits MSB-first */
std::vector<bool> extract(uint8_t byte) { std::vector<bool> bits; for (int i = 7; i >= 0; i--) bits.push_back(((byte >> i) & 1) != 0); return bits; }

Image<RGB16BitPixel> loadImagePbm(const std::string path, bool binary)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error(std::string("File Did not open, Path: ") + path + '\n');
    char magic[2]; int width, height;
    if (binary)
    {
        file.read(magic, 2);
        if (magic[0] != 'P' && magic[1] != '4') throw std::runtime_error("Magic is not P4");
        while (file >> std::ws && file.peek() == '#') { std::string dummy; std::getline(file, dummy); }
        file >> width >> height; file.get();
        int row_bytes = (width + 7) / 8; int image_size = row_bytes * height;
        Image<RGB16BitPixel> img{width, height, 65035, PBM_BINARY, 3};
        std::vector<uint8_t> buffer(image_size); file.read(reinterpret_cast<char*>(buffer.data()), image_size);
        for (int y = 0; y < height; y++) for (int xb = 0; xb < row_bytes; xb++) { uint8_t byte = buffer[y * row_bytes + xb]; auto bits = extract(byte); int bitpos = 7; for (const bool bit : bits) { int x = xb * 8 + (7 - bitpos); if (x >= width) { bitpos--; continue; } img.Set(XY{x, y}, bit ? RGB16BitPixel{0,0,0} : RGB16BitPixel{65035,65035,65035}); bitpos--; } }
        return img;
    }
    std::istringstream filec(clean(path)); filec >> magic >> width >> height; if (magic[0] != 'P' && magic[1] != '1') throw std::runtime_error("Magic is not P1");
    Image<RGB16BitPixel> img{width, height, 65035, PBM_ASCII, 3};
    for (int y = 0; y < height; y++) for (int x = 0; x < width; x++) { char bit; filec >> bit; if (bit == '0') img.Set(XY{x, y}, RGB16BitPixel{0,0,0}); if (bit == '1') img.Set(XY{x, y}, RGB16BitPixel{65035,65035,65035}); }
    return img;
}

Image<RGB16BitPixel> loadImagePgm(const std::string path, bool binary)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error(std::string("File Did not open, Path: ") + path + '\n');
    char magic[2]; int width, height, maxval;
    file.read(magic, 2);
    if (binary)
    {
        if (magic[0] != 'P' && magic[1] != '5') throw std::runtime_error("Magic is not P5");
        while (file >> std::ws && file.peek() == '#') { std::string dummy; std::getline(file, dummy); }
        file >> width >> height >> maxval; file.get(); if (maxval > 65035) throw std::runtime_error("Maxval too large");
        Image<RGB16BitPixel> img{width, height, maxval, PGM_BINARY, 3}; bool bit16 = (maxval > 255);
        if (!bit16) { std::vector<Gray8BitPixel> buffer(width * height); file.read(reinterpret_cast<char*>(buffer.data()), width * height); for (int y=0;y<height;y++) for (int x=0;x<width;x++) img.Set(XY{x,y}, toRGB16Bit(buffer[y*width + x])); return img; }
        std::vector<Gray16BitPixel> buffer16(width * height); file.read(reinterpret_cast<char*>(buffer16.data()), width * height);
        for (int y=0;y<height;y++) for (int x=0;x<width;x++) img.Set(XY{x,y}, toRGB16Bit(buffer16[y*width + x]));
        return img;
    }
    std::istringstream filec(clean(path)); filec.read(magic, 2); if (magic[0] != 'P' && magic[1] != '2') throw std::runtime_error("Magic is not P2"); filec >> width >> height >> maxval; if (maxval > 65035) throw std::runtime_error("Maxval too large"); Image<RGB16BitPixel> img{width, height, maxval, PGM_ASCII, 3}; for (int y=0;y<height;y++) for (int x=0;x<width;x++) { int value; filec >> value; uint16_t gray = scale8ToMax(value, maxval); img.Set(XY{x,y}, RGB16BitPixel{gray,gray,gray}); } return img;
}

Image<RGB16BitPixel> loadImagePpm(const std::string path, bool binary)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error(std::string("File Did not open, Path: ") + path + '\n');
    char magic[2]; int width, height, maxval;
    if (binary)
    {
        file.read(magic, 2); if (magic[0] != 'P' && magic[1] != '6') throw std::runtime_error("Magic is not P6"); while (file >> std::ws && file.peek() == '#') { std::string dummy; std::getline(file, dummy); } file >> width >> height >> maxval; file.get(); if (maxval > 65035) throw std::runtime_error("Maxval too large"); Image<RGB16BitPixel> img{width, height, maxval, PPM_BINARY, 3}; bool bit16 = (maxval > 255);
        if (!bit16) { std::vector<RGB8BitPixel> buffer8Bit(width * height * 3); file.read(reinterpret_cast<char*>(buffer8Bit.data()), width * height * 3); for (int y=0;y<height;y++) for (int x=0;x<width;x++) { RGB8BitPixel p = buffer8Bit[(y*width + x)]; RGB16BitPixel p16 = toRGB16Bit(p); img.Set(XY{x,y}, p16); } return img; }
        std::vector<RGB16BitPixel> buffer16Bit(width * height); file.read(reinterpret_cast<char*>(buffer16Bit.data()), width * height * 3); for (int y=0;y<height;y++) for (int x=0;x<width;x++) img.Set(XY{x,y}, buffer16Bit[y*width + x]); return img;
    }
    file.read(magic, 2); if (magic[0] != 'P' && magic[1] != '3') throw std::runtime_error("Magic is not P3"); std::istringstream filec(clean(path)); filec >> magic >> width >> height >> maxval; if (maxval > 65035) throw std::runtime_error("Maxval too large"); Image<RGB16BitPixel> img{width, height, maxval, PPM_ASCII, 3}; for (int y=0;y<height;y++) for (int x=0;x<width;x++) { int r,g,b; filec >> r >> g >> b; img.Set(XY{x,y}, RGB16BitPixel{scale8ToMax(r, maxval), scale8ToMax(g, maxval), scale8ToMax(b, maxval)}); } return img;
}

template<typename Pixel>
int writeImagePbm(const std::string path, Image<Pixel>& img, bool binary)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return 1;
    if (binary)
    {
        std::string header = "P4\n" + std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n";
        file << header;
        for (int y = 0; y < img.HEIGHT; y++) for (int x = 0; x < img.WIDTH; x++) { Pixel p = img.Get(XY{x, y}); RGB8BitPixel p8Bit = toRGB8Bit(p); uint8_t byte = (p8Bit.r > 127 ? 0 : 1) << 7 | (p8Bit.g > 127 ? 0 : 1) << 6 | (p8Bit.b > 127 ? 0 : 1) << 5; file.write(reinterpret_cast<const char*>(&byte), sizeof(uint8_t)); }
        return 0;
    }
    else
    {
        std::string header = "P1\n" + std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n";
        file << header;
        for (int y = 0; y < img.HEIGHT; y++) for (int x = 0; x < img.WIDTH; x++) { Pixel p = img.Get(XY{x, y}); RGB8BitPixel p8Bit = toRGB8Bit(p); file << ((p8Bit.r > 127 || p8Bit.g > 127 || p8Bit.b > 127) ? '1' : '0') << "\n"; }
        return 0;
    }
}

template<typename Pixel>
int writeImagePgm(const std::string path, Image<Pixel>& img, bool binary)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return 1;
    if (binary)
    {
        std::string header = "P5\n" + std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n" + std::to_string(img.MAXVAL) + "\n";
        file << header;
        for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { Pixel p = img.Get(XY{x,y}); RGB8BitPixel p8Bit = toRGB8Bit(p); file.write(reinterpret_cast<const char*>(&p8Bit), sizeof(RGB8BitPixel)); }
        return 0;
    }
    else
    {
        std::string header = "P2\n" + std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n" + std::to_string(img.MAXVAL) + "\n";
        file << header;
        for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { Pixel p = img.Get(XY{x,y}); RGB8BitPixel p8Bit = toRGB8Bit(p); file << static_cast<int>(p8Bit.r) << " " << static_cast<int>(p8Bit.g) << " " << static_cast<int>(p8Bit.b) << "\n"; }
        return 0;
    }
}

template<typename Pixel>
int writeImagePpm(const std::string path, Image<Pixel>& img, bool binary)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error(std::string("File Did not open for writing, Path: ") + path + '\n');
    if (binary)
    {
        std::string header = "P6\n" + std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n" + std::to_string(img.MAXVAL) + "\n";
        file << header;
        for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { Pixel p = img.Get(XY{x,y}); RGB8BitPixel p8Bit = toRGB8Bit(p); RGB16BitPixel p16Bit = toRGB16Bit(p); if (img.MAXVAL > 255) file.write(reinterpret_cast<const char*>(&p16Bit), sizeof(RGB16BitPixel)); else file.write(reinterpret_cast<const char*>(&p8Bit), sizeof(RGB8BitPixel)); }
        return 0;
    }
    std::string header = "P3\n" + std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n" + std::to_string(img.MAXVAL) + "\n";
    file << header;
    for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { Pixel p = img.Get(XY{x,y}); RGB8BitPixel p8Bit = toRGB8Bit(p); RGB16BitPixel p16Bit = toRGB16Bit(p); if (img.MAXVAL > 255) file << p16Bit.r << " " << p16Bit.g << " " << p16Bit.b << "\n"; else file << p8Bit.r << " " << p8Bit.g << " " << p8Bit.b << "\n"; }
    return 0;
}

// ---------------- BMP IO ----------------

struct CompressedBmpPixel { uint8_t runlength; uint8_t index; };
struct CompressedBmp { int compression; std::vector<RGB8BitPixel> ColorPalette; std::vector<CompressedBmpPixel> data; };

bool PixelIsSimilar(RGB8BitPixel px, RGB8BitPixel dpx, int threshold)
{ return (abs(int(px.r) - int(dpx.r)) <= threshold) && (abs(int(px.g) - int(dpx.g)) <= threshold) && (abs(int(px.b) - int(dpx.b)) <= threshold); }

int findOrAddColorToPalette(RGB8BitPixel px, std::vector<RGB8BitPixel>& colorPalette, int threshold)
{
    for (size_t i=0;i<colorPalette.size();i++) if (PixelIsSimilar(px, colorPalette[i], threshold)) return static_cast<int>(i);
    if (colorPalette.size() < 256) { colorPalette.push_back(px); return static_cast<int>(colorPalette.size()) - 1; }
    int BestIndex = 0; int BestDistance = 255*3 + 1;
    for (size_t i=0;i<colorPalette.size();i++) { int dr = int(px.r) - int(colorPalette[i].r); int dg = int(px.g) - int(colorPalette[i].g); int db = int(px.b) - int(colorPalette[i].b); int Distance = abs(dr) + abs(dg) + abs(db); if (Distance < BestDistance) { BestIndex = static_cast<int>(i); BestDistance = Distance; } }
    return BestIndex;
}

template<typename Pixel>
CompressedBmp compressBmp(Image<Pixel>& img, int compression = BITMAP_RLE8)
{
    CompressedBmp bmpimg{compression};
    for (int y = img.HEIGHT - 1; y >= 0; y--)
    {
        int runLength = 0; int runColorIndex = 0;
        for (int x = 0; x < img.WIDTH; x++)
        {
            RGB8BitPixel px = toRGB8Bit(img.Get(XY{x, y}));
            int idx = findOrAddColorToPalette(px, bmpimg.ColorPalette, 10);
            if (runLength == 0) { runLength = 1; runColorIndex = idx; }
            else if (idx == runColorIndex && runLength < 255) { runLength++; }
            else { bmpimg.data.push_back(CompressedBmpPixel{static_cast<uint8_t>(runLength), static_cast<uint8_t>(runColorIndex)}); runLength = 1; runColorIndex = idx; }
        }
        if (runLength > 0) bmpimg.data.push_back(CompressedBmpPixel{static_cast<uint8_t>(runLength), static_cast<uint8_t>(runColorIndex)});
        bmpimg.data.push_back(CompressedBmpPixel{0,0});
    }
    bmpimg.data.push_back(CompressedBmpPixel{0,1});
    return bmpimg;
}

Image<RGBA16BitPixel> loadImageBmp(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return {16,16,65535,BMP,4};
    char magic[2]; int filesize; uint16_t reserved1; uint16_t reserved2; int offbits; int dibsize; int width; int height; uint16_t planes; uint16_t bpp; int compression; int imagesize; int xppm; int yppm; int colorsused; int importantcolors;
    file.read(magic,2); file.read(reinterpret_cast<char*>(&filesize),4); file.read(reinterpret_cast<char*>(&reserved1),2); file.read(reinterpret_cast<char*>(&reserved2),2); file.read(reinterpret_cast<char*>(&offbits),4); file.read(reinterpret_cast<char*>(&dibsize),4); file.read(reinterpret_cast<char*>(&width),4); file.read(reinterpret_cast<char*>(&height),4); file.read(reinterpret_cast<char*>(&planes),2); file.read(reinterpret_cast<char*>(&bpp),2); file.read(reinterpret_cast<char*>(&compression),4); file.read(reinterpret_cast<char*>(&imagesize),4); file.read(reinterpret_cast<char*>(&xppm),4); file.read(reinterpret_cast<char*>(&yppm),4); file.read(reinterpret_cast<char*>(&colorsused),4); file.read(reinterpret_cast<char*>(&importantcolors),4);
    if (magic[0] != 'B' || magic[1] != 'M') return {16,16,65535,BMP,4}; if (colorsused == 0) colorsused = 256;
    CompressedBmp bmpimg{compression, std::vector<RGB8BitPixel>(colorsused), {}};
    if (compression == 1 && bpp == 8) { for (int i=0;i<colorsused;i++) { RGBA16BitPixel p; file.read(reinterpret_cast<char*>(&p.b),1); file.read(reinterpret_cast<char*>(&p.g),1); file.read(reinterpret_cast<char*>(&p.r),1); bmpimg.ColorPalette[i].r = p.r; bmpimg.ColorPalette[i].g = p.g; bmpimg.ColorPalette[i].b = p.b; } }
    int rowSize = ((bpp * width + 31) / 32) * 4; int padding = rowSize - (width * (bpp / 8)); file.seekg(offbits); Image<RGBA16BitPixel> img{width,height,65535,BMP,4};
    if (compression == 0) { for (int y=height-1;y>=0;y--) { for (int x=0;x<width;x++) { uint8_t b,g,r,a=255; file.read(reinterpret_cast<char*>(&b),1); file.read(reinterpret_cast<char*>(&g),1); file.read(reinterpret_cast<char*>(&r),1); if (bpp==32) file.read(reinterpret_cast<char*>(&a),1); img.Set(XY{x,y}, RGBA16BitPixel{ scale8ToMax(r,65535), scale8ToMax(g,65535), scale8ToMax(b,65535), scale8ToMax(a,65535) }); } file.ignore(padding); } }
    if (compression == 1) { for (int y=height-1;y>=0;y--) { for (int x=0;x<width;x++) { uint8_t runlength; uint8_t idx; file.read(reinterpret_cast<char*>(&runlength),1); file.read(reinterpret_cast<char*>(&idx),1); if (runlength==0 && idx==1) { y=-1; break; } if (runlength==0 && idx==0) { x=-1; continue; } RGB8BitPixel p = bmpimg.ColorPalette[idx]; RGBA16BitPixel pixel = toRGBA16Bit(p); for (int i=0;i<runlength;i++) img.Set(XY{x+i,y}, pixel); } } }
    return img;
}

template<typename Pixel>
int writeImageBmp(std::string path, Image<Pixel>& img, int bpp, int compression)
{
    std::ofstream file(path, std::ios::binary); if (!file.is_open()) return 1; CompressedBmp bmpimg; int colorsused = 0; int padding = 0;
    if (compression == BITMAP_RLE8) { if (bpp != 8) return 1; bmpimg = compressBmp(img, compression); colorsused = static_cast<int>(bmpimg.ColorPalette.size()); }
    else { if (bpp <= 8) colorsused = 1 << bpp; else colorsused = 0; }
    int offbits = 14 + 40; if (bpp <= 8) offbits += colorsused * 4;
    int imagesize = 0; if (compression == BITMAP_RLE8) imagesize = static_cast<int>(bmpimg.data.size()) * 2; else { int rowSize = ((bpp * img.WIDTH + 31) / 32) * 4; padding = rowSize - (img.WIDTH * (bpp / 8)); imagesize = rowSize * img.HEIGHT; }
    int filesize = offbits + imagesize; int dibsize = 40; int planes = 1;
    file.write("BM",2); file.write(reinterpret_cast<const char*>(&filesize),4); uint16_t zero16 = 0; file.write(reinterpret_cast<const char*>(&zero16),2); file.write(reinterpret_cast<const char*>(&zero16),2); file.write(reinterpret_cast<const char*>(&offbits),4); file.write(reinterpret_cast<const char*>(&dibsize),4); file.write(reinterpret_cast<const char*>(&img.WIDTH),4); file.write(reinterpret_cast<const char*>(&img.HEIGHT),4); file.write(reinterpret_cast<const char*>(&planes),2); file.write(reinterpret_cast<const char*>(&bpp),2); file.write(reinterpret_cast<const char*>(&compression),4); file.write(reinterpret_cast<const char*>(&imagesize),4); int zero32 = 0; file.write(reinterpret_cast<const char*>(&zero32),4); file.write(reinterpret_cast<const char*>(&zero32),4); file.write(reinterpret_cast<const char*>(&colorsused),4); file.write(reinterpret_cast<const char*>(&zero32),4);
    if (compression == BITMAP_RLE8) { for (RGB8BitPixel pixel : bmpimg.ColorPalette) { uint8_t r = pixel.r; uint8_t g = pixel.g; uint8_t b = pixel.b; file.write(reinterpret_cast<const char*>(&b),1); file.write(reinterpret_cast<const char*>(&g),1); file.write(reinterpret_cast<const char*>(&r),1); uint8_t zero = 0; file.write(reinterpret_cast<const char*>(&zero),1); } for (CompressedBmpPixel p : bmpimg.data) { file.write(reinterpret_cast<const char*>(&p.runlength),1); file.write(reinterpret_cast<const char*>(&p.index),1); } return 0; }
    for (int y = img.HEIGHT - 1; y >= 0; y--) { for (int x=0;x<img.WIDTH;x++) { RGBA8BitPixel pixel = toRGBA8Bit(img.Get(XY{x,y})); uint8_t r = pixel.r; uint8_t g = pixel.g; uint8_t b = pixel.b; file.write(reinterpret_cast<const char*>(&b),1); file.write(reinterpret_cast<const char*>(&g),1); file.write(reinterpret_cast<const char*>(&r),1); if (bpp == 32) { uint8_t a = pixel.a; file.write(reinterpret_cast<const char*>(&a),1); } } for (int p=0;p<padding;p++) file.put(0); }
    return 0;
}

// ---------------- Operations ----------------

std::vector<XY> findNeighborIndices(int kernel_size, XY center)
{
    std::vector<XY> neighbors; int r = kernel_size / 2; for (int dy=-r; dy<=r; dy++) for (int dx=-r; dx<=r; dx++) neighbors.push_back({center.x + dx, center.y + dy}); return neighbors;
}

template<typename Pixel>
Image<Pixel> Blur(Image<Pixel>& src_img, int passes = 5, int kernel_size = 3)
{
    Image<Pixel> new_img = src_img; uint32_t average_r, average_g, average_b, average_a; int num_neighbors; std::vector<XY> neighbors; RGBA16BitPixel blurred_pixel;
    for (int pass=0; pass<passes; pass++) {
        #pragma omp parallel for private(average_r, average_g, average_b, average_a, num_neighbors, neighbors, blurred_pixel) schedule(dynamic)
        for (int y=0;y<src_img.HEIGHT;y++) {
            for (int x=0;x<src_img.WIDTH;x++) {
                average_r = average_g = average_b = average_a = 0; num_neighbors = 0; neighbors = findNeighborIndices(kernel_size, {x,y});
                for (const auto& nb : neighbors) { if (nb.x < 0 || nb.x >= src_img.WIDTH || nb.y < 0 || nb.y >= src_img.HEIGHT) continue; RGBA16BitPixel p = toRGBA16Bit(src_img.Get(XY{nb.x, nb.y})); average_r += p.r; average_g += p.g; average_b += p.b; average_a += p.a; num_neighbors++; }
                average_r /= num_neighbors; average_g /= num_neighbors; average_b /= num_neighbors; average_a /= num_neighbors;
                blurred_pixel = { Clamp(average_r, 65035), Clamp(average_g, 65035), Clamp(average_b, 65035), Clamp(average_a, 65035) };
                new_img.Set(XY{x,y}, convertPixel<Pixel, RGBA16BitPixel>(blurred_pixel));
            }
        }
        src_img = new_img;
    }
    return new_img;
}

template<typename Pixel>
Image<Pixel> Grayscale(Image<Pixel>& src_img, int method = GRAYSCALE_AVERAGE, std::vector<float> weights = {0.21f, 0.72f, 0.07f})
{
    if (method != GRAYSCALE_AVERAGE && method != GRAYSCALE_LUMINOSITY) throw std::invalid_argument("Invalid grayscale method"); Image<Pixel> new_img = src_img;
    if (method == GRAYSCALE_AVERAGE) { for (int y=0;y<src_img.HEIGHT;y++) for (int x=0;x<src_img.WIDTH;x++) { auto p = toRGB16Bit(src_img.Get(XY{x,y})); uint16_t gray = static_cast<uint16_t>((p.r + p.g + p.b)/3); new_img.Set(XY{x,y}, convertPixel<Pixel, Gray16BitPixel>(Gray16BitPixel{gray})); } }
    else { for (int y=0;y<src_img.HEIGHT;y++) for (int x=0;x<src_img.WIDTH;x++) { auto p = toRGB16Bit(src_img.Get(XY{x,y})); uint16_t gray = static_cast<uint16_t>(p.r * weights[0] + p.g * weights[1] + p.b * weights[2]); new_img.Set(XY{x,y}, convertPixel<Pixel, Gray16BitPixel>(Gray16BitPixel{gray})); } }
    return new_img;
}

template<typename Pixel>
Image<Pixel> Bw(Image<Pixel>& src_img, uint16_t threshold = 0)
{
    if (threshold == 0) threshold = src_img.MAXVAL / 2; Image<Pixel> new_img = src_img; for (int y=0;y<src_img.HEIGHT;y++) for (int x=0;x<src_img.WIDTH;x++) new_img.Set(XY{x,y}, convertPixel<Pixel, BWPixel>(toBW(src_img.Get(XY{x,y})))); return new_img;
}

template<typename Pixel>
bool isGrayscaleOrBw(Image<Pixel>& img) { for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { auto p = toRGB8Bit(img.Get(XY{x,y})); if (p.r != p.g || p.g != p.b) return false; } return true; }

template<typename Pixel>
Image<Pixel> invertColor(Image<Pixel>& src_img)
{
    Image<Pixel> out = src_img;
    for (int y=0;y<src_img.HEIGHT;y++) for (int x=0;x<src_img.WIDTH;x++) { auto p = toRGBA16Bit(src_img.Get(XY{x,y})); int inv_r = static_cast<int>(65535) - static_cast<int>(p.r); int inv_g = static_cast<int>(65535) - static_cast<int>(p.g); int inv_b = static_cast<int>(65535) - static_cast<int>(p.b); int inv_a = static_cast<int>(65535) - static_cast<int>(p.a); RGBA16BitPixel inv = { Clamp(inv_r,65535), Clamp(inv_g,65535), Clamp(inv_b,65535), Clamp(inv_a,65535) }; out.Set(XY{x,y}, convertPixel<Pixel, RGBA16BitPixel>(inv)); }
    return out;
}

template<typename Pixel>
Image<Pixel> flipVertical(Image<Pixel>& src_img) { Image<Pixel> out = src_img; for (int y=0;y<src_img.HEIGHT;y++) for (int x=0;x<src_img.WIDTH;x++) out.Set(XY{x, src_img.HEIGHT - 1 - y}, src_img.Get(XY{x,y})); return out; }

template<typename Pixel>
Image<Pixel> flipHorizontal(Image<Pixel>& src_img) { Image<Pixel> out = src_img; for (int y=0;y<src_img.HEIGHT;y++) for (int x=0;x<src_img.WIDTH;x++) out.Set(XY{src_img.WIDTH - 1 - x, y}, src_img.Get(XY{x,y})); return out; }

template<typename Pixel>
Image<Pixel> Bloom(Image<Pixel>& img, int brightnessThreshold = 32768, int radius = 13, int intensity = 1)
{
    Image<Pixel> brightImage = img;
    for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { RGBA16BitPixel p = toRGBA16Bit(img.Get(XY{x,y})); bool isBright = (brightnessThreshold < (p.r + p.g + p.b) / 3 ); if (isBright) brightImage.Set(XY{x,y}, img.Get(XY{x,y})); else brightImage.Set(XY{x,y}, convertPixel<Pixel, RGBA16BitPixel>(RGBA16BitPixel{0,0,0,65535})); }
    Image<Pixel> blurred = Blur(brightImage, 1, radius);
    Image<Pixel> finalImage = img; brightImage.DeleteImageBuffer();
    for (int y=0;y<img.HEIGHT;y++) for (int x=0;x<img.WIDTH;x++) { RGBA16BitPixel original = toRGBA16Bit(img.Get(XY{x,y})); RGBA16BitPixel bloom = toRGBA16Bit(blurred.Get(XY{x,y})); uint16_t r = Clamp(static_cast<int>(original.r) + static_cast<int>(bloom.r) * intensity, 65535); uint16_t g = Clamp(static_cast<int>(original.g) + static_cast<int>(bloom.g) * intensity, 65535); uint16_t b = Clamp(static_cast<int>(original.b) + static_cast<int>(bloom.b) * intensity, 65535); uint16_t a = Clamp(static_cast<int>(original.a) + static_cast<int>(bloom.a) * intensity, 65535); finalImage.Set(XY{x,y}, convertPixel<Pixel, RGBA16BitPixel>(RGBA16BitPixel{r,g,b,a})); }
    return finalImage;
}

#endif // IMAGEENGINEER_H
