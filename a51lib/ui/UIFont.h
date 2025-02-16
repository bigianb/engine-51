#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "../Bitmap.h"
#include "../Colour.h"
#include "../VectorMath.h"

class Renderer;

namespace ui
{
    class Font
    {
    public:
        Font() : bitmap(nullptr) {};

        bool readFile(uint8_t* fileData, int len);

        void renderText(Renderer& renderer, const IntRect& pos, int flags, Colour textColor, std::wstring text, bool ignoreEmbeddedColor = true, bool useGradient = true, float flareAmount = 0.0) const;

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
