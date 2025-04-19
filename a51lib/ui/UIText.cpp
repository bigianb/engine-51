#include "UIText.h"

#include <iostream>

#include "../Colour.h"
#include "../system/Renderer.h"

void ui::Text::render(Renderer& renderer, int ox, int oy)
{
    if (!visible()) {
        return;
    }

    IntRect     pos(position.left + ox, position.top + oy, position.right + ox, position.bottom + oy);
    Colour      textColor1 = COLOR_WHITE;
    const char* fontName = useSmallText ? "small" : "large";
    getUIManger()->renderText(renderer, fontName, pos, labelFlags, textColor1, label);
}
