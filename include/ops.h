// ops.h

#ifndef OPS_H
#define OPS_H

#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <omp.h>
#include <iostream>
#include "image.h"

/**
 * @file ops.h
 * @brief Common image processing operations (blur, grayscale, bloom, flips).
 */

/**
 * @brief Return coordinates of neighboring pixels for a square kernel.
 *
 * The returned vector contains all XY offsets within a kernel of size
 * `kernel_size` centered at `center`. Caller is responsible for bounds checks
 * when sampling the neighboring coordinates.
 */
std::vector<XY> findNeighborIndices(int kernel_size, XY center)
{
    std::vector<XY> neighbors;
    int r = kernel_size / 2;

    for (int dy = -r; dy <= r; dy++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            neighbors.push_back({center.x + dx, center.y + dy});
        }
    }

    return neighbors;
}
/**
 * @brief Apply a simple box blur to `src_img`.
 *
 * The blur runs for `passes` iterations and uses a `kernel_size` x
 * `kernel_size` neighbourhood. The operation is implemented using OpenMP for
 * parallelization across rows.
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> Blur(Image<Pixel, FormatData>& src_img, int passes = 5, int kernel_size = 3)
{
    Image<Pixel, FormatData> new_img = src_img;

    uint32_t average_r, average_g, average_b, average_a;
    int num_neighbors;
    std::vector<XY> neighbors;
    RGBA16BitPixel blurred_pixel;

    for (int pass = 0; pass < passes; pass++)
    {
        #pragma omp parallel for private(average_r, average_g, average_b, average_a, num_neighbors, neighbors, blurred_pixel) schedule(dynamic)
        for (int y = 0; y < src_img.HEIGHT; y++)
        {
            for (int x = 0; x < src_img.WIDTH; x++)
            {
                average_r = average_g = average_b = average_a = 0;
                num_neighbors = 0;
                neighbors = findNeighborIndices(kernel_size, {x, y});

                for (const auto& nb : neighbors)
                {
                    if (nb.x < 0 || nb.x >= src_img.WIDTH || nb.y < 0 || nb.y >= src_img.HEIGHT)
                        continue;

                    RGBA16BitPixel p = toRGBA16Bit(src_img.Get(XY{nb.x, nb.y}));
                    average_r += p.r;
                    average_g += p.g;
                    average_b += p.b;
                    average_a += p.a;
                    num_neighbors++;
                }

                average_r /= num_neighbors;
                average_g /= num_neighbors;
                average_b /= num_neighbors;
                average_a /= num_neighbors;

                blurred_pixel = {
                    Clamp(average_r, 65035),
                    Clamp(average_g, 65035),
                    Clamp(average_b, 65035),
                    Clamp(average_a, 65035)
                };

                new_img.Set(XY{x, y}, convertPixel<Pixel, RGBA16BitPixel>(blurred_pixel));
            }
        }
        src_img = new_img;
    }
    return new_img;
}

/**
 * @brief Convert `src_img` to grayscale.
 *
 * `method` selects either a simple average or a weighted luminosity formula
 * using `weights` for the RGB channels.
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> Grayscale(Image<Pixel, FormatData>& src_img, int method = GRAYSCALE_AVERAGE, std::vector<float> weights = {0.21f, 0.72f, 0.07f})
{
    if (method != GRAYSCALE_AVERAGE && method != GRAYSCALE_LUMINOSITY)
        throw std::invalid_argument("Invalid grayscale method");

    Image<Pixel, FormatData> new_img = src_img;

    if (method == GRAYSCALE_AVERAGE)
    {
        for (int y = 0; y < src_img.HEIGHT; y++)
        {
            for (int x = 0; x < src_img.WIDTH; x++)
            {
                auto p = toRGB16Bit(src_img.Get(XY{x, y}));
                uint16_t gray = static_cast<uint16_t>((p.r + p.g + p.b) / 3);

                new_img.Set(XY{x, y}, convertPixel<Pixel, Gray16BitPixel>(Gray16BitPixel{gray}));
            }
        }
    }
    else
    {
        for (int y = 0; y < src_img.HEIGHT; y++)
        {
            for (int x = 0; x < src_img.WIDTH; x++)
            {
                auto p = toRGB16Bit(src_img.Get(XY{x, y}));
                uint16_t gray = static_cast<uint16_t>(p.r * weights[0] + p.g * weights[1] + p.b * weights[2]);

                new_img.Set(XY{x, y}, convertPixel<Pixel, Gray16BitPixel>(Gray16BitPixel{gray}));
            }
        }
    }

    return new_img;
}

/**
 * @brief Convert `src_img` to a black-and-white image (binary threshold).
 *
 * `threshold` is taken from `src_img.MAXVAL` if zero is passed.
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> Bw(Image<Pixel, FormatData>& src_img, uint16_t threshold = 0)
{
    if (threshold == 0)
        threshold = src_img.MAXVAL / 2;

    Image<Pixel, FormatData> new_img = src_img;

    for (int y = 0; y < src_img.HEIGHT; y++)
    {
        for (int x = 0; x < src_img.WIDTH; x++)
        {
            new_img.Set(XY{x, y}, convertPixel<Pixel, BWPixel>(toBW(src_img.Get(XY{x, y}))));
        }
    }

    return new_img;
}

/**
 * @brief Return true if every pixel in `img` has equal R,G,B components.
 */
