#include <algorithm>
#include <argp.h>
#include <chrono>
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

static char DOC[] =
    "Boustrophedon Cell Decomposition -- split a binary image into regions";
static char ARGS_DOC[] = "INPUT_PNG OUTPUT_PNG";

static argp_option options[] = {
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

static error_t parse_opt(int key, char *arg, argp_state *state)
{
    arguments *args =
        static_cast<arguments *>(state->input);
    switch (key)
    {
        case 'v':
            args->verbose = true;
            break;
        case 'l':
            args->leeway = std::stoi(arg);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 2)
            {
                argp_usage(state);
            }
            args->args[state->arg_num] = arg;
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

static argp ARGP = { options, parse_opt, ARGS_DOC, DOC };

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
    std::vector<std::vector<color>> &color_columns)
{
    color c;
    switch (event.event_type)
    {
        case BCD_EVENT_START:
            c = {0x00, 0x99, 0xff};
            break;
        case BCD_EVENT_END:
            c = {0x99, 0x00, 0xff};
            break;
        case BCD_EVENT_MERGE:
            c = {0x00, 0xff, 0x99};
            break;
        case BCD_EVENT_SPLIT:
            c = {0xff, 0x00, 0x99};
            break;
        default:
            c = {0xff, 0xff, 0xff};
            break;
    }
    auto &left_col = color_columns.at(x);
    for (const slice &a : event.left)
    {
        std::fill(left_col.begin() + a.first,
            left_col.begin() + a.second, c);
    }
    auto &right_col = color_columns.at(x + 1);
    for (const slice &b : event.right)
    {
        std::fill(right_col.begin() + b.first,
            right_col.begin() + b.second, c);
    }
}

int main(int argc, char **argv)
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    arguments args;
    // Default values
    args.verbose = false;
    args.leeway = 2;
    // Parse arguments
    argp_parse(&ARGP, argc, argv, 0, 0, &args);
    // Print arguments
    if (args.verbose)
    {
        std::cout << "Input PNG: " << args.args[0] << std::endl
            << "Output PNG: " << args.args[1] << std::endl
            << "Overhang Leeway: " << args.leeway << std::endl;
    }

    auto t0 = high_resolution_clock::now();

    std::vector<std::vector<bool>> binary_columns;
    BinaryPNGReader reader(args.args[0], binary_columns);
    reader.read_png();

    size_t height = binary_columns.at(0).size();
    binary_columns.emplace(binary_columns.begin(), height, false);
    binary_columns.emplace_back(height, false);

    auto t1 = high_resolution_clock::now();

    auto events = find_all_events(binary_columns, true);

    auto t2 = high_resolution_clock::now();

    invalidate_overhangs(events, args.leeway);
    erase_invalid(events);

    size_t width = binary_columns.size();
    std::vector color_columns(width,
        std::vector<color>(height, {0x00, 0x00, 0x00}));

    auto t3 = high_resolution_clock::now();

    for (size_t x = 0; x < width; ++x)
    {
        auto &b_col = binary_columns.at(x);
        auto &c_col = color_columns.at(x);
        for (size_t y = 0; y < height; ++y)
        {
            if (b_col.at(y))
            {
                c_col.at(y) = {0xff, 0xff, 0xff};
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

    auto t4 = high_resolution_clock::now();

    RGBPNGWriter writer(args.args[1], color_columns);
    writer.write_png();

    auto t5 = high_resolution_clock::now();

    if (args.verbose)
    {
        std::cout << "Read PNG: "
            << duration_cast<milliseconds>(t1 - t0).count()
            << "ms" << std::endl;
        std::cout << "Find Events: "
            << duration_cast<milliseconds>(t2 - t1).count()
            << "ms" << std::endl;
        std::cout << "Erase Overhangs: "
            << duration_cast<milliseconds>(t3 - t2).count()
            << "ms" << std::endl;
        std::cout << "Fill Colors: "
            << duration_cast<milliseconds>(t4 - t3).count()
            << "ms" << std::endl;
        std::cout << "Write PNG: "
            << duration_cast<milliseconds>(t5 - t4).count()
            << "ms" << std::endl;
        std::cout << "Total: "
            << duration_cast<milliseconds>(t5 - t0).count()
            << "ms" << std::endl;
    }

    return 0;
}