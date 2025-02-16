#include "UIFont.h"
#include "../DataReader.h"

#include <iostream>

bool ui::Font::readFile(uint8_t* fileData, int len)
{
    DataReader reader(fileData, len);
    int cmapSize = reader.readUInt16();
    int numGlyphs = reader.readUInt16();
    lineHeight = reader.readUInt16();

    for (int i = 0; i < cmapSize; ++i) {
        Charmap cm;
        cm.character = reader.readUInt16();
        cm.glyph = reader.readUInt16();
        charmaps.push_back(cm);
    }

    for (int i = 0; i < numGlyphs; ++i) {
        Glyph g;
        g.x = reader.readUInt16();
        g.y = reader.readUInt16();
        g.w = reader.readUInt16();
        glyphs.push_back(g);
    }
    return true;
}

void ui::Font::renderText(Renderer& renderer, const IntRect& pos, int flags, Colour textColor, std::wstring text, bool ignoreEmbeddedColor, bool useGradient, float flareAmount) const
{

}
