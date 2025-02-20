#pragma once

#include "../../../a51lib/system/Renderer.h"


class QtRenderer : public Renderer
{
public:
    void begin() override {};
    void end() override {};

    void renderRect(const IntRect& rect, const Colour& colour, bool doWire) override
    {
    }

    void getRes(int& width, int& height) override
    {
        width = 640;
        height = 480;
    }

    void drawBegin(Primitive, int drawFlags) override;
    void drawEnd() override;
    void setTexture(Bitmap* tex) override;
    void drawUV(float u, float v) override;
    void drawColour(const Colour& colour) override;
    void drawVertex(float x, float y, float z) override;
};
