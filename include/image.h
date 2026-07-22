// image.h

#pragma once
#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <cstdint>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

/**
 * @file image.h
 * @brief Core image pixel types, conversion utilities and the Image template.
 *
 * This header provides:
 * - Plain-old-data pixel structs (BW, Gray8/16, RGB8/16, RGBA8/16).
 * - Pixel conversion helpers (toBW, toRGB8Bit, toRGBA16Bit, etc.).
 * - Small utility functions for scaling/clamping values.
 * - The `Image<Pixel>` container class used to store pixel buffers.
 */

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

const int BITMAP_NONE = 0;
const int BITMAP_RLE8 = 1;
const int BITMAP_RLE4 = 2;
const int BITMAP_BITFIELDS = 3;

/**
 * @brief 8-bit grayscale pixel.
 *
 * Holds a single 8-bit luminance sample in `value` (0 = black, 255 = white).
 */
struct Gray8BitPixel
{
    uint8_t value = 0;
};

/**
 * @brief 16-bit grayscale pixel.
 *
 * Stores a single 16-bit luminance sample in `value` (0 = black, 65535 = white).
 */
struct Gray16BitPixel
{
    uint16_t value = 0;
};

/**
 * @brief Black-and-white (binary) pixel.
 *
 * `value` is true for white and false for black (or vice versa depending on
 * caller semantics). This type is used for PBM-style 1-bit images.
 */
struct BWPixel
{
    bool value = false;
};

/**
 * @brief 8-bit RGB pixel.
 *
 * Channels `r`, `g`, `b` range 0..255.
 */
struct RGB8BitPixel
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

/**
 * @brief 16-bit RGB pixel.
 *
 * Channels `r`, `g`, `b` range 0..65535.
 */
struct RGB16BitPixel
{
    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;
};

/**
 * @brief 8-bit RGBA pixel.
 *
 * Channels `r`, `g`, `b` range 0..255. `a` is the alpha channel (default 255).
 */
struct RGBA8BitPixel
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

/**
 * @brief 16-bit RGBA pixel.
 *
 * Channels `r`, `g`, `b`, `a` range 0..65535.
 */
struct RGBA16BitPixel
{
    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;
    uint16_t a = 65535;
};

/**
 * @brief Clamp integer `v` to the range [0, maxval] and return as uint16_t.
 *
 * @param v Value to clamp.
 * @param maxval Upper bound for the result.
 * @return uint16_t Clamped value.
 */
inline uint16_t Clamp(int v, int maxval)
{
    return static_cast<uint16_t>(v > maxval ? maxval : v);
}

/**
 * @brief Scale an 8-bit sample (0..255) to an arbitrary `maxval` range.
 *
 * Performs integer scaling equivalent to round-down((v/255.0) * maxval).
 */
inline uint16_t scale8ToMax(int v, int maxval)
{
    return static_cast<uint16_t>((v * maxval) / 255);
}

/**
 * @brief Scale a sample in range [0..maxval] down to 8-bit (0..255).
 */
inline uint8_t scaleMaxTo8(int v, int maxval)
{
    return static_cast<uint8_t>((v * 255) / maxval);
}

/**
 * @brief Convert an arbitrary pixel type to a binary `BWPixel`.
 *
 * Supports RGB(A) and Gray formats in both 8/16-bit variants. If the
 * provided pixel type is not supported a `std::runtime_error` is thrown.
 */
template<typename Pixel>
inline BWPixel toBW(Pixel p)
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

/**
 * @brief Convert a pixel to 16-bit grayscale (`Gray16BitPixel`).
 *
 * Handles RGB(A) (8/16-bit), Gray8/Gray16 and BW types.
 */
template<typename Pixel>
inline Gray16BitPixel toGray16Bit(Pixel p)
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

/**
 * @brief Convert a pixel to 8-bit RGB (`RGB8BitPixel`).
 *
 * Converts grayscale and 16-bit samples down to 8-bit where necessary.
 */
template<typename Pixel>
inline RGB8BitPixel toRGB8Bit(Pixel p)
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

/**
 * @brief Convert a pixel to 8-bit RGBA (`RGBA8BitPixel`).
 */
template<typename Pixel>
inline RGBA8BitPixel toRGBA8Bit(Pixel p)
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

/**
 * @brief Convert a pixel to 16-bit RGB (`RGB16BitPixel`).
 */
template<typename Pixel>
inline RGB16BitPixel toRGB16Bit(Pixel p)
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

/**
 * @brief Convert a pixel to 16-bit RGBA (`RGBA16BitPixel`).
 */
template<typename Pixel>
inline RGBA16BitPixel toRGBA16Bit(Pixel p)
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

