#include <iostream> // for I/O
#include <vector> // for storing pixel data
#include <string> // for I/O
#include <fstream> // file operations
#include <chrono> // for timing
#include <sstream> // for I/O
#include "image.h" // for Pixel and Image struct and class
#include "ops.h" // for Image operations


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

std::vector<unsigned char> extract(unsigned char byte) {
    std::vector<unsigned char> bits;
    for (int i = 7; i >= 0; i--) {
        // Mask the specific bit with bitwise AND and right-shift to isolate it
        unsigned char bit = (byte >> i) & 1;  
        bits.push_back(bit);
    }
    return bits;
}

Image loadImagePBM(std::string path, bool binary)
{
    if (binary)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return {};

        std::string magicnumber;
        int width, height;

        file >> magicnumber;

        while (file >> std::ws && file.peek() == '#') {
            std::string dummy;
            std::getline(file, dummy);
        }

        file >> width >> height;
        file.get();

        int row_bytes = (width + 7) / 8;
        int image_size = row_bytes * height;

        std::vector<unsigned char> buffer(image_size);
        file.read(reinterpret_cast<char*>(buffer.data()), image_size);

        Image img{width, height, 255, PBM_BINARY};
        img.data.resize(width * height);

        int index = 0;
        for (int y = 0; y < height; y++)
        {
            for (int xb = 0; xb < row_bytes; xb++)
            {
                unsigned char byte = buffer[y * row_bytes + xb];
                for (int bit = 7; bit >= 0; bit--)
                {
                    int x = xb * 8 + (7 - bit);
                    if (x >= width) continue;

                    unsigned char value = (byte >> bit) & 1;

                    if (value == 0) { img.data[index++] = {255, 255, 255}; }
                    else           { img.data[index++] = {0, 0, 0}; }
                }
            }
        }

        return img;
    }

    std::istringstream cleaned_file(clean(path));

    std::string magicnumber;
    int width, height;

    cleaned_file >> magicnumber >> width >> height;

    Image img{width, height, 255, PBM_ASCII};
    img.data.resize(width * height);

    int bit;
    for (int i = 0; i < width * height; i++)
    {
        cleaned_file >> bit;
        if (bit == 0) { img.data[i] = {255, 255, 255}; }
        else          { img.data[i] = {0, 0, 0}; }
    }

    return img;
}

Image loadImagePGM(std::string path, bool binary)
{
    if (binary)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return {};

        std::string magicnumber;
        int width, height, maxval;

        file >> magicnumber;

        while (file >> std::ws && file.peek() == '#')
        {
            std::string dummy;
            std::getline(file, dummy);
        }

        file >> width >> height;

        while (file >> std::ws && file.peek() == '#')
        {
            std::string dummy;
            std::getline(file, dummy);
        }

        file >> maxval;
        file.get();

        Image img{width, height, maxval, PGM_BINARY};
        img.data.resize(width * height);

        for (int i = 0; i < width * height; i++)
        {
            if (maxval > 255) {
                uint16_t gray;
                file.read(reinterpret_cast<char*>(&gray), 2);
                img.data[i] = {gray, gray, gray};
            } else {
                uint8_t gray;
                file.read(reinterpret_cast<char*>(&gray), 1);
                img.data[i] = {gray, gray, gray};
            }
        }
        return img;
    }

    std::ifstream file(path);

    std::string magicnumber;
    int width;
    int height;
    int maxval;

    std::istringstream cleaned_file(clean(path));

    cleaned_file >> magicnumber >> width >> height >> maxval;

    cleaned_file.ignore();

    Image img{width, height, maxval, PGM_ASCII};
    img.data.resize(width * height);
    int value;
    for (int i = 0; i < width * height; i++)
    {
        cleaned_file >> value;
        uint16_t gray = static_cast<uint16_t>(value);
        img.data[i] = {gray, gray, gray};
    }
    return img;
}

Image loadImagePPM(std::string path, bool binary)
{
    if (binary) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return {};
        }

        std::string magicnumber;
        int width, height, maxval;

        file >> magicnumber;

        while (file >> std::ws && file.peek() == '#')
        {
            std::string dummy;
            std::getline(file, dummy);
        }

        file >> width >> height;

        while (file >> std::ws && file.peek() == '#')
        {
            std::string dummy;
            std::getline(file, dummy);
        }

        file >> maxval;
        file.get();

        Image img{width, height, maxval, PPM_BINARY};
        img.data.resize(width * height);

        for (int i = 0; i < width * height; ++i)
        {
            if (maxval > 255)
            {
                uint16_t rgb[3];
                file.read(reinterpret_cast<char*>(rgb), 6);
                img.data[i] = { rgb[0], rgb[1], rgb[2], 255 };
            }
            else
            {
                uint8_t rgb[3];
                file.read(reinterpret_cast<char*>(rgb), 3);
                img.data[i] = { rgb[0], rgb[1], rgb[2], 255 };
            }
        }

        return img;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::string magicnumber;
    int width;
    int height;
    int maxval;

    std::istringstream cleaned_file(clean(path));

    cleaned_file >> magicnumber >> width >> height >> maxval;
    cleaned_file.ignore();
    
    Image img{width, height, maxval, PPM_ASCII};
    img.data.resize(width * height);
    int r, g, b;
    for (int i = 0; i < width * height; i++) {
        cleaned_file >> r >> g >> b;
        img.data[i] = {static_cast<uint16_t>(r), static_cast<uint16_t>(g), static_cast<uint16_t>(b)};
    }

    return img;
}

int writeImagePBM(std::string path, Image& img, bool binary)
{
    if (binary)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file) return 1;

        std::string header = "P4\n# Made by the coolest Image Proccessor in Town!\n# The NetPBM Image Proccessor!!\n";
        header += std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n";
        file << header;

        if (!isGrayscaleOrBw(img))
        {
            img = Bw(img, 128);
        }

        uint8_t byte = 0;
        int current_bit = 7;
        int x = 0;

        for (const auto& pixel : img.data)
        {
            bool bit = (pixel.r == 0); // PBM: black = 1

            if (bit)
                byte |= (1 << current_bit);

            current_bit--;
            x++;

            if (current_bit < 0)
            {
                file.write(reinterpret_cast<const char*>(&byte), 1);
                byte = 0;
                current_bit = 7;
            }

            if (x == img.WIDTH)
            {
                if (current_bit != 7)
                {
                    file.write(reinterpret_cast<const char*>(&byte), 1);
                    byte = 0;
                    current_bit = 7;
                }
                x = 0;
            }
        }

        return 0;
    }

    std::ofstream file(path);
    if (!file) return 1;

    std::string final_string = "P1\n# Made by the coolest Image Proccessor in Town!\n# The NetPBM Image Proccessor!!\n";
    final_string += std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT) + "\n";

    if (!isGrayscaleOrBw(img))
    {
        img = Bw(img, 128);
    }
    for (const auto& pixel : img.data)
    {
        final_string += (pixel.r == 0 ? "1\n" : "0\n");
    }

    file << final_string;
    return 0;
}

int writeImagePGM(std::string path, Image& img, bool binary)
{
    if (binary)
    {
        std::ofstream file(path, std::ios::binary);

        if (!file) return 1;
        std::string header = "P5\n# Made by the coolest Image Proccessor in Town!\n# The NetPBM Image Proccessor!!\n";
        std::string dimensions = std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT);
        header += dimensions;
        header += "\n";
        header += std::to_string(img.MAXVAL);
        header += "\n";
        file << header;

        if (!isGrayscaleOrBw(img))
        {
            img = Grayscale(img, GRAYSCALE_LUMINOSITY);
        }
        for (const auto& pixel : img.data)
        {
            if (img.MAXVAL > 255) 
            {
                file.write(reinterpret_cast<const char*>(&pixel.r), 2);
            }
            else
            {
                uint8_t r = static_cast<uint8_t>(pixel.r);
                file.write(reinterpret_cast<const char*>(&r), 1);
            }
        }
        return 0;
    }

    std::ofstream file(path);

    if (!file) return 1;

    std::string final_string = "P2\n# Made by the coolest Image Proccessor in Town!\n# The NetPBM Image Proccessor!!\n";
    const std::string dimensions = std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT);
    final_string += dimensions;
    final_string += "\n";
    final_string += std::to_string(img.MAXVAL);
    final_string += "\n";

    int er, eg, eb;
    std::string r, g, b;
    if (img.TYPE != PGM_BINARY && img.TYPE != PGM_ASCII)
    {
        img = Grayscale(img, GRAYSCALE_LUMINOSITY);
    }
    for (const auto& pixel : img.data)
    {
        std::string gray = std::to_string(static_cast<int>(pixel.r));
        final_string += gray;
        final_string += "\n";
    }

    file << final_string;
    return 0;
}

int writeImagePPM(std::string path, Image& img, bool binary)
{
    if (binary)
    {
        std::ofstream file(path, std::ios::binary);

        if (!file) return 1;

        std::string header = "P6\n# Made by the coolest Image Proccessor in Town!\n# The NetPBM Image Proccessor!!\n";
        std::string dimensions = std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT);
        header += dimensions;
        header += "\n";
        header += std::to_string(img.MAXVAL);
        header += "\n";

        file << header;
        for (const auto& pixel : img.data)
        {
            if (img.MAXVAL > 255) 
            {
                file.write(reinterpret_cast<const char*>(&pixel.r), 2);
                file.write(reinterpret_cast<const char*>(&pixel.g), 2);
                file.write(reinterpret_cast<const char*>(&pixel.b), 2);
            }
            else
            {
                uint8_t r = static_cast<uint8_t>(pixel.r);
                uint8_t g = static_cast<uint8_t>(pixel.g);
                uint8_t b = static_cast<uint8_t>(pixel.b);
                file.write(reinterpret_cast<const char*>(&r), 1);
                file.write(reinterpret_cast<const char*>(&g), 1);
                file.write(reinterpret_cast<const char*>(&b), 1);
            }
        }

        return 0;
    }
    std::ofstream file(path);

    if (!file) return 1;

    std::string final_string = "P3\n# Made by the coolest Image Proccessor in Town!\n# The NetPBM Image Proccessor!!\n";
    const std::string dimensions = std::to_string(img.WIDTH) + " " + std::to_string(img.HEIGHT);
    final_string += dimensions;
    final_string += "\n";
    final_string += std::to_string(img.MAXVAL);
    final_string += "\n";

    int er, eg, eb;
    std::string r, g, b;
    for (const auto& pixel : img.data)
    {
        er = static_cast<int>(pixel.r);
        eg = static_cast<int>(pixel.g);
        eb = static_cast<int>(pixel.b);

        r = std::to_string(er);
        g = std::to_string(eg);
        b = std::to_string(eb);
        final_string += r + " " + g + " " + b;
        final_string += "\n";
    }

    file << final_string;
    return 0;
}