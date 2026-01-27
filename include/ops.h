// ops.h

#ifndef OPS_H
#define OPS_H

#include "image.h"

class Image;

Image Blur(Image& src_img, int passes, int kernel_size);
Image Grayscale(Image& src_img, int method=GRAYSCALE_AVERAGE, std::vector<float> weights={0.21f, 0.72f, 0.07f});
Image Bw(Image& src_img, uint16_t threshold=0);
bool isGrayscaleOrBw(Image& img);
Image invertColor(Image& src_img);
Image flipVertical(Image& src_img);
Image flipHorizontal(Image& src_img);

#endif