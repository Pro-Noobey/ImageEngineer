// image_io_bmp.h
// Cannot write/read compression = BMP_NONE && bpp != 32 bpp != 24
// Meaning custom bpp (Indexed Images) with BMP_NONE compression is not supported

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

using std::string_literals::operator""s;
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

RGB8BitPixel avgRGB8Bit(RGB8BitPixel px1, RGB8BitPixel px2)
{
    // Total weight = 1 + 3 = 4
    return RGB8BitPixel{
        static_cast<uint8_t>((static_cast<int>(px1.r) + static_cast<int>(px2.r) * 3) / 4),
        static_cast<uint8_t>((static_cast<int>(px1.g) + static_cast<int>(px2.g) * 3) / 4),
        static_cast<uint8_t>((static_cast<int>(px1.b) + static_cast<int>(px2.b) * 3) / 4)
    };
}

/*
/**
 * @brief Find a close color in `colorPalette` or add `px` if room exists.
 *
 * If the palette is full (256 entries) the closest existing color index is
 * returned. `threshold` controls when a color is considered "similar".
 */

int findOrAddColorToPalette(RGB8BitPixel px, std::vector<RGB8BitPixel>& colorPalette, int threshold, int maxPaletteSize = 256, int absDistance = 56)
{
    // Try to reuse existing colors if similar enough
    for (size_t i = 0; i < colorPalette.size(); i++)
    {
        if (PixelIsSimilar(px, colorPalette[i], threshold))
            return static_cast<int>(i);
    }

    // Add new color if palette not full
    if (colorPalette.size() < maxPaletteSize)
    {
        colorPalette.push_back(px);
        return static_cast<int>(colorPalette.size()) - 1; // Return new index
    }

    // Palette full, find closest color index to reuse
    int BestIndex = 0;
    int BestDistance = (maxPaletteSize - 1) * 3 + 1; // Max possible distance + 1
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

    if (BestDistance >= absDistance)
    {
        colorPalette[BestIndex] = avgRGB8Bit(colorPalette[BestIndex], px); // Average the colors to reduce distance

        return BestIndex;
    }

    return BestIndex;
}


/**
 * @brief Compress an image using a simple BMP RLE8-like scheme.
 *
 * Produces a `CompressedBmp` containing a small palette and run-length
 * encoded pixel pairs. This is an internal helper used before writing
 * RLE-compressed BMP files.
 */
/*
template<typename Pixel, typename FormatDataType = EmptyFormatData>
CompressedBmp compressBmp(Image<Pixel, FormatDataType>& img, int compression = BITMAP_RLE8)
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
*/

template<typename Pixel, typename FormatDataType = EmptyFormatData>
CompressedBmp compressBmp(Image<Pixel, FormatDataType>& img, int compression = BITMAP_RLE8)
{
    CompressedBmp bmpimg{compression};

    bmpimg.ColorPalette.reserve(256); // Reserve space for 256 colors

    for (int y = img.HEIGHT - 1; y >= 0; y--)
    {
        int runLength = 0;
        int runColorIndex;

        for (int x = 0; x < img.WIDTH; x++)
        {
            RGB8BitPixel px = toRGB8Bit(img.Get(XY{x, y}));
            int idx = findOrAddColorToPalette(px, bmpimg.ColorPalette, 16, 256, 48);

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

    return bmpimg;
}


/**
 * @brief Extract a single channel from a bitfield and scale to maxVal.
 *
 * @param value The raw pixel value (16 or 32-bit).
 * @param mask  The channel mask (e.g., rmask, gmask, bmask, amask).
 * @param maxVal Maximum output value (65535 for RGBA16BitPixel).
 * @return uint16_t Scaled channel value.
 */
inline uint16_t extractChannel(uint32_t value, uint32_t mask, uint16_t maxVal = 65535)
{
    if (mask == 0) return maxVal; // treat missing alpha as fully opaque

    // shift the bits down until LSB
    int shift = 0;
    uint32_t temp = mask;
    while ((temp & 1) == 0) { temp >>= 1; shift++; }

    // extract masked bits
    uint32_t channel = (value & mask) >> shift;

    // calculate number of bits used in mask
    int maskBits = 0;
    temp = mask >> shift;
    while (temp) { maskBits++; temp >>= 1; }

    // scale channel to maxVal
    return static_cast<uint16_t>((channel * maxVal) / ((1u << maskBits) - 1));
}


/**
 * @brief Load a BMP file from disk into an `Image<RGBA16BitPixel>`.
 *
 * Supports a subset of BMP variants including uncompressed 24/32-bit and
 * RLE8-compressed 8-bit indexed images. This is a best-effort reader focused
 * on compatibility with the project's needs.
 */
Image<RGBA16BitPixel, BmpFormatData> loadImageBmp(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("File did not open. Path: "s + path + "\n"s);
    }

    BmpFormatData formatData;

    // Read file header
    file.read(reinterpret_cast<char*>(&formatData.fileHeader), sizeof(BMPFileHeader));

    // Read DIB size
    uint32_t dibSize;
    file.read(reinterpret_cast<char*>(&dibSize), 4);
    file.seekg(-4, std::ios::cur);

    // Read full DIB
    std::vector<char> dibBuffer(dibSize);
    file.read(dibBuffer.data(), dibSize);

    // Interpret
    auto* info = reinterpret_cast<BMPInfoHeader*>(dibBuffer.data());
    auto* v3   = reinterpret_cast<BMPV3InfoHeader*>(dibBuffer.data() + sizeof(BMPInfoHeader));

    int32_t width        = info->biWidth;
    int32_t height       = info->biHeight;
    uint32_t bpp         = info->biBitCount;
    uint32_t compression = info->biCompression;
    uint32_t coloursused = info->biClrUsed;

    // Masks
    uint32_t rmask = 0, gmask = 0, bmask = 0, amask = 0;

    if (dibSize >= 56)
    {
        rmask = v3->bV4RedMask;
        gmask = v3->bV4GreenMask;
        bmask = v3->bV4BlueMask;
        amask = v3->bV4AlphaMask;
    }
    else if (compression == BITMAP_BITFIELDS)
    {
        file.read(reinterpret_cast<char*>(&rmask), 4);
        file.read(reinterpret_cast<char*>(&gmask), 4);
        file.read(reinterpret_cast<char*>(&bmask), 4);

        if (bpp == 32)
            file.read(reinterpret_cast<char*>(&amask), 4);
    }

    if (amask == 0)
        amask = 0xFF000000;

    // Validate magic
    if (formatData.fileHeader.bfType != 0x4D42)
        throw std::runtime_error("Not a valid BMP file: "s + path + "\n"s);

    bool topDown = height < 0;
    height = abs(height);

    if (coloursused == 0 && bpp <= 8)
    {
        coloursused = 1 << bpp;
    }

    CompressedBmp bmpimg{compression, std::vector<RGB8BitPixel>(coloursused), {}};

    if (compression == 1 && bpp == 8)
    {
        for (int i = 0; i < coloursused; i++)
        {
            RGBA16BitPixel p;
            uint8_t reserved;
            file.read(reinterpret_cast<char*>(&p.b), 1);
            file.read(reinterpret_cast<char*>(&p.g), 1);
            file.read(reinterpret_cast<char*>(&p.r), 1);
            file.read(reinterpret_cast<char*>(&reserved), 1); // reserved byte
            bmpimg.ColorPalette[i].r = p.r;
            bmpimg.ColorPalette[i].g = p.g;
            bmpimg.ColorPalette[i].b = p.b;
        }
    }

    int rowSize = ((bpp * width + 31) / 32) * 4;
    int padding = rowSize - (width * (bpp / 8));

    file.seekg(formatData.fileHeader.bfOffBits);

    Image<RGBA16BitPixel, BmpFormatData> img{width, height, 65535, BMP, 4, formatData};

    if (compression == BITMAP_NONE)
    {
        for (int y = (topDown) ? 0 : height - 1; (topDown) ? y < height : y >= 0; (topDown) ? y++ : y--)
        {
            for (int x = 0; x < width; x++)
            {

                uint8_t a = 255;
                
                uint8_t b, g, r;
                file.read(reinterpret_cast<char*>(&b), 1);
                file.read(reinterpret_cast<char*>(&g), 1);
                file.read(reinterpret_cast<char*>(&r), 1);

                if (bpp == 32)
                {
                    uint8_t throwaway;
                    file.read(reinterpret_cast<char*>(&throwaway), 1);
                }

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

    if (compression == BITMAP_BITFIELDS)
    {
        /*
        `int y = (topDown) ? 0 : height - 1;`
        If order is top to down then set y to 0 else
        if it is bottom to up, set it to `height - 1`.

        `(topDown) ? y < height : y >= 0;`
        If order is top to down then check if 
        `y < height` else if bottom to up,
        check if `y >= 0`.

        `(topDown) ? y++ : y--`
        If is top to down, then `y++` else `y--`.
        */
        for (int y = (topDown) ? 0 : height - 1; (topDown) ? y < height : y >= 0; (topDown) ? y++ : y--)
        {
            for (int x = 0; x < width; x++)
            {
                uint32_t rawPixel;

                if (bpp == 16)
                {
                    uint16_t p16;
                    file.read(reinterpret_cast<char*>(&p16), 2);
                    rawPixel = p16; // promote to 32
                }
                /*32 Bit case*/
                else
                {
                    file.read(reinterpret_cast<char*>(&rawPixel), 4);
                }

                RGBA16BitPixel p{
                    extractChannel(rawPixel, rmask),
                    extractChannel(rawPixel, gmask),
                    extractChannel(rawPixel, bmask),
                    extractChannel(rawPixel, amask) // 0 if no alpha
                };

                img.Set(XY{x, y}, p);
            }
            file.ignore(padding);
        }
    }

    if (compression == BITMAP_RLE8 && bpp == 8)
    {
        XY cursor = {0, (topDown) ? 0 : height - 1};

        while (true)
        {
            uint8_t idx;
            uint8_t runlength;
            file.read(reinterpret_cast<char*>(&runlength), 1);
            file.read(reinterpret_cast<char*>(&idx), 1);

            if (runlength == 0)
            {
                if (idx == 0) // End of line
                {
                    cursor.x = 0;
                    cursor.y += (topDown ? 1 : -1);
                    continue;
                }

                if (idx == 1)
                {
                    break; // End of bitmap
                }

                if (idx == 2) // Delta
                {
                    uint8_t dx, dy;
                    file.read(reinterpret_cast<char*>(&dx), 1);
                    file.read(reinterpret_cast<char*>(&dy), 1);
                    cursor.x += dx;
                    cursor.y += (topDown ? dy : -dy);
                    continue;
                }

                // Absolute mode: read idx pixels directly
                uint8_t padding = (idx & 1) ? 1 : 0; // pad to even byte count
                for (int i = 0; i < idx; i++)
                {
                    uint8_t colorIndex;
                    file.read(reinterpret_cast<char*>(&colorIndex), 1);
                    if (colorIndex >= coloursused)
                    {
                        throw std::runtime_error("Invalid color index in RLE data: "s + std::to_string(colorIndex) + "\n"s);
                    }
                    RGB8BitPixel p = bmpimg.ColorPalette[colorIndex];
                    RGBA16BitPixel pixel = toRGBA16Bit(p);
                    img.Set(XY{cursor.x + i, cursor.y}, pixel);
                }
                file.ignore(padding);
                cursor.x += idx;
                continue;
            }
            if (idx >= coloursused)
            {
                throw std::runtime_error("Invalid color index in RLE data: "s + std::to_string(idx) + "\n"s);
            }

            RGB8BitPixel p = bmpimg.ColorPalette[idx];
            RGBA16BitPixel pixel = toRGBA16Bit(p);

            for (int i = 0; i < runlength; i++)
            {
                img.Set(XY{cursor.x + i, cursor.y}, pixel);
            }
            cursor.x += runlength;
        }
    }
    return img;
}

/**
 * @brief Write an Image to disk as a BMP file.
 *
 * Writes the provided Image<Pixel> to a BMP file at the given path using the
 * specified bits-per-pixel (bpp) and compression mode. The function constructs
 * a minimal BMP file including a 14-byte BITMAPFILEHEADER and a 40-byte
 * BITMAPINFOHEADER, optional color palette for indexed formats, optional
 * color masks for 32-bit images, and the pixel or compressed pixel data.
 *
 * Behavior and notes:
 * - The file is opened with std::ofstream in binary mode. If the file cannot
 *   be opened the function returns 1.
 * - Supported common bpp values: indexed (<=8), 24 (RGB), 32 (RGBA). When
 *   compression == BITMAP_RLE8, only 8 bpp is valid; the function returns 1
 *   for invalid combinations.
 * - For indexed (bpp <= 8) non-RLE output, the palette size is computed as
 *   colorsused = 1 << bpp (e.g. 256 for 8-bit). For RLE8, the palette is taken
 *   from compressBmp(img, compression) and colorsused is set to the palette
 *   size in the returned CompressedBmp.
 * - Pixel data offset (bfOffBits) is computed as 14 + 40 + palette_size*4 when
 *   a palette is present.
 * - Image data size:
 *   - For RLE8: imagesize is taken from the returned compressed data length
 *     (the code writes compressed pairs of bytes, see below).
 *   - For uncompressed formats: imagesize = rowSize * height where
 *     rowSize = ((bpp * width + 31) / 32) * 4 (i.e. 4-byte aligned scanlines).
 *     padding = rowSize - (width * (bpp / 8)) bytes are appended per row.
 * - Header fields written:
 *   - "BM" signature, file size, bfReserved1/bfReserved2 (zeros), bfOffBits.
 *   - BITMAPINFOHEADER (biSize=40), image width, image height, planes=1,
 *     biBitCount=bpp, biCompression=compression, biSizeImage=imagesize,
 *     biXPelsPerMeter/biYPelsPerMeter (zeros), biClrUsed=colorsused,
 *     biClrImportant (zero).
 *   - Color masks are written as rmask=0x00FF0000, gmask=0x0000FF00,
 *     bmask=0x000000FF and amask=0xFF000000 only when bpp == 32.
 * - Palette and pixel/stream layout:
 *   - For indexed (palette) output the code writes palette entries in BMP
 *     order (B, G, R, 0) as 4 bytes per entry.
 *   - For compression == BITMAP_RLE8:
 *     - The function uses compressBmp(img, compression) to obtain a
 *       CompressedBmp containing ColorPalette and data (CompressedBmpPixel).
 *     - It writes the palette entries (B,G,R,0) then writes the compressed
 *       stream as pairs of bytes: (runlength, index) for each CompressedBmpPixel.
 *   - For uncompressed output:
 *     - Pixels are written bottom-up (rows from height-1 down to 0).
 *     - Each pixel is written in BGR order (1 byte each). For 32 bpp an alpha
 *       byte is also written per pixel (BGRA order on disk).
 *     - Each scanline is padded with zero bytes to the 4-byte aligned rowSize.
 *
 * Parameters:
 * - path: filesystem path to create/overwrite with the BMP file.  
 * - img: reference to the Image<Pixel> to be written. The function queries  
 *   img.WIDTH, img.HEIGHT and uses img.Get(XY{x,y}) to obtain pixel values.  
 * - bpp: bits per pixel to write (controls palette usage and per-pixel bytes).  
 * - compression: BMP compression method (e.g. BITMAP_RLE8 for RLE8; 0 for  
 *   BI_RGB/uncompressed).
 *
 * Return value:
 * - 0 on success (file written).
 * - 1 on failure (file open failure, invalid bpp/compression combination, or
 *   other early-detected error conditions).
 *
 * Remarks / Limitations:
 * - The function assumes little-endian architecture for directly writing
 *   integer header fields to disk in native form.
 * - Sizes and header fields are stored in int/uint32_t sized variables and
 *   may truncate very large images; callers should ensure image dimensions
 *   and bpp produce sizes within 32-bit limits for BMP.
 * - The function depends on external helpers/types: compressBmp,
 *   CompressedBmp, CompressedBmpPixel, RGB8BitPixel, RGBA8BitPixel,
 *   toRGBA8Bit, and Image<T>::Get which must supply the expected semantics.
 */
template<typename Pixel, typename FormatDataType = EmptyFormatData>
int writeImageBmp(
    std::string path,
    Image<Pixel, FormatDataType>& img, 
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

    if (compression == BITMAP_BITFIELDS)
        offbits += (bpp == 32) ? 16 : 12;

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

    if (compression == BITMAP_BITFIELDS)
    {
        uint32_t rmask = 0x00FF0000;
        uint32_t gmask = 0x0000FF00;
        uint32_t bmask = 0x000000FF;
        uint32_t amask = (bpp == 32) ? 0xFF000000 : 0;

        file.write(reinterpret_cast<const char*>(&rmask), 4);
        file.write(reinterpret_cast<const char*>(&gmask), 4);
        file.write(reinterpret_cast<const char*>(&bmask), 4);
        if (bpp == 32)
            file.write(reinterpret_cast<const char*>(&amask), 4);
    }

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

    if (compression == BITMAP_NONE)
    {
        for (int y = img.HEIGHT - 1; y >= 0; y--)
        {
            for (int x = 0; x < img.WIDTH; x++)
            {
                Pixel px = img.Get(XY{x, y});
                RGBA8BitPixel p = toRGBA8Bit(px);

                uint8_t r = p.r;
                uint8_t g = p.g;
                uint8_t b = p.b;

                file.write(reinterpret_cast<const char*>(&b), 1);
                file.write(reinterpret_cast<const char*>(&g), 1);
                file.write(reinterpret_cast<const char*>(&r), 1);

                if (bpp == 32)
                {
                    uint8_t full = 255;
                    file.write(reinterpret_cast<const char*>(&full), 1);
                }
            }
            for (int p = 0; p < padding; p++)
                file.write(reinterpret_cast<const char*>(&zero), 1);
        }
        return 0;
    }

    if (compression == BITMAP_BITFIELDS)
    {
        for (int y = img.HEIGHT - 1; y >= 0; y--)
        {
            for (int x = 0; x < img.WIDTH; x++)
            {
                Pixel px = img.Get(XY{x, y});
                RGBA8BitPixel p = toRGBA8Bit(px);

                uint8_t r = p.r;
                uint8_t g = p.g;
                uint8_t b = p.b;

                file.write(reinterpret_cast<const char*>(&b), 1);
                file.write(reinterpret_cast<const char*>(&g), 1);
                file.write(reinterpret_cast<const char*>(&r), 1);

                if (bpp == 32)
                {
                    uint8_t a = p.a;
                    file.write(reinterpret_cast<const char*>(&a), 1);
                }
            }
            for (int p = 0; p < padding; p++)
                file.write(reinterpret_cast<const char*>(&zero), 1);
        }
        return 0;
    }

    return 1; // Unsupported compression/bpp combination
}


#endif