template<typename Pixel, typename FormatData>
bool isGrayscaleOrBw(Image<Pixel, FormatData>& img)
{
    for (int y = 0; y < img.HEIGHT; y++)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            auto p = toRGB8Bit(img.Get(XY{x, y}));
            if (p.r != p.g || p.g != p.b)
                return false;
        }
    }
    return true;
}

/**
 * @brief Invert the color channels of `src_img` producing a new image.
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> invertColor(Image<Pixel, FormatData>& src_img)
{
    Image<Pixel, FormatData> out = src_img;

    for (int y = 0; y < src_img.HEIGHT; y++)
    {
        for (int x = 0; x < src_img.WIDTH; x++)
        {
            auto p = toRGBA16Bit(src_img.Get(XY{x, y}));

            /*
            // toRGBA16Bit() produces 16-bit channel values (0..65535).
            // Use the 16-bit max (65535) for inversion so conversions
            // back to the image's pixel type scale correctly.
            int inv_r = static_cast<int>(65535) - static_cast<int>(p.r);
            int inv_g = static_cast<int>(65535) - static_cast<int>(p.g);
            int inv_b = static_cast<int>(65535) - static_cast<int>(p.b);
            int inv_a = static_cast<int>(65535) - static_cast<int>(p.a);

            RGBA16BitPixel inv = {
                Clamp(inv_r, 65535),
                Clamp(inv_g, 65535),
                Clamp(inv_b, 65535),
                Clamp(inv_a, 65535)
            };
            */

            // More optimized, less cluttery code snippet for inversion
            RGBA16BitPixel inv = {
                ~p.r,
                ~p.g,
                ~p.b,
                p.a // Preserve Alpha channel
            };
            

            out.Set(XY{x, y}, convertPixel<Pixel, RGBA16BitPixel>(inv));
        }
    }

    return out;
}

/**
 * @brief Flip the image vertically (top <-> bottom).
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> flipVertical(Image<Pixel, FormatData>& src_img)
{
    Image<Pixel, FormatData> out = src_img;

    for (int y = 0; y < src_img.HEIGHT; y++)
    {
        for (int x = 0; x < src_img.WIDTH; x++)
        {
            out.Set(XY{x, src_img.HEIGHT - 1 - y}, src_img.Get(XY{x, y}));
        }
    }
    return out;
}

/**
 * @brief Flip the image horizontally (left <-> right).
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> flipHorizontal(Image<Pixel, FormatData>& src_img)
{
    Image<Pixel, FormatData> out = src_img;

    for (int y = 0; y < src_img.HEIGHT; y++)
    {
        for (int x = 0; x < src_img.WIDTH; x++)
        {
            out.Set(XY{src_img.WIDTH - 1 - x, y}, src_img.Get(XY{x, y}));
        }
    }
    return out;
}

/**
 * @brief Apply a bloom effect by isolating bright areas, blurring them,
 * and adding them back into the original image.
 *
 * @param brightnessThreshold Threshold (16-bit) above which a pixel is
 * considered bright.
 * @param radius Blur kernel radius.
 * @param intensity Multiplier applied when adding the blurred bright image
 * back into the original.
 */
template<typename Pixel, typename FormatData>
Image<Pixel, FormatData> Bloom(
    Image<Pixel, FormatData>& img,
    int brightnessThreshold = 32768, /*In 16 Bit*/
    int radius = 5,
    int intensity = 1
)
{
    Image<Pixel, FormatData> brightImage = img;

    for (int y = 0; y < img.HEIGHT; y++)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            RGBA16BitPixel p = toRGBA16Bit(img.Get(XY{x, y}));
            bool isBright = (
                brightnessThreshold < (p.r + p.g + p.b) / 3 /*Average of RGB channels*/
            );

            if (isBright)
            {
                brightImage.Set(XY{x, y}, img.Get(XY{x, y}));
            }
            else if (!isBright)
            {
                brightImage.Set(XY{x, y}, convertPixel<Pixel, RGBA16BitPixel>(RGBA16BitPixel{0, 0, 0, 65535}));
            }
        }
    }

    Image<Pixel, FormatData> blurred = Blur(brightImage, 1, radius);

    Image<Pixel, FormatData> finalImage = img;

    brightImage.DeleteImageBuffer(); // free memory of brightImage as it's no longer needed

    for (int y = 0; y < img.HEIGHT; y++)
    {
        for (int x = 0; x < img.WIDTH; x++)
        {
            RGBA16BitPixel original = toRGBA16Bit(img.Get(XY{x, y}));
            RGBA16BitPixel bloom = toRGBA16Bit(blurred.Get(XY{x, y}));

            uint16_t r = Clamp(static_cast<int>(original.r) + static_cast<int>(bloom.r) * intensity, 65535);
            uint16_t g = Clamp(static_cast<int>(original.g) + static_cast<int>(bloom.g) * intensity, 65535);
            uint16_t b = Clamp(static_cast<int>(original.b) + static_cast<int>(bloom.b) * intensity, 65535);
            uint16_t a = Clamp(static_cast<int>(original.a) + static_cast<int>(bloom.a) * intensity, 65535);

            finalImage.Set(XY{x, y}, convertPixel<Pixel, RGBA16BitPixel>(RGBA16BitPixel{r, g, b, a}));
        }
    }
    return finalImage;
}

#endif