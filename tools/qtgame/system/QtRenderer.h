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

    void renderText(const char* fontName, IntRect& r, unsigned int flags, Colour colour, std::wstring text) override
    {

    }
};
