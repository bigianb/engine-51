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

    IntRect r((getPositon().left + x), (getPositon().top + y), (getPositon().right + x), (getPositon().bottom + y));

    Colour color = COLOR_WHITE;
    if (disabled()) {
        color = COLOR_GREY;
    } else if (highlighted() || selected()) {
        color = COLOR_RED;
    }

    renderer.renderRect(r, color, true);

    // Render children
    for (Window* child : children) {
        child->render(renderer, getPositon().left + x, getPositon().top + y);
    }
}
