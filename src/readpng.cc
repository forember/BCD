#include "readpng.hh"

BinaryPNGReader::BinaryPNGReader(const std::string file_name,
    std::vector<std::vector<bool>> &columns_target)
{
    fp = fopen(file_name.c_str(), "rb");
    columns = &columns_target;
}

BinaryPNGReader::~BinaryPNGReader()
{
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    if (fp != nullptr)
    {
        fclose(fp);
    }
}

bool BinaryPNGReader::check_if_png()
{
    if (fp == nullptr)
    {
        return false;
    }
    png_byte buf[SIG_READ];
    int nread = fread(buf, 1, SIG_READ, fp);
    sig_num_bytes += nread;
    if (nread != SIG_READ ||
        png_sig_cmp(buf, static_cast<png_size_t>(0), SIG_READ))
    {
        return false;
    }
    return true;
}

void BinaryPNGReader::read_png()
{
    if (!check_if_png())
    {
        throw PNGReadError();
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
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
    png_set_sig_bytes(png_ptr, sig_num_bytes);
    int transforms = PNG_TRANSFORM_STRIP_16 |
        PNG_TRANSFORM_STRIP_ALPHA |
        PNG_TRANSFORM_PACKING |
        PNG_TRANSFORM_GRAY_TO_RGB;
    png_read_png(png_ptr, info_ptr, transforms, nullptr);
    load_columns();
}

int BinaryPNGReader::get_bytes_per_pixel()
{
    if (png_ptr == nullptr || info_ptr == nullptr)
    {
        throw PNGNotInitializedError();
    }
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    if (bit_depth != 8)
    {
        throw PNGUnsupportedError();
    }
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    if (color_type != PNG_COLOR_TYPE_RGB)
    {
        throw PNGUnsupportedError();
    }
    return 3;
}

void BinaryPNGReader::load_columns()
{
    int bpp = get_bytes_per_pixel();
    png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    png_bytepp rows = png_get_rows(png_ptr, info_ptr);
    columns->assign(width, std::vector<bool>(height, false));
    for (size_t x = 0; x < width; ++x)
    {
        for (size_t y = 0; y < height; ++y)
        {
            int r = rows[y][x*bpp];
            int g = rows[y][x*bpp+1];
            int b = rows[y][x*bpp+2];
            if ((r + g + b) >= 383)
            {
                columns->at(x).at(y) = true;
            }
        }
    }
}