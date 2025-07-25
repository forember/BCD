#ifndef READPNG_HH
#define READPNG_HH

#include <png.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "exceptpng.hh"

class BinaryPNGReader
{
private:
    static const int SIG_READ = 8;
    int sig_num_bytes = 0;
    FILE *fp = nullptr;
    std::vector<std::vector<bool>> *columns;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    bool check_if_png();
    int get_bytes_per_pixel();
    void load_columns();

public:
    BinaryPNGReader(const std::string file_name,
        std::vector<std::vector<bool>> &columns_target);
    ~BinaryPNGReader();
    void read_png();
};

#endif