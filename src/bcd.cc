#include <algorithm>
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

void invalidate_start_merge(BCDEvent &a, size_t x,
    std::vector<std::vector<BCDEvent>> &events, int leeway)
{
    slice &a_slice = a.right.at(0);
    for (size_t i = 1; i <= leeway && x + i < events.size(); ++i)
    {
        for (BCDEvent &b : events.at(x + i))
        {
            for (size_t j = 0; j < b.left.size(); ++j)
            {
                slice &b_slice = b.left.at(j);
                if (a_slice.first < b_slice.second && b_slice.first < a_slice.second)
                {
                    if (b.event_type == BCD_EVENT_MERGE)
                    {
                        a.left.clear();
                        a.right.clear();
                        if (b.left.size() <= 2)
                        {
                            b.left.clear();
                            b.right.clear();
                        }
                        else
                        {
                            b.left.erase(b.left.begin() + j);
                        }
                    }
                    return;
                }
            }
        }
    }
}

void invalidate_split_end(BCDEvent &a, size_t x,
    std::vector<std::vector<BCDEvent>> &events, int leeway)
{
    slice &a_slice = a.left.at(0);
    for (size_t i = 1; i <= leeway && x - i >= 0; ++i)
    {
        for (BCDEvent &b : events.at(x - i))
        {
            for (size_t j = 0; j < b.right.size(); ++j)
            {
                slice &b_slice = b.right.at(j);
                if (a_slice.first < b_slice.second && b_slice.first < a_slice.second)
                {
                    if (b.event_type == BCD_EVENT_SPLIT)
                    {
                        a.left.clear();
                        a.right.clear();
                        if (b.right.size() <= 2)
                        {
                            b.left.clear();
                            b.right.clear();
                        }
                        else
                        {
                            b.right.erase(b.right.begin() + j);
                        }
                    }
                    return;
                }
            }
        }
    }
}

void invalidate_overhangs(std::vector<std::vector<BCDEvent>> &events,
    int leeway)
{
    for (size_t x = 0; x < events.size(); ++x)
    {
        for (BCDEvent &a : events.at(x))
        {
            if (a.event_type == BCD_EVENT_START)
            {
                invalidate_start_merge(a, x, events, leeway);
            }
            else if (a.event_type == BCD_EVENT_END)
            {
                invalidate_split_end(a, x, events, leeway);
            }
            
        }
    }
}

void erase_invalid(std::vector<std::vector<BCDEvent>> &events)
{
    for (auto &column : events)
    {
        column.erase(std::remove_if(column.begin(), column.end(),
            [](BCDEvent &event) {
                return event.left.empty() && event.right.empty();
            }), column.end());
    }
}

void color_event(const BCDEvent &event, size_t x,
    std::vector<std::vector<struct color>> &color_columns)
{
    struct color color = {0x00, 0x00, 0x00};
    switch (event.event_type)
    {
        case BCD_EVENT_START:
            color.g = 0x99;
            color.b = 0xff;
            break;
        case BCD_EVENT_END:
            color.r = 0x99;
            color.b = 0xff;
            break;
        case BCD_EVENT_MERGE:
            color.g = 0xff;
            color.b = 0x99;
            break;
        case BCD_EVENT_SPLIT:
            color.r = 0xff;
            color.b = 0x99;
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
    auto events = find_all_events(binary_columns, true);
    invalidate_overhangs(events, arguments.leeway);
    erase_invalid(events);
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