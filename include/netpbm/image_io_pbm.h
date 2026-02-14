// image_io_pbm.h

#ifndef IMAGE_IO_PBM_H
#define IMAGE_IO_PBM_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "image.h"

using namespace std::string_literals;

/**
 * @file image_io_pbm.h
 * @brief Loading and saving functions for Netpbm formats (PBM/PGM/PPM).
 *
 * This header provides helpers to read/write ASCII and binary Netpbm files
 * into/from the project's in-memory `Image<RGB16BitPixel>` representation.
 */

/**
 * @brief Read an ASCII text file and remove comment lines.
 *
 * This helper reads the file at `path` (treated as ASCII text) and
 * returns a string containing the file contents with any full-line
 * comments removed. A comment line is any line whose first non-newline
 * character is `#`.
 *
 * @param path Path to the ASCII file to clean.
 * @return std::string The file contents with comment lines removed.
 *
 * @note This function is intended for use with ASCII Netpbm formats
 *       (P1/P2/P3). It does not perform binary-aware reads; calling
 *       this on binary data will produce undefined/incorrect results.
 */
/**
 * @brief Read an ASCII file and remove full-line comments (# ...).
 *
 * Returns the cleaned file contents as a single string where comment lines
 * have been removed. Intended for use with ASCII P1/P2/P3 Netpbm files.
 */
std::string clean(std::string path)
{
	std::ifstream file(path, std::ios::binary);

	std::string cleaned = "";
	std::string line;

	while (std::getline(file, line))
	{
		if ((!line.empty() && line[0] == '#'))
		{
			continue;
		}
		else
		{
			cleaned += line;
			cleaned += "\n";
		}
	}

	return cleaned;
}

/**
 * @brief Extract the 8 individual bits from a single byte.
 *
 * Bits are returned in MSB-to-LSB order: bits[0] is the most
 * significant bit (bit 7) and bits[7] is the least significant bit.
 *
 * @param byte The input byte to extract bits from.
 * @return std::vector<bool> Vector of 8 booleans representing the bits.
 */
/**
 * @brief Extract the 8 bits from `byte` into a vector<bool> (MSB first).
 */
std::vector<bool> extract(uint8_t byte)
{
	std::vector<bool> bits;

	for (int i = 7; i >= 0; i--)
	{
		bool bit = (byte >> i) & 1;
		bits.push_back(bit);
	}
	return bits;
}

/**
 * @brief Load a PBM (Portable Bitmap) image into an Image<RGB16BitPixel>.
 *
 * Supports both binary (P4) and ASCII (P1) PBM files. The function reads the
 * header (magic number, width, height) and then the pixel payload. In binary
 * mode pixels are stored as packed bits (MSB first) where each bit represents
 * one pixel. In ASCII mode pixels are read as whitespace-separated '0'/'1'
 * tokens after removing comment lines.
 *
 * @param path Filesystem path to the PBM file.
 * @param binary If true, parse as P4 (binary); otherwise parse as P1 (ASCII).
 * @return Image<RGB16BitPixel> Constructed image with 16-bit RGB pixels.
 * @throws std::runtime_error On file open failure or unexpected magic/header.
 */
/**
 * @brief Load a PBM (P1 ASCII or P4 binary) into an `Image<RGB16BitPixel>`.
 *
 * In PBM files each pixel is a single bit. The returned image uses 16-bit
 * RGB pixels where black is 0 and white is `65035` (chosen by this project).
 */
Image<RGB16BitPixel> loadImagePbm(const std::string path, bool binary)
{
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open())
	{
		std::string msg = "File Did not open, Path: "s + path + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, 65035, PBM_ASCII, 3};
	}

	char magic[2];
	int width, height;

	if (binary)
	{
		file.read(magic, 2);

		if (magic[0] != 'P' && magic[1] != '4')
		{
			std::string msg = "Magic is not P4. Magic: "s +magic[0]+magic[1] + '\n';
			throw std::runtime_error(msg);
			return Image<RGB16BitPixel>{16, 16, 65035, PBM_BINARY, 3};
		}

		while (file >> std::ws && file.peek() == '#') {
			std::string dummy;
			std::getline(file, dummy);
		}

		file >> width >> height;
		file.get();

		int row_bytes = (width + 7) / 8;
		int image_size = row_bytes * height;
		Image<RGB16BitPixel> img{width, height, 65035, PBM_BINARY, 3};

		std::vector<uint8_t> buffer(image_size); 
		file.read(reinterpret_cast<char*>(buffer.data()), image_size);

		for (int y = 0; y < height; y++)
		{
			for (int xb = 0; xb < row_bytes; xb++)
			{
				uint8_t byte = buffer[y * row_bytes + xb];
				std::vector<bool> bits = extract(byte);

				int bitpos = 7;
				for (const bool bit : bits)
				{
					int x = xb * 8 + (7 - bitpos);
					if (x >= width) continue;
					img.Set(XY{x, y}, bit ? RGB16BitPixel{0,0,0} : RGB16BitPixel{65035,65035,65035});
					bitpos--;
				}
			}
		}

		return img;
	}

	// PBM ASCII

	std::istringstream filec(clean(path)); // cleans the file from comments

	filec >> magic >> width >> height;

		if (magic[0] != 'P' && magic[1] != '1')
		{
			std::string msg = "Magic is not P1. Magic: "s +magic[0]+magic[1] + '\n';
			throw std::runtime_error(msg);
			return Image<RGB16BitPixel>{16, 16, 65035, PBM_ASCII, 3};
		}

	Image<RGB16BitPixel> img{width, height, 65035, PBM_ASCII, 3};

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			char bit; // '1' or '0'

			filec >> bit; // grabs that bit

			if (bit == '0') img.Set(XY{x, y}, RGB16BitPixel{0, 0, 0}); // 1 = black
			if (bit == '1') img.Set(XY{x, y}, RGB16BitPixel{65035, 65035, 65035}); // 0 = white
		}
	}
	return img;
}

/**
 * @brief Load a PGM (Portable Graymap) image into an Image<RGB16BitPixel>.
 *
 * Supports binary (P5) and ASCII (P2) PGM files. The header (magic, width,
 * height, maxval) is parsed first. If `maxval` is > 255, samples are treated
 * as 16-bit; otherwise they are 8-bit and are up-converted to 16-bit
 * grayscale RGB values in the returned image.
 *
 * @param path Path to the PGM file.
 * @param binary If true, parse as P5 (binary); otherwise parse as P2 (ASCII).
 * @return Image<RGB16BitPixel> The resulting grayscale image stored in 16-bit RGB.
 * @throws std::runtime_error On file errors or invalid header/magic values.
 */
/**
 * @brief Load a PGM (P2 ASCII or P5 binary) into an `Image<RGB16BitPixel>`.
 *
 * The grayscale samples are normalized/up-converted into the 16-bit RGB
 * representation used by the project.
 */
Image<RGB16BitPixel> loadImagePgm(const std::string path, bool binary)
{
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open())
	{
		std::string msg = "File Did not open, Path: "s + path + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, 65035, PGM_BINARY, 3};
	}

	char magic[2];
	int width, height, maxval;

	file.read(magic, 2);
	if (binary)
	{

		if (magic[0] != 'P' && magic[1] != '5')
		{
			std::string msg = "Magic is not P5. Magic: "s +magic[0]+magic[1] + '\n';
			throw std::runtime_error(msg);
			return Image<RGB16BitPixel>{16, 16, maxval, PGM_BINARY, 3};
		}
		while (file >> std::ws && file.peek() == '#') {
			std::string dummy;
			std::getline(file, dummy);
		}

		file >> width >> height >> maxval;
		file.get();

		if (maxval > 65035)
		{
			std::string msg = "Maxval is bigger than 65035. Maxval: "s +magic[0]+magic[1] + '\n';
			throw std::runtime_error(msg);
			return Image<RGB16BitPixel>{16, 16, maxval, PGM_BINARY, 3};
		}

		Image<RGB16BitPixel> img{width, height, maxval, PGM_BINARY, 3};

		bool bit16 = (maxval > 255); // true for 16 Bit

		if (!bit16)
		{
			std::vector<Gray8BitPixel> buffer(width * height);
			file.read(reinterpret_cast<char*>(buffer.data()), width * height);

			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					img.Set(XY{x, y}, toRGB16Bit(buffer[y * width + x]));
				}
			}

			return img;
		}

		// 16-bit case
		std::vector<Gray16BitPixel> buffer16(width * height);
		file.read(reinterpret_cast<char*>(buffer16.data()), width * height);

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				img.Set(XY{x, y}, toRGB16Bit(buffer16[y * width + x]));
			}
		}

		return img;

	}

	// ASCII

	std::istringstream filec(clean(path));

	filec.read(magic, 2);

	if (magic[0] != 'P' && magic[1] != '2')
	{
		std::string msg = "Magic is not P2. Magic: "s +magic[0]+magic[1] + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, maxval, PGM_ASCII, 3};
	}

	filec >> width >> height >> maxval;

	if (maxval > 65035)
	{
		std::string msg = "Maxval is bigger than 65035. Maxval: "s +magic[0]+magic[1] + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, maxval, PGM_ASCII, 3};
	}

	Image<RGB16BitPixel> img{width, height, maxval, PGM_ASCII, 3};

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int value;
			filec >> value;
			uint16_t gray = scale8ToMax(value, maxval);
			img.Set(XY{x, y}, RGB16BitPixel{gray, gray, gray});
		}
	}
	return img;
}

/**
 * @brief Load a PPM (Portable Pixmap) image into an Image<RGB16BitPixel>.
 *
 * Handles binary (P6) and ASCII (P3) PPM files. The header (magic, width,
 * height, maxval) is parsed, comments are skipped, and pixel data are read.
 * If `maxval` is greater than 255 the file contains 16-bit samples; otherwise
 * samples are 8-bit and are promoted to 16-bit in the returned image.
 *
 * @param path Path to the PPM file.
 * @param binary If true, parse as P6 (binary); otherwise parse as P3 (ASCII).
 * @return Image<RGB16BitPixel> The parsed image stored with 16-bit RGB channels.
 * @throws std::runtime_error On file I/O errors or invalid header/magic values.
 */
/**
 * @brief Load a PPM (P3 ASCII or P6 binary) into an `Image<RGB16BitPixel>`.
 *
 * Supports 8-bit and 16-bit sample depths; 8-bit samples are promoted to
 * 16-bit in the returned image.
 */
Image<RGB16BitPixel> loadImagePpm(const std::string path, bool binary)
{
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open())
	{
		std::string msg = "File Did not open, Path: "s + path + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, 65035, PPM_BINARY, 3};
	}

	char magic[2];
	int width, height, maxval;

	if (binary)
	{
		file.read(magic, 2);

		if (magic[0] != 'P' && magic[1] != '6')
		{
			std::string msg = "Magic is not P6. Magic: "s +magic[0]+magic[1] + '\n';
			throw std::runtime_error(msg);
			return Image<RGB16BitPixel>{16, 16, 65035, PPM_BINARY, 3};
		}

		while (file >> std::ws && file.peek() == '#') {
			std::string dummy;
			std::getline(file, dummy);
		}

		file >> width >> height >> maxval;
		file.get();

		if (maxval > 65035)
		{
			std::string msg = "Maxval is bigger than 65035. Maxval: "s +magic[0]+magic[1] + '\n';
			throw std::runtime_error(msg);
			return Image<RGB16BitPixel>{16, 16, maxval, PPM_BINARY, 3};
		}

		Image<RGB16BitPixel> img{width, height, maxval, PPM_BINARY, 3};

		bool bit16 = (maxval > 255); // true for 16 Bit

		if (!bit16) // 8-bit case
		{
			std::vector<RGB8BitPixel> buffer8Bit(width * height);
			file.read(reinterpret_cast<char*>(buffer8Bit.data()), width * height * 3);

			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					RGB8BitPixel p = buffer8Bit[y * width + x];
					RGB16BitPixel p16 = toRGB16Bit(p);

					img.Set(XY{x, y}, p16);
				}
			}

			return img;
		}

		// 16-bit case
		std::vector<RGB16BitPixel> buffer16Bit(width * height);
		file.read(reinterpret_cast<char*>(buffer16Bit.data()), width * height * 3);

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				img.Set(XY{x, y}, buffer16Bit[y * width + x]);
			}
		}

		return img;
	}
	// continuing here is ASCII

	file.read(magic, 2);

	if (magic[0] != 'P' && magic[1] != '3')
	{
		std::string msg = "Magic is not P3. Magic: "s +magic[0]+magic[1] + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, 65035, PPM_ASCII, 3};
	}

	std::istringstream filec(clean(path));

	filec >> magic >> width >> height >> maxval;

	if (maxval > 65035)
	{
		std::string msg = "Maxval is bigger than 65035. Maxval: "s +magic[0]+magic[1] + '\n';
		throw std::runtime_error(msg);
		return Image<RGB16BitPixel>{16, 16, maxval, PPM_ASCII, 3};
	}

	Image<RGB16BitPixel> img{width, height, maxval, PPM_ASCII, 3};

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int r, g, b;
			filec >> r >> g >> b;
			img.Set(XY{x, y}, RGB16BitPixel{scale8ToMax(r, maxval), scale8ToMax(g, maxval), scale8ToMax(b, maxval)});
		}
	}
	return img;
}

/**
 * @brief Write an image as PBM (P1/P4). The image is converted to a
 * monochrome representation before being written.
 *
 * @return 0 on success, non-zero on failure.
 */
template<typename Pixel>
int writeImagePbm(const std::string path, Image<Pixel>& img, bool binary)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) return 1;

	if (binary)
	{
		std::string header = "P4\n"s + 
		std::to_string(img.WIDTH) + 
		" "s +
		std::to_string(img.HEIGHT) +
		"\n"s;

		file << header;

		for (int y = 0; y < img.HEIGHT; y++)
		{
			for (int x = 0; x < img.WIDTH; x++)
			{
				Pixel p = img.Get(XY{x, y});
				RGB8BitPixel p8Bit = toRGB8Bit(p);
				uint8_t byte = (p8Bit.r > 127 ? 0 : 1) << 7 | (p8Bit.g > 127 ? 0 : 1) << 6 | (p8Bit.b > 127 ? 0 : 1) << 5;
				file.write(reinterpret_cast<const char*>(&byte), sizeof(uint8_t));
			}
		}
		return 0;
	}
	else
	{
		std::string header = "P1\n"s + 
		std::to_string(img.WIDTH) + 
		" " + 
		std::to_string(img.HEIGHT) + 
		"\n"s;

		file << header;

		for (int y = 0; y < img.HEIGHT; y++)
		{
			for (int x = 0; x < img.WIDTH; x++)
			{
				Pixel p = img.Get(XY{x, y});
				RGB8BitPixel p8Bit = toRGB8Bit(p);
				file << ((p8Bit.r > 127 || p8Bit.g > 127 || p8Bit.b > 127) ? '1' : '0') << "\n";
			}
		}
		return 0;
	}
}

/**
 * @brief Write an image as PGM (P2/P5). Image pixels are converted to
 * grayscale as part of the write process.
 */
template<typename Pixel>
int writeImagePgm(const std::string path, Image<Pixel>& img, bool binary)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) return 1;
	
	if (binary)
	{
		std::string header = "P5\n"s + 
		std::to_string(img.WIDTH) + 
		" "s +
		std::to_string(img.HEIGHT) +
		"\n"s + 
		std::to_string(img.MAXVAL) +
		"\n"s;

		file << header;

		for (int y = 0; y < img.HEIGHT; y++)
		{
			for (int x = 0; x < img.WIDTH; x++)
			{
                Pixel p = img.Get(XY{x, y});

                RGB8BitPixel p8Bit = toRGB8Bit(p);
				file.write(reinterpret_cast<const char*>(&p8Bit), sizeof(RGB8BitPixel));
			}
		}
        return 0;
    }
    else
    {
        std::string header = "P2\n"s + 
        std::to_string(img.WIDTH) + 
        " " + 
        std::to_string(img.HEIGHT) + 
        "\n"s + 
        std::to_string(img.MAXVAL) + 
        "\n"s;

        file << header;

        for (int y = 0; y < img.HEIGHT; y++)
        {
            for (int x = 0; x < img.WIDTH; x++)
            {
                Pixel p = img.Get(XY{x, y});
                RGB8BitPixel p8Bit = toRGB8Bit(p);
                file << static_cast<int>(p8Bit.r) << " " << static_cast<int>(p8Bit.g) << " " << static_cast<int>(p8Bit.b) << "\n";
            }
        }
        return 0;
    }
}

/**
 * @brief Write an image as PPM (P3/P6). The sample depth written is
 * selected using `img.MAXVAL` (<=255 => 8-bit, >255 => 16-bit).
 */
template<typename Pixel>
int writeImagePpm(const std::string path, Image<Pixel>& img, bool binary)
{
	std::ofstream file(path, std::ios::binary);

	if (!file.is_open())
	{
		std::string msg = "File Did not open for writing, Path: "s + path + '\n';
		throw std::runtime_error(msg);
		return -1;
	}

	if (binary)
	{
		std::string header = "P6\n"s + 
		std::to_string(img.WIDTH) + 
		" "s +
		std::to_string(img.HEIGHT) +
		"\n"s + 
		std::to_string(img.MAXVAL) +
		"\n"s;

		file << header;

		for (int y = 0; y < img.HEIGHT; y++)
		{
			for (int x = 0; x < img.WIDTH; x++)
			{
                Pixel p = img.Get(XY{x, y});

                RGB8BitPixel p8Bit = toRGB8Bit(p);
                RGB16BitPixel p16Bit = toRGB16Bit(p);
				if (img.MAXVAL > 255) file.write(reinterpret_cast<const char*>(&p16Bit), sizeof(RGB16BitPixel));
                else file.write(reinterpret_cast<const char*>(&p8Bit), sizeof(RGB8BitPixel));
			}
		}
		return 0;
	}
	// ASCII case

	std::string header = "P3\n"s + 
	std::to_string(img.WIDTH) + 
	" " + 
	std::to_string(img.HEIGHT) + 
	"\n"s + 
	std::to_string(img.MAXVAL) + 
	"\n"s;

	file << header;

	for (int y = 0; y < img.HEIGHT; y++)
	{
		for (int x = 0; x < img.WIDTH; x++)
		{
			Pixel p = img.Get(XY{x, y});

            RGB8BitPixel p8Bit = toRGB8Bit(p);
            RGB16BitPixel p16Bit = toRGB16Bit(p);
            if (img.MAXVAL > 255) file << p16Bit.r << " " << p16Bit.g << " " << p16Bit.b << "\n";
            else file << p8Bit.r << " " << p8Bit.g << " " << p8Bit.b << "\n";
		}
	}

	return 0;
}

#endif