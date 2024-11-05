#pragma once
#include <cstdint>

class Colour
{
public:
    uint8_t b, g, r, a;

    void set(uint8_t rIn, uint8_t gIn, uint8_t bIn, uint8_t aIn)
    {
        b = bIn;
        g = gIn;
        r = rIn;
        a = aIn;
    }
};
