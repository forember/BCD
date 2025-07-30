#ifndef WRITEPNG_HH
#define WRITEPNG_HH

#include <png.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "exceptpng.hh"

struct color
{
    png_byte r;
    png_byte g;
    png_byte b;
};

class RGBPNGWriter
{
private:
    static const int BIT_DEPTH = 8;
    static const int BYTES_PER_PIXEL = 3;
    FILE *fp = nullptr;
    const std::vector<std::vector<color>> *columns;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

public:
    RGBPNGWriter(const std::string file_name,
        const std::vector<std::vector<color>> &columns_source);
    ~RGBPNGWriter();
    void write_png();
};

#endif