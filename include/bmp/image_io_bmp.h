// image_io_bmp.h

#ifndef IMAGE_IO_BMP_H
#define IMAGE_IO_BMP_H

#include <string>
#include "image.h"

Image loadImageBmp(std::string path);
int writeImageBmp(std::string path, Image& img);

#endif