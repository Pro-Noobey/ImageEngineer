# ImgLib

A lightweight native image processing library written in C++ that supports the following formats:

- **NetPBM formats**
  - PBM (ASCII & Binary)
  - PGM (ASCII & Binary)
  - PPM (ASCII & Binary)
- **BMP**
  - 24-bit & 32-bit (uncompressed)

With basic filters and transformations built-in, this project focuses on understanding image formats at a low level and handling everything manually without external image libraries.

## Supported Formats

### Input
- PBM (ASCII & Binary)
- PGM (ASCII & Binary)
- PPM (ASCII & Binary)
- BMP (24-bit & 32-bit, uncompressed)

### Output
- PBM
- PGM
- PPM
- BMP (24-bit & 32-bit)

## Features

- Native BMP support (no external libs)
- Full NetPBM support
- Format conversion between NetPBM and BMP
- Built-in image operations:
  - Blur
  - Grayscale (Average & Luminosity)
  - Black & White
  - Color Inversion
  - Vertical Flip
  - Horizontal Flip
- Supports images with MAXVAL &gt; 255 internally
- Manual parsing and writing of file formats

## Example Usage

```cpp
Image img = loadImage("input.ppm");
img = Grayscale(img);
writeImage("output.bmp", img, BMP);
```

## Design Goals

- Zero external image libraries
- Educational focus on image formats
- Cross-platform compatibility
- Simple, hackable codebase

## Build

Compile with any modern C++ compiler. There is a build.bat for Windows, build.sh for Linux.:  

Linux >>
```bash
./build
```
Windows >>
```powershell
.\build
```

(Enable OpenMP if you're using the blur optimization)

## Project Structure

```
include/bmp        -> BMP reader/writer
include/netpbm     -> PBM/PGM/PPM reader/writer
image.h     -> Core image structures
ops.cpp     -> Image operations
main.cpp    -> Example usage / CLI
```

## Why This Exists

### Because sometimes it’s more fun to *build the wheel* than use the wheel
