// image_io_bmp.h

#ifndef IMAGE_IO_BMP_H
#define IMAGE_IO_BMP_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "image.h"

/**
 * @file image_io_bmp.h
 * @brief BMP image load/save support including simple RLE8 compression helper.
 */

/**
 * @brief One run-length encoded pixel entry for BMP RLE8 compression.
 */
struct CompressedBmpPixel
{
    uint8_t runlength;
    uint8_t index;
};

/**
 * @brief Result of compressBmp(): compression metadata, palette and data.
 */
struct CompressedBmp
{
    int compression;

    std::vector<RGB8BitPixel> ColorPalette;

    std::vector<CompressedBmpPixel> data;
};

/**
 * @brief Return true when two 8-bit RGB pixels differ by at most
 * `threshold` per channel.
 */
bool PixelIsSimilar(RGB8BitPixel px /*Pixel*/, RGB8BitPixel dpx /*Distance Pixel*/, int threshold)
{
    return (abs(int(px.r) - int(dpx.r)) <= threshold) &&
           (abs(int(px.g) - int(dpx.g)) <= threshold) &&
           (abs(int(px.b) - int(dpx.b)) <= threshold);
}

/**
 * @brief Find a close color in `colorPalette` or add `px` if room exists.
 *
 * If the palette is full (256 entries) the closest existing color index is
 * returned. `threshold` controls when a color is considered "similar".
 */
int findOrAddColorToPalette(RGB8BitPixel px, std::vector<RGB8BitPixel>& colorPalette, int threshold)
{
    // Try to reuse existing colors if similar enough
    for (size_t i = 0; i < colorPalette.size(); i++)
    {
        if (PixelIsSimilar(px, colorPalette[i], threshold))
            return static_cast<int>(i);
    }

    // Add new color if palette not full
    if (colorPalette.size() < 256)
    {
        colorPalette.push_back(px);
        return static_cast<int>(colorPalette.size()) - 1; // Return new index
    }

    // Palette full, find closest color index to reuse
    int BestIndex = 0;
    int BestDistance = 255 * 3 + 1; // Max possible distance + 1
    for (size_t i = 0; i < colorPalette.size(); i++)
    {
        int dr = int(px.r) - int(colorPalette[i].r);
        int dg = int(px.g) - int(colorPalette[i].g);
        int db = int(px.b) - int(colorPalette[i].b);

        int Distance = abs(dr) + abs(dg) + abs(db);
        if (Distance < BestDistance)
        {
            BestIndex = static_cast<int>(i);
            BestDistance = Distance;
        }
    }
    return BestIndex;
}


// I HATE YOU BMP
/**
 * @brief Compress an image using a simple BMP RLE8-like scheme.
 *
 * Produces a `CompressedBmp` containing a small palette and run-length
 * encoded pixel pairs. This is an internal helper used before writing
 * RLE-compressed BMP files.
 */
template<typename Pixel>
CompressedBmp compressBmp(Image<Pixel>& img, int compression = BITMAP_RLE8)
{
    CompressedBmp bmpimg{compression};

    for (int y = img.HEIGHT - 1; y >= 0; y--)
    {
        int runLength = 0;
        int runColorIndex;

        for (int x = 0; x < img.WIDTH; x++)
        {
            RGB8BitPixel px = toRGB8Bit(img.Get(XY{x, y}));
            int idx = findOrAddColorToPalette(px, bmpimg.ColorPalette, 10);

            if (runLength == 0)
            {
                runLength = 1;
                runColorIndex = idx;
            }
            else if (idx == runColorIndex && runLength < 255)
            {
                runLength++;
            }
            else
            {
                bmpimg.data.push_back(CompressedBmpPixel{
                    static_cast<uint8_t>(runLength),
                    static_cast<uint8_t>(runColorIndex)
                });
                runLength = 1;
                runColorIndex = idx;
            }
        }

        if (runLength > 0)
        {
            bmpimg.data.push_back(CompressedBmpPixel{
                static_cast<uint8_t>(runLength),
                static_cast<uint8_t>(runColorIndex)
            });
        }

        bmpimg.data.push_back(CompressedBmpPixel{0, 0});
    }
    bmpimg.data.push_back(CompressedBmpPixel{0, 1});
    return bmpimg;
}

