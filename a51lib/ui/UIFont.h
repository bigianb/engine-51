#pragma once

#include <cstdint>
#include <vector>
#include "../Bitmap.h"

namespace ui
{
    class Font
    {
    public:
        Font() : bitmap(nullptr) {};

        bool readFile(uint8_t* fileData, int len);

        struct Glyph
        {
            uint16_t x;
            uint16_t y;
            uint16_t w;
        };

        struct Charmap
        {
            uint16_t character;
            uint16_t glyph;
        };

        int lineHeight;

        std::vector<Glyph>   glyphs;
        std::vector<Charmap> charmaps;

        Bitmap* bitmap;
    };
}