/**
 * @brief Convert a pixel to 8-bit grayscale (`Gray8BitPixel`).
 */
template<typename Pixel>
inline Gray8BitPixel toGray8Bit(Pixel p)
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


/*
### IMPORTANT:
This enum's values are INTEGRAL,
meaning that THEY ARE NOT STRINGS.
They are just identifiers for the log level,
and do not have any inherent string representation. 
When logging messages,
you can use these enum values to determine how to format or color the output,
but you will need to convert them to strings manually if you want to include the log level in the message text.
*/
enum class ImageLoggerLevel : uint8_t
{
    NORMAL,
    INFO,
    WARNING,
    ERROR
};

struct MessageBlock
{
    std::string message;
    ImageLoggerLevel level;
    /*
    Recommended usage for coloring:
    NORMAL - Default terminal color/ White
    Info - Green
    Warning - Yellow
    Error - Red
    */
    RGB8BitPixel color;
};

std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

    #ifdef _WIN32
        localtime_s(&local_time, &t);
    #else
        localtime_r(&t, &local_time);
    #endif

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%b %d : %Hh : %Mm : %Ss");

    return oss.str();
}

std::string formatMessage(MessageBlock msg)
{
    std::string levelStr;
    switch (msg.level)
    {
        case ImageLoggerLevel::NORMAL: levelStr = "NORMAL"; break;
        case ImageLoggerLevel::INFO: levelStr = "INFO"; break;
        case ImageLoggerLevel::WARNING: levelStr = "WARNING"; break;
        case ImageLoggerLevel::ERROR: levelStr = "ERROR"; break;
    }
    
        auto now = std::chrono::system_clock::now();

        // Convert to time_t for calendar time
        auto t_c = std::chrono::system_clock::to_time_t(now);

        // Get milliseconds part
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

        // Format nicely
        // need to fix
        std::string timestamp = getTimestamp();

        /*
        ## IMPORTANT:  
        The color is applied using ANSI escape codes, which may not work in all terminal environments.
        #### The format is: \x1b[38;2;<r>;<g       
        */
        std::string color = "\x1b[38;2;" + 
                            std::to_string(msg.color.r) + ";" + 
                            std::to_string(msg.color.g) + ";" + 
                            std::to_string(msg.color.b) + "m";

    return "[" + timestamp + "][" + levelStr + "] " + color + msg.message + "\x1b[0m\n" /*Reset color*/;
}

class ImageLogger
{
private:
    std::vector<MessageBlock> messages;
public:
    void log
    (
        const std::string& message,
        ImageLoggerLevel level = ImageLoggerLevel::NORMAL,
        RGB8BitPixel color = RGB8BitPixel{255, 255, 255}
    )
    {
        messages.push_back({message, level, color});
    }

    void clear()
    {
        messages.clear();
    }

    MessageBlock getMessage(int index) const
    {
        if (index < 0 || index >= messages.size())
        {
            throw std::out_of_range("Index out of range in getMessage");
        }
        return messages[index];
    }

    std::vector<MessageBlock> getAllMessages() const
    {
        return messages;
    }

    void flush()
    {
        for (const auto& msg : messages)
        {  
            std::cout << formatMessage(msg);
        }
        messages.clear();
    }
};

struct EmptyFormatData {};

#pragma pack(push, 1)

struct CIEXYZ
{
    int32_t ciexyzX;
    int32_t ciexyzY;
    int32_t ciexyzZ;
};

struct CIEXYZTRIPLE
{
    CIEXYZ ciexyzRed;
    CIEXYZ ciexyzGreen;
    CIEXYZ ciexyzBlue;
};

struct BMPFileHeader
{
    uint16_t bfType;      // Signature: must be 'BM' (0x4D42)
    uint32_t bfSize;      // Size of the file in bytes
    uint16_t bfReserved1; // Reserved, must be 0
    uint16_t bfReserved2; // Reserved, must be 0
    uint32_t bfOffBits;   // Offset to the pixel array (start of bitmap data)
};

struct BMPInfoHeader
{
    uint32_t biSize;          // Size of this header (40 bytes for V1)
    int32_t  biWidth;         // Image width in pixels
    int32_t  biHeight;        // Image height in pixels (negative for top-down)
    uint16_t biPlanes;        // Must be 1
    uint16_t biBitCount;      // Bits per pixel: 1, 4, 8, 16, 24, 32
    uint32_t biCompression;   // Compression type (0=BI_RGB, 1=RLE8, 2=RLE4, 3=BITFIELDS)
    uint32_t biSizeImage;     // Image data size (may be 0 for uncompressed)
    int32_t  biXPelsPerMeter; // Horizontal resolution in pixels/meter (optional)
    int32_t  biYPelsPerMeter; // Vertical resolution in pixels/meter (optional)
    uint32_t biClrUsed;       // Number of colors used in palette (0 = default)
    uint32_t biClrImportant;  // Number of important colors (0 = all)
};