/**
 * @brief Load a BMP file from disk into an `Image<RGBA16BitPixel>`.
 *
 * Supports a subset of BMP variants including uncompressed 24/32-bit and
 * RLE8-compressed 8-bit indexed images. This is a best-effort reader focused
 * on compatibility with the project's needs.
 */
Image<RGBA16BitPixel> loadImageBmp(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return {16, 16, 65535, BMP, 4};

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

    if (magic[0] != 'B' || magic[1] != 'M') return {16, 16, 65535, BMP, 4};
    if (colorsused == 0) colorsused = 256;

    CompressedBmp bmpimg{compression, std::vector<RGB8BitPixel>(colorsused), {}};

    if (compression == 1 && bpp == 8)
    {
        for (int i = 0; i < colorsused; i++)
        {
            RGBA16BitPixel p;
            file.read(reinterpret_cast<char*>(&p.b), 1);
            file.read(reinterpret_cast<char*>(&p.g), 1);
            file.read(reinterpret_cast<char*>(&p.r), 1);
            bmpimg.ColorPalette[i].r = p.r;
            bmpimg.ColorPalette[i].g = p.g;
            bmpimg.ColorPalette[i].b = p.b;
        }
    }

    int rowSize = ((bpp * width + 31) / 32) * 4;
    int padding = rowSize - (width * (bpp / 8));

    file.seekg(offbits);

    Image<RGBA16BitPixel> img{width, height, 65535, BMP, 4};

    if (compression == 0)
    {
        for (int y = height - 1; y >= 0; y--)
        {
            for (int x = 0; x < width; x++)
            {
                uint8_t b, g, r, a = 255;
                file.read(reinterpret_cast<char*>(&b), 1);
                file.read(reinterpret_cast<char*>(&g), 1);
                file.read(reinterpret_cast<char*>(&r), 1);

                if (bpp == 32)
                    file.read(reinterpret_cast<char*>(&a), 1);

                img.Set(XY{x, y}, RGBA16BitPixel{
                    scale8ToMax(r, 65535),
                    scale8ToMax(g, 65535),
                    scale8ToMax(b, 65535),
                    scale8ToMax(a, 65535)
                });
            }
            file.ignore(padding);
        }
    }
    if (compression == 1)
    {
        for (int y = height - 1; y >= 0; y--)
        {
            for (int x = 0; x < width; x++)
            {
                uint8_t idx;
                uint8_t runlength;
                file.read(reinterpret_cast<char*>(&runlength), 1);
                file.read(reinterpret_cast<char*>(&idx), 1);

                if (runlength == 0 && idx == 1) { y = -1; break; }  // End of bitmap
                if (runlength == 0 && idx == 0) // End of line
                {
                    x = -1; // will become 0 at next iteration
                    continue;
                }
                RGB8BitPixel p = bmpimg.ColorPalette[idx];
                RGBA16BitPixel pixel = toRGBA16Bit(p);

                for (int i = 0; i < runlength; i++)
                {
                    img.Set(XY{x + i, y}, pixel);
                }
            }
        }
    }
    return img;
}

/**
 * @brief Write an image to a BMP file.
 *
 * `bpp` selects bits-per-pixel (8/24/32) and `compression` selects BMP
 * compression (0 = none, BITMAP_RLE8 = RLE8). Returns 0 on success.
 */
template<typename Pixel>
int writeImageBmp(
    std::string path,
    Image<Pixel>& img, 
    int bpp, 
    int compression
)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return 1;

    CompressedBmp bmpimg;
    int colorsused = 0;
    int padding = 0;

    // --- Handle compression first ---
    if (compression == BITMAP_RLE8)
    {
        // RLE8 requires 8-bit indexed color
        if (bpp != 8)
            return 1; // invalid combination

        bmpimg = compressBmp(img, compression);
        colorsused = static_cast<int>(bmpimg.ColorPalette.size());
    }
    else
    {
        // Only indexed formats use a palette
        if (bpp <= 8)
            colorsused = 1 << bpp;   // e.g. 256 for 8-bit
        else
            colorsused = 0;
    }

    // --- Calculate pixel data offset ---
    int offbits = 14 + 40;  // FILEHEADER + INFOHEADER

    if (bpp <= 8)
        offbits += colorsused * 4;  // palette entries are 4 bytes each


    // --- Calculate image size ---
    int imagesize = 0;

    if (compression == BITMAP_RLE8)
    {
        imagesize = static_cast<int>(bmpimg.data.size()) * 2;
    }
    else
    {
        int rowSize = ((bpp * img.WIDTH + 31) / 32) * 4;
        padding = rowSize - (img.WIDTH * (bpp / 8));
        imagesize = rowSize * img.HEIGHT;
    }


    // --- Calculate file size ---
    int filesize = offbits + imagesize;


    // --- Standard header values ---
    int dibsize = 40;
    int planes = 1;

    file.write("BM", 2);
    file.write(reinterpret_cast<const char*>(&filesize), 4);

    uint8_t zero = 0;
    uint16_t zero16 = 0;
    file.write(reinterpret_cast<const char*>(&zero16), 2);
    file.write(reinterpret_cast<const char*>(&zero16), 2);

    file.write(reinterpret_cast<const char*>(&offbits), 4);
    file.write(reinterpret_cast<const char*>(&dibsize), 4);
    file.write(reinterpret_cast<const char*>(&img.WIDTH), 4);
    file.write(reinterpret_cast<const char*>(&img.HEIGHT), 4);
    file.write(reinterpret_cast<const char*>(&planes), 2);
    file.write(reinterpret_cast<const char*>(&bpp), 2);
    file.write(reinterpret_cast<const char*>(&compression), 4);
    file.write(reinterpret_cast<const char*>(&imagesize), 4);

    int zero32 = 0;
    file.write(reinterpret_cast<const char*>(&zero32), 4);
    file.write(reinterpret_cast<const char*>(&zero32), 4);
    file.write(reinterpret_cast<const char*>(&colorsused), 4);
    file.write(reinterpret_cast<const char*>(&zero32), 4);


    // I have lost my mind
    if (compression == BITMAP_RLE8)
    {
        for (RGB8BitPixel pixel : bmpimg.ColorPalette)
        {
            uint8_t r = pixel.r;
            uint8_t g = pixel.g;
            uint8_t b = pixel.b;

            file.write(reinterpret_cast<const char*>(&b), 1);
            file.write(reinterpret_cast<const char*>(&g), 1);
            file.write(reinterpret_cast<const char*>(&r), 1);
            file.write(reinterpret_cast<const char*>(&zero), 1);
        }

        for (CompressedBmpPixel p : bmpimg.data)
        {
            file.write(reinterpret_cast<const char*>(&p.runlength), 1);
            file.write(reinterpret_cast<const char*>(&p.index), 1);
        }

        return 0;
    }

    for (int y = img.HEIGHT - 1; y >= 0; y--)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            RGBA8BitPixel pixel = toRGBA8Bit(img.Get(XY{x, y}));

            uint8_t r = pixel.r;
            uint8_t g = pixel.g;
            uint8_t b = pixel.b;

            file.write(reinterpret_cast<const char*>(&b), 1);
            file.write(reinterpret_cast<const char*>(&g), 1);
            file.write(reinterpret_cast<const char*>(&r), 1);

            if (bpp == 32)
            {
                uint8_t a = pixel.a;
                file.write(reinterpret_cast<const char*>(&a), 1);
            }
        }
        for (int p = 0; p < padding; p++) file.put(0);
    }

    return 0;
}


#endif