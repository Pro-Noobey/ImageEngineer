# ImageEngineer

ImageEngineer is a small, single-header-friendly C++ image processing library
that focuses on teaching and low-level image format handling without
third-party image libraries.

Supported formats
- NetPBM: PBM / PGM / PPM (ASCII + Binary)
- BMP: 24-bit, 32-bit and 8-bit RLE8 (basic subset)

Features
- Read and write NetPBM and BMP files
- Convert between formats and sample depths (8/16-bit internally)
- Basic image operations: blur, grayscale (average/luminosity),
  black & white, inversion, bloom, vertical/horizontal flip
- Small codebase intended for learning and experimentation

Quick usage
Include the single header and call the helpers:

```cpp
#include "ImageEngineer.h"

// Load (use the appropriate loader for your format)
auto img = loadImagePpm("input.ppm", /*binary=*/true);

// Apply an operation
auto gray = Grayscale(img);

// Write back to disk
writeImagePpm("out.ppm", gray, /*binary=*/true);
```

Build
- Windows: run `build.bat`
- Linux/macOS: run `build.sh`

Notes
- The project uses OpenMP for the blur implementation; enable OpenMP in
  your compiler flags if you want the parallel speed-up.
- The repository now provides `include/ImageEngineer.h` which aggregates the
  original headers into a single include for convenience.

Project structure (relevant files)

```
include/            -> headers (ImageEngineer.h)
src/                -> implementation / example CLI
build.bat / build.sh -> build scripts
```

This project exists for education and experimentation — feel free to fork
and modify `ImageEngineer.h` for your own experiments.