struct BMPV3InfoHeader 
{
    uint32_t bV4RedMask;
    uint32_t bV4GreenMask;
    uint32_t bV4BlueMask;
    uint32_t bV4AlphaMask; // optional
};

struct BMPV4Header
{
    uint32_t bV4CSType;    // Color space type
    CIEXYZTRIPLE bV4Endpoints; // CIEXYZTRIPLE endpoints (color space info)
    uint32_t bV4GammaRed;
    uint32_t bV4GammaGreen;
    uint32_t bV4GammaBlue;
};

struct BMPV5Header
{
    uint32_t bV5Intent;      // Rendering intent
    uint32_t bV5ProfileData; // Offset to ICC profile
    uint32_t bV5ProfileSize; // Size of ICC profile
    uint32_t bV5Reserved;    // Reserved
};

struct BmpFormatData
{
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    BMPV3InfoHeader v3Header;
    BMPV4Header v4Header;
    BMPV5Header v5Header;
};

#pragma pack(pop)

struct XY
{
    int x;
    int y;
};

/**
 * @brief Generic image container.
 *
 * `Image<Pixel>` stores a contiguous pixel buffer in row-major order and
 * exposes simple accessors. The `Pixel` type is one of the pixel structs
 * declared above (for example `RGB16BitPixel`).
 */
template<typename Pixel, typename FormatData = EmptyFormatData>
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
    FormatData FORMATDATA;
    /**
     * @brief Construct an Image with dimensions and metadata.
     *
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param maxval Maximum sample value (used by PNM-style formats).
     * @param type File/type identifier (one of the PBM/PGM/PPM/BMP constants).
     * @param channels Number of channels per pixel (default 3).
     */
    Image(int width, int height, int maxval, int type, int channels = 3, FormatData formatData = FormatData()):
        FORMATDATA(formatData),
        WIDTH(width),
        HEIGHT(height),
        MAXVAL(maxval),
        TYPE(type),
        CHANNELS(channels)
    {
        Init(); // allocate pixel buffer
    }
    
    /*
    Intializes PixelData size
    */
    /**
     * @brief Allocate or reallocate the internal pixel buffer.
     *
     * The buffer size is set to `WIDTH * HEIGHT`. Existing pixel values are
     * discarded when this is called.
     */
    void Init()
    {
        PixelData.resize(WIDTH * HEIGHT);
    }

    /*
    Unsafe way to get pixel values
    But FAST
    ------------------------------
    Does no bound checking
    */
    Pixel getPixelRaw(XY coord)
    {
        return PixelData[coord.y * WIDTH + coord.x];
    }

    /*
    Unsafe way to set pixel values
    But FAST
    ------------------------------
    Does no bound checking
    */
    void setPixelRaw(XY coord, Pixel value)
    {
        PixelData[coord.y * WIDTH + coord.x] = value;
    }

    /*
    Safe way to get pixel values
    But slow
    */
    /**
     * @brief Get a pixel at `coord` (bounds-checked).
     *
     * @throws std::out_of_range if coordinates are outside the image.
     */
    Pixel Get(XY coord) const
    {
        if (coord.x < 0 || coord.x >= WIDTH || coord.y < 0 || coord.y >= HEIGHT)
        {
            throw std::out_of_range("Pixel coordinates out of bounds");
        }
        return PixelData[coord.y * WIDTH + coord.x];
    }

    /*
    Safe way to set pixel values
    */
    /**
     * @brief Set a pixel at `coord` to `value` (bounds-checked).
     */
    void Set(XY coord, Pixel value)
    {
        if (coord.x < 0 || coord.x >= WIDTH || coord.y < 0 || coord.y >= HEIGHT)
        {
            throw std::out_of_range("Pixel coordinates out of bounds");
        }
        PixelData[coord.y * WIDTH + coord.x] = value;
    }

    // Delete Image Buffer
    /**
     * @brief Clear the internal pixel buffer, releasing memory.
     */
    void DeleteImageBuffer()
    {
        PixelData.clear();
    }

    const std::vector<Pixel>& returnImageBufferConst() const
    {
        return PixelData;
    } 

    /*
    ## Resize image and reallocate buffer  
    
    #### IMPORTANT  

    This only resizes the vector. It does not scale the pixels up or down.  
    It simply changes the dimensions and allocates a new buffer of the appropriate size.  
    The pixel data in the new buffer will be uninitialized (default constructed).  
    Use with caution, as this can lead to loss of data if not used properly.

    #### This does not resample image content;  
    the buffer is reallocated and  
    default-initialized pixels replace any previous contents.
    */
    void ResizeImage(int new_width, int new_height)
    {
        WIDTH = new_width;
        HEIGHT = new_height;
        Init(); 
    }
};

