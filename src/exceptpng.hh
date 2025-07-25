#ifndef EXCEPTPNG_HH
#define EXCEPTPNG_HH

#include <exception>

class PNGReadError : std::exception
{
    virtual const char *what() const noexcept;
};

class PNGWriteError : std::exception
{
    virtual const char *what() const noexcept;
};

class PNGNotInitializedError : std::exception
{
    virtual const char *what() const noexcept;
};

class PNGUnsupportedError : std::exception
{
    virtual const char *what() const noexcept;
};

#endif