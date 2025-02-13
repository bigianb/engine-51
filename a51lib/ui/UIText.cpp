#include "UIText.h"

#include <iostream>

#include "../Colour.h"
#include "../system/Renderer.h"

void ui::Text::render(Renderer& renderer, int ox, int oy)
{
    if (!visible()) {
        return;
    }

    std::cerr << "render text control" << std::endl;

    IntRect     pos(position.left + ox, position.top + oy, position.right + ox, position.bottom + oy);
    Colour      textColor1 = COLOR_WHITE;
    const char* fontName = useSmallText ? "small" : "large";
    renderer.renderText(fontName, pos, labelFlags, textColor1, label);
}
