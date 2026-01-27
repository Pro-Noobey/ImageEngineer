// image_io_pbm.h

#ifndef IMAGE_IO_PBM_H
#define IMAGE_IO_PBM_H

#include <string>

class Image;

Image loadImagePBM(std::string path, bool binary);
Image loadImagePGM(std::string path, bool binary);
Image loadImagePPM(std::string path, bool binary);
int writeImagePBM(std::string path, Image& img, bool binary);
int writeImagePGM(std::string path, Image& img, bool binary);
int writeImagePPM(std::string path, Image& img, bool binary);


#endif