#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <omp.h>
#include <iostream>
#include "image.h"

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

Image Blur(Image& src_img, int passes = 5, int kernel_size = 3)
{
    Image new_img = src_img;

    uint32_t average_r, average_g, average_b, average_a;
    int num_neighbors;
    std::vector<XY> neighbors;
    Pixel blurred_pixel;

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

                    Pixel p = src_img.get({nb.x, nb.y});
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
                    Clamp(average_r, src_img.MAXVAL),
                    Clamp(average_g, src_img.MAXVAL),
                    Clamp(average_b, src_img.MAXVAL),
                    Clamp(average_a, src_img.MAXVAL)
                };

                new_img.data[y * new_img.WIDTH + x] = blurred_pixel;
            }
        }
        src_img = new_img;
    }
    return new_img;
}

Image Grayscale(Image& src_img, int method = GRAYSCALE_AVERAGE, std::vector<float> weights = {0.21f, 0.72f, 0.07f})
{
    if (method != GRAYSCALE_AVERAGE && method != GRAYSCALE_LUMINOSITY)
        throw std::invalid_argument("Invalid grayscale method");

    Image new_img = src_img;
    int pixel_i = 0;

    if (method == GRAYSCALE_AVERAGE)
    {
        for (const auto& pixel : new_img.data)
        {
            uint32_t gray = (pixel.r + pixel.g + pixel.b) / 3;
            uint16_t g = Clamp(gray, new_img.MAXVAL);

            new_img.data[pixel_i].r = g;
            new_img.data[pixel_i].g = g;
            new_img.data[pixel_i].b = g;
            pixel_i++;
        }
    }
    else
    {
        for (const auto& pixel : new_img.data)
        {
            uint32_t gray = static_cast<uint32_t>(
                weights[0] * pixel.r +
                weights[1] * pixel.g +
                weights[2] * pixel.b
            );

            uint16_t g = Clamp(gray, new_img.MAXVAL);

            new_img.data[pixel_i].r = g;
            new_img.data[pixel_i].g = g;
            new_img.data[pixel_i].b = g;
            pixel_i++;
        }
    }

    return new_img;
}

Image Bw(Image& src_img, uint16_t threshold = 0)
{
    if (threshold == 0)
        threshold = src_img.MAXVAL / 2;

    int pixel_i = 0;

    for (const auto& pixel : src_img.data)
    {
        uint32_t gray = (pixel.r + pixel.g + pixel.b) / 3;
        uint16_t out = (gray >= threshold) ? src_img.MAXVAL : 0;

        src_img.data[pixel_i].r = out;
        src_img.data[pixel_i].g = out;
        src_img.data[pixel_i].b = out;
        pixel_i++;
    }

    return src_img;
}

bool isGrayscaleOrBw(Image& img)
{
    for (Pixel& pixel : img.data)
    {
        if (!(pixel.r == pixel.g && pixel.g == pixel.b))
            return false;
    }
    return true;
}

Image invertColor(Image& src_img)
{
    int pixel_i = 0;

    for (const auto& pixel : src_img.data)
    {
        src_img.data[pixel_i].r = src_img.MAXVAL - pixel.r;
        src_img.data[pixel_i].g = src_img.MAXVAL - pixel.g;
        src_img.data[pixel_i].b = src_img.MAXVAL - pixel.b;
        pixel_i++;
    }

    return src_img;
}

Image flipVertical(Image& src_img)
{
    Image out = src_img;

    for (int y = 0; y < src_img.HEIGHT; y++)
    {
        for (int x = 0; x < src_img.WIDTH; x++)
        {
            out.data[(src_img.HEIGHT - 1 - y) * src_img.WIDTH + x] =
                src_img.data[y * src_img.WIDTH + x];
        }
    }
    return out;
}

Image flipHorizontal(Image& src_img)
{
    Image out = src_img;

    for (int y = 0; y < src_img.HEIGHT; y++)
    {
        for (int x = 0; x < src_img.WIDTH; x++)
        {
            out.data[y * src_img.WIDTH + (src_img.WIDTH - 1 - x)] =
                src_img.data[y * src_img.WIDTH + x];
        }
    }
    return out;
}
