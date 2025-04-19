#pragma once
#include <cstdint>
#include <algorithm>

class Colour
{
public:
    uint8_t b, g, r, a;

    Colour()
        : r(255)
        , b(255)
        , g(255)
        , a(255)
    {
    }
    Colour(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red)
        , b(blue)
        , g(green)
        , a(alpha)
    {
    }

    bool operator==(const Colour& other) const
    {
        return b == other.b && g == other.g && r == other.r && a == other.a;
    }

    void clear()
    {
        b = g = r = a = 0;
    }

    void set(uint8_t rIn, uint8_t gIn, uint8_t bIn, uint8_t aIn = 255)
    {
        b = bIn;
        g = gIn;
        r = rIn;
        a = aIn;
    }

    const Colour& operator=(const Colour& C)
    {
        a = C.a;
        r = C.r;
        g = C.g;
        b = C.b;
        return *this;
    }

    const Colour& operator=(uint32_t argb)
    {
        a = (uint8_t)((argb & 0xFF000000) >> 24);
        r = (uint8_t)((argb & 0x00FF0000) >> 16);
        g = (uint8_t)((argb & 0x0000FF00) >> 8);
        b = (uint8_t)((argb & 0x000000FF) >> 0);

        return *this;
    }

    Colour& operator+=(Colour C)
    {
        r = (uint8_t)std::min(255, r + (int)C.r);
        g = (uint8_t)std::min(255, g + (int)C.g);
        b = (uint8_t)std::min(255, b + (int)C.b);
        return *this;
    }

    void set565(uint16_t col565)
    {
        a = 0xff;
        r = (uint8_t)((col565 >> 11) * 255 / 0x1f);
        g = (uint8_t)(((col565 >> 5) & 0x3f) * 255 / 0x3f);
        b = (uint8_t)((col565 & 0x1f) * 255 / 0x1f);
    }

    uint32_t getRGBA() const
    {
        return ( g << 24 ) | ( g << 16 ) | ( b << 8 ) | a;
    }
};

#define COLOR_BLACK Colour(0, 0, 0)
#define COLOR_WHITE Colour(255, 255, 255)
#define COLOR_RED Colour(255, 0, 0)
#define COLOR_GREEN Colour(0, 255, 0)
#define COLOR_BLUE Colour(0, 0, 255)
#define COLOR_AQUA Colour(0, 255, 255)
#define COLOR_PURPLE Colour(255, 0, 255)
#define COLOR_YELLOW Colour(255, 255, 0)
#define COLOR_GREY Colour(127, 127, 127)
