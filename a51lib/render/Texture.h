#pragma once

#include "../resourceManager/ResourceManager.h"
#include "../Bitmap.h"

class texture
{
public:
    texture(void);

    static void GetStats(int* pNumTextureLoaded, int* pTextureMemorySize);

    typedef ResourceHandle<texture> handle;

    Bitmap m_Bitmap;

    friend struct texture_loader;
};

class cubemap
{
public:
    cubemap();

    enum sides
    {
        TOP = 0,
        BOTTOM,
        FRONT,
        BACK,
        LEFT,
        RIGHT
    };

    typedef ResourceHandle<cubemap> handle;

    void* m_hTexture;

    Bitmap m_Bitmap[6];

    friend struct cubemap_loader;
};
