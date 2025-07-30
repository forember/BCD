# Boustrophedon Cell Decomposition

Splits a binary image representing free space and obstacles into regions that can be covered using simple boustrophedon coverage.
Mostly did this to brush up on C++ in preparation of resuming my CS degree at Winthrop University.

## Dependencies

* [libpng](https://libpng.org/pub/png/libpng.html)
* (build) [CMake](https://cmake.org/)

## Building

To build, just run `scripts/build.sh`. Make sure you have a C++ compiler, libpng headers, and CMake installed.

## Usage

```
bcd [-l LEEWAY] INPUT_PNG OUTPUT_PNG
```

* `INPUT_PNG`: PNG image file with white as open space and black as obstacles.
* `OUTPUT_PNG`: PNG image file to write partitioned map to.
* `LEEWAY`: Number of pixels leeway to give overhangs.
