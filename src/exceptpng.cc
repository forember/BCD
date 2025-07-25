#include "exceptpng.hh"

const char *PNGReadError::what() const noexcept
{
    return "Error reading PNG file";
}

const char *PNGWriteError::what() const noexcept
{
    return "Error writing PNG file";
}

const char *PNGNotInitializedError::what() const noexcept
{
    return "PNG reader not initialized";
}

const char *PNGUnsupportedError::what() const noexcept
{
    return "Unsupported PNG type";
}