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
        Font()
            : bitmap(nullptr) {};

        bool readFile(uint8_t* fileData, int len);

        int lookUpCharacter(int) const;

        void renderText(Renderer& renderer, const IntRect& pos, int flags, Colour textColor, const wchar_t* text, bool ignoreEmbeddedColor = true, bool useGradient = true, float flareAmount = 0.0) const;
        int  textWidth(const wchar_t* text, int count = -1) const;
        int  textHeight(const wchar_t* text, int count = -1) const;

        enum EFlags
        {
            h_left = 0x0001,
            h_center = 0x0002,
            h_right = 0x0004,
            v_top = 0x0008,
            v_center = 0x0010,
            v_bottom = 0x0020,
            clip_character = 0x0040,
            clip_l_justify = 0x0080,
            clip_r_justify = 0x0100,
            clip_ellipsis = 0x0200,
            is_help_text = 0x0400,
            set_position = 0x0800,
            blend_additive = 0x1000,
        };

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