/**
 * @brief Convert a single pixel from `PixelInputType` to `PixelReturnType`.
 *
 * This wrapper dispatches to the appropriate `to*` helper based on the
 * requested return type at compile-time.
 */
template<typename PixelReturnType, typename PixelInputType>
PixelReturnType convertPixel(PixelInputType p)
{

    if constexpr (std::is_same_v<PixelReturnType, BWPixel>)
    {
        return toBW(p);
    }
    else if constexpr (std::is_same_v<PixelReturnType, Gray16BitPixel>)
    {
        return toGray16Bit(p);
    }
    else if constexpr (std::is_same_v<PixelReturnType, RGB8BitPixel>)
    {
        return toRGB8Bit(p);
    }
    else if constexpr (std::is_same_v<PixelReturnType, RGBA8BitPixel>)
    {
        return toRGBA8Bit(p);
    }
    else if constexpr (std::is_same_v<PixelReturnType, RGB16BitPixel>)
    {
        return toRGB16Bit(p);
    }
    else if constexpr (std::is_same_v<PixelReturnType, RGBA16BitPixel>)
    {
        return toRGBA16Bit(p);
    }
    else if constexpr (std::is_same_v<PixelReturnType, Gray8BitPixel>)
    {
        return toGray8Bit(p);
    }
    else
    {
        throw std::runtime_error(std::string("Unsupported conversion from ") + typeid(PixelInputType).name() + " to " + typeid(PixelReturnType).name());
    }
}

/**
 * @brief Convert an image from one pixel representation to another.
 *
 * Performs a pixel-by-pixel conversion of the source image into a new Image
 * whose pixel type is PixelReturnType. The conversion for each pixel is
 * selected at compile time using if constexpr and the available helper
 * functions (toBW, toGray16Bit, toRGB8Bit, toRGBA8Bit, toRGB16Bit,
 * toRGBA16Bit, toGray8Bit).
 *
 * @tparam PixelReturnType The desired pixel type for the returned Image.
 * @tparam PixelInputType  The pixel type of the input Image.
 *
 * @param img Reference to the source Image<PixelInputType> to convert.
 *
 * @return Image<PixelReturnType> A new image with the same dimensions and
 *         metadata as the source (copied/initialized from img) where each
 *         pixel has been converted to `PixelReturnType`.
 *
 * @throws std::runtime_error If no supported conversion path exists for the
 *         requested PixelReturnType / PixelInputType combination.
 * @throws std::out_of_range If accessing or setting pixels triggers bounds
 *         checks (propagates exceptions from Image::Get / Image::Set).
 *
 * @note If PixelReturnType is the same as PixelInputType the image will be
 *       effectively copied. The conversion complexity is O(WIDTH * HEIGHT).
 */
/**
 * @brief Convert an entire image from `PixelInputType` to `PixelReturnType`.
 *
 * Performs a pixel-by-pixel conversion and returns a new `Image<PixelReturnType>`
 * with the same dimensions and metadata as the source `img`.
 */
template<typename PixelReturnType, typename PixelInputType, typename FormatDataType = EmptyFormatData>
Image<PixelReturnType, FormatDataType> convertImage(
    const Image<PixelInputType, FormatDataType>& img
)
{
    Image<PixelReturnType, FormatDataType> new_img(img.WIDTH, img.HEIGHT, img.MAXVAL, img.TYPE, img.CHANNELS, img.FORMATDATA);

    for (int y = 0; y < img.HEIGHT; y++)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            PixelInputType p = img.Get(XY{x, y});
            PixelReturnType converted_pixel = convertPixel<PixelReturnType, PixelInputType>(p);
            new_img.Set(XY{x, y}, converted_pixel);
        }
    }

    return new_img;
};

template<typename PixelType, typename FormatDataType = EmptyFormatData>
std::vector<uint8_t> convertToRawBuffer(const Image<PixelType, FormatDataType>& img)
{
    std::vector<uint8_t> buffer;
    auto pixels = img.returnImageBufferConst();

    for (int y = 0; y < img.HEIGHT; y++)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            PixelType pixel = pixels[y * img.WIDTH + x];
            
            auto p = convertPixel<RGBA8BitPixel, PixelType>(pixel);

            buffer.push_back(p.r);
            buffer.push_back(p.g);
            buffer.push_back(p.b);
            buffer.push_back(p.a);

        }
    }

    return buffer;
}

#endif