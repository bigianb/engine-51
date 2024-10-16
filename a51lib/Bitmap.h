#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>

class Bitmap
{
public:
    Bitmap();
    ~Bitmap();

    bool readFile(uint8_t* fileData, int len);

    void describe(std::ostringstream& ss);

    int          dataSize;
    int          clutSize;
    int          width;
    int          height;
    int          physicalWidth;
    unsigned int flags;
    int          nMips;
    int          format;

    uint8_t* pixelData;
    uint8_t* clutData;
};
