#pragma once

#include "../../../a51lib/system/Renderer.h"

class SDL_Window;
class SDL_GPUDevice;

class SDLRenderer : public Renderer
{
public:
    SDLRenderer();
    void begin() override {};
    void end() override {};

    bool init();
    void quit();
    
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

private:
    SDL_Window*    window;
    SDL_GPUDevice* device;
};
