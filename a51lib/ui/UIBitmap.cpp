#include "UIBitmap.h"

#include <iostream>
#include <cassert>

#include "../Colour.h"
#include "../system/Renderer.h"

ui::BitmapControl::BitmapControl()
{
    labelColor = COLOR_WHITE;
}

bool ui::BitmapControl::create(User*          user,
                               Manager*       manager,
                               const IntRect& position,
                               Window*        parent,
                               int            flags)
{
    bool success = ui::Control::create(user, manager, position, parent, flags);

    bitmap = nullptr;
    isElement = false;
    renderState = 0;

    return success;
}

void ui::BitmapControl::setBitmap(Bitmap* b)
{
    bitmap = b;
}

void ui::BitmapControl::render(Renderer& renderer, int ox, int oy)
{
    if (!visible()) {
        return;
    }

    IntRect r((position.left + ox), (position.top + oy), (position.right + ox), (position.bottom + oy));

    if (bitmap != nullptr) {
        if (isElement) {
            assert(false);
 //           getUIManger()>renderElement(renderer, bitmapID, r, renderState, getLabelColour());
        } else {
            renderer.drawBegin( Renderer::Primitive::DRAW_TRIANGLES, DRAW_2D|DRAW_TEXTURED|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER|DRAW_UV_CLAMP );
            renderer.setTexture(bitmap); 
            //draw_DisableBilinear();
            renderer.drawColour(getLabelColour());
            renderer.drawVertex(r.left, r.top, 0.0f, 0, 0);
            renderer.drawVertex(r.right, r.top, 0.0f, 1.0, 0);
            renderer.drawVertex(r.left, r.bottom, 0.0f, 0, 1.0);

            renderer.drawVertex(r.left, r.bottom, 0.0f, 0, 1.0);
            renderer.drawVertex(r.right, r.bottom, 0.0f, 1.0, 1.0);
            renderer.drawVertex(r.right, r.top, 0.0f, 1.0, 0);
            
            renderer.drawEnd();
        }
    }

    for (Window* child : children) {
        child->render(renderer, getPosition().left + ox, getPosition().top + oy);
    }
}
