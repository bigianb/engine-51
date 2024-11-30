#pragma once
#include <cstdint>
#include <string>
#include "../VectorMath.h"

class AnimData
{
public:
    bool readFile(uint8_t* fileData, int len);

    BBox bbox;
    std::string filename;
    int version;
};

