#pragma once

#include <SDL3/SDL.h>
#include <map>

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
    void draw();

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
    void drawColour(const Colour& colour) override;
    void drawVertex(float x, float y, float z, float u, float v) override;

private:
    struct PositionTextureVertex
    {
        float x, y, z;
        float u, v;
    };

    struct Batch
    {
        SDL_GPUTexture* texture;
        SDL_GPUBuffer *vertexBuffer;
        SDL_GPUBuffer *indexBuffer;
        int numIndices;
        Colour colour;
    };

    Colour currentColour;

    std::vector<Batch> batches;

    std::vector<PositionTextureVertex> accumulatedVertices;

    SDL_Window*              window;
    SDL_GPUDevice*           device;
    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUSampler*          pointSampler;

    std::map<Bitmap*, SDL_GPUTexture*> gpuTextures;
    Bitmap* currentTex;
};
