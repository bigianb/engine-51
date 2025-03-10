#pragma once

#include <SDL3/SDL.h>
#include <map>
#include <vector>

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
    void drawSpriteUV(const Vector3& Position, // Hot spot (2D Left-Top), (3D Center)
                      const Vector2& WH,       // (2D pixel W&H), (3D World W&H)
                      const Vector2& UV0,      // Upper Left   UV  [0.0 - 1.0]
                      const Vector2& UV1,      // Bottom Right UV  [0.0 - 1.0]
                      const Colour&  Color) override;
    void drawColourRect(const IntRect& pos, const Colour& colour, bool isAdditive) override;

private:
    void flushTextureVertices();
    void flushColourVertices();
    void drawColourVertex(float x, float y, float z, const Colour& c);

    Primitive primitive;
    int       drawFlags;
    bool      is2D;
    bool      isTextured;

    struct PositionTextureVertex
    {
        float x, y, z;
        float u, v;
    };

    struct PositionColourVertex
    {
        float x, y, z;
        float r, g, b, a;
    };

    struct Batch
    {
        SDL_GPUGraphicsPipeline* pipeline;
        SDL_GPUTexture* texture;
        SDL_GPUBuffer*  vertexBuffer;
        SDL_GPUBuffer*  indexBuffer;
        int             numIndices;
        Colour          colour;
    };

    Colour currentColour;

    std::vector<Batch> batches;

    std::vector<PositionTextureVertex> accumulatedVertices;
    std::vector<PositionColourVertex> accumulatedColourVertices;

    SDL_Window*              window;
    SDL_GPUDevice*           device;
    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUGraphicsPipeline* colourPipeline;
    SDL_GPUSampler*          pointSampler;

    std::map<Bitmap*, SDL_GPUTexture*> gpuTextures;
    Bitmap*                            currentTex;
};
