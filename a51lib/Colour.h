#pragma once
#include <cstdint>

class Colour
{
public:
    uint8_t b, g, r, a;

    void clear(){
        b = g = r = a = 0;
    }

    void set(uint8_t rIn, uint8_t gIn, uint8_t bIn, uint8_t aIn)
    {
        b = bIn;
        g = gIn;
        r = rIn;
        a = aIn;
    }

    void set565(uint16_t col565)
    {
        a = 0xff;
        r = (uint8_t)((col565 >> 11) * 255 / 0x1f);
        g = (uint8_t)(((col565 >> 5) & 0x3f) * 255 / 0x3f);
        b = (uint8_t)((col565 & 0x1f) * 255 / 0x1f);
    }
};
