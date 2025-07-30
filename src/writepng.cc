#include "writepng.hh"

RGBPNGWriter::RGBPNGWriter(const std::string file_name,
    const std::vector<std::vector<color>> &columns_source)
{
    fp = fopen(file_name.c_str(), "wb");
    columns = &columns_source;
}

RGBPNGWriter::~RGBPNGWriter()
{
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if (fp != nullptr)
    {
        fclose(fp);
    }
}

void RGBPNGWriter::write_png()
{
    if (fp == nullptr)
    {
        throw PNGWriteError();
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        nullptr, nullptr, nullptr);
    if (png_ptr == nullptr)
    {
        throw PNGReadError();
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr || setjmp(png_jmpbuf(png_ptr)))
    {
        throw PNGReadError();
    }
    png_init_io(png_ptr, fp);
    png_uint_32 width = columns->size();
    png_uint_32 height = columns->at(0).size();
    png_set_IHDR(png_ptr, info_ptr, width, height,
        BIT_DEPTH, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    png_byte image[height*width*BYTES_PER_PIXEL];
    png_bytep row_pointers[height];
    for (size_t y = 0; y < height; ++y)
    {
        row_pointers[y] = &image[y*width*BYTES_PER_PIXEL];
        for (size_t x = 0; x < width; ++x)
        {
            image[(y*width + x)*BYTES_PER_PIXEL] = columns->at(x).at(y).r;
            image[(y*width + x)*BYTES_PER_PIXEL + 1] = columns->at(x).at(y).g;
            image[(y*width + x)*BYTES_PER_PIXEL + 2] = columns->at(x).at(y).b;
        }
    }
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
}