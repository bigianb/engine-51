#pragma once

/*
 Generic renderer. Implement system specific versions.
*/
class Renderer
{
public:
    virtual ~Renderer() {}
    virtual void begin() = 0;
    virtual void end() = 0;

    virtual void getRes(int& width, int& height) = 0;

    virtual void renderRect(const IntRect& rect, const Colour& colour, bool doWire) = 0;
    virtual void renderText(const char* fontName, IntRect& r, unsigned int flags, Colour colour, std::wstring text) = 0;
};
