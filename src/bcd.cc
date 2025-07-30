#include <argp.h>
#include <iostream>
#include <png.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <zlib.h>

#include "readpng.hh"
#include "slices.hh"
#include "writepng.hh"

const char *argp_program_version = "bcd 0.1";
const char *argp_program_bug_address = "<em@embermckinney.com>";

static char doc[] =
    "Boustrophedon Cell Decomposition -- split a binary image into regions";
static char args_doc[] = "INPUT_PNG OUTPUT_PNG";

static struct argp_option options[] = {
    { "verbose", 'v', 0, 0, "Produce verbose output" },
    { "overhang-leeway", 'l', "PIXELS", 0,
        "Number of pixels leeway for overhangs" },
    { 0 }
};

struct arguments
{
    char *args[2];
    bool verbose;
    int leeway;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments =
        static_cast<struct arguments *>(state->input);
    switch (key)
    {
        case 'v':
            arguments->verbose = true;
            break;
        case 'l':
            arguments->leeway = std::stoi(arg);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 2)
            {
                argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 2)
            {
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void color_event(const BCDEvent &event, size_t x,
    std::vector<std::vector<struct color>> &color_columns)
{
    struct color color = {0x00, 0x00, 0x00};
    switch (event.event_type)
    {
        case BCD_EVENT_START:
            color.g = 0xff;
            break;
        case BCD_EVENT_END:
            color.r = 0xff;
            break;
        case BCD_EVENT_MERGE:
            color.g = color.b = 0xff;
            break;
        case BCD_EVENT_SPLIT:
            color.r = color.b = 0xff;
            break;
    }
    for (const slice &a : event.left)
    {
        for (size_t y = a.first; y < a.second; ++y)
        {
            color_columns.at(x).at(y) = color;
        }
    }
    for (const slice &b : event.right)
    {
        for (size_t y = b.first; y < b.second; ++y)
        {
            color_columns.at(x + 1).at(y) = color;
        }
    }
}

int main(int argc, char **argv)
{
    struct arguments arguments;
    // Default values
    arguments.verbose = false;
    arguments.leeway = 2;
    // Parse arguments
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    // Print arguments
    if (arguments.verbose)
    {
        std::cout << "Input PNG: " << arguments.args[0] << std::endl
            << "Output PNG: " << arguments.args[1] << std::endl
            << "Overhang Leeway: " << arguments.leeway << std::endl;
    }
    std::vector<std::vector<bool>> binary_columns;
    BinaryPNGReader reader(arguments.args[0], binary_columns);
    reader.read_png();
    size_t height = binary_columns.at(0).size();
    binary_columns.emplace(binary_columns.begin(), height, false);
    binary_columns.emplace_back(height, false);
    const auto events = find_all_events(binary_columns, true);
    size_t width = binary_columns.size();
    std::vector<std::vector<struct color>> color_columns(width,
        std::vector<struct color>(height, {0x00, 0x00, 0x00}));
    for (size_t x = 0; x < width; ++x)
    {
        for (size_t y = 0; y < height; ++y)
        {
            if (binary_columns.at(x).at(y))
            {
                struct color &color = color_columns.at(x).at(y);
                color.r = color.g = color.b = 0xff;
            }
        }
        if (x > 0)
        {
            for (const BCDEvent &event : events.at(x - 1))
            {
                color_event(event, x - 1, color_columns);
            }
        }
    }
    RGBPNGWriter writer(arguments.args[1], color_columns);
    writer.write_png();
    return 0;
}