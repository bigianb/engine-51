#include "UIControl.h"
#include "../Colour.h"

#include "../system/Renderer.h"

ui::Control::Control()
    : navPos(0, 0, 0, 0)
{
}

void ui::Control::render(Renderer& renderer, int x, int y)
{
    if (!visible()) {
        return;
    }

    IntRect r((getPosition().left + x), (getPosition().top + y), (getPosition().right + x), (getPosition().bottom + y));

    Colour color = COLOR_WHITE;
    if (disabled()) {
        color = COLOR_GREY;
    } else if (highlighted() || selected()) {
        color = COLOR_RED;
    }

    renderer.renderRect(r, color, true);

    // Render children
    for (Window* child : children) {
        child->render(renderer, getPosition().left + x, getPosition().top + y);
    }
}
