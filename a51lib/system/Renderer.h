#pragma once

#include "../Colour.h"
#include "../VectorMath.h"
#include "../render/Render.h"

#define DRAW_2D (1 << 0)             // Default: 3D
#define DRAW_TEXTURED (1 << 1)       // Default: Not textured
#define DRAW_USE_ALPHA (1 << 2)      // Default: Alpha disabled
#define DRAW_WIRE_FRAME (1 << 3)     // Default: Solid/filled
#define DRAW_NO_ZBUFFER (1 << 4)     // Default: Z-Buffer
#define DRAW_NO_ZWRITE (1 << 5)      // Default: Z-Write enabled
#define DRAW_2D_KEEP_Z (1 << 6)      // Default: all on same Z plane close to camera
#define DRAW_BLEND_ADD (1 << 7)      // Default: Multiplicative (alpha) ADD and SUB mutually exclusive
#define DRAW_BLEND_SUB (1 << 8)      // Default: Multiplicative (alpha) ADD and SUB mutually exclusive
#define DRAW_U_CLAMP (1 << 9)        // Default: WRAP
#define DRAW_V_CLAMP (1 << 10)       // Default: WRAP
#define DRAW_KEEP_STATES (1 << 11)   // Default: Off (doesn't set any render/texture-stage states)
#define DRAW_XBOX_NO_BEGIN (1 << 12) // Default: Off
#define DRAW_XBOX_WRITE_A (1 << 13)  // Default: Off (writes into the frame-buffer alpha channel)

#define DRAW_CULL_NONE (1 << 29)   // Default: Cull CW
#define DRAW_CUSTOM_MODE (1 << 30) // Default: Off
#define DRAW_HAS_NORMAL (1 << 31)  // Default: has no normal

#define DRAW_UV_CLAMP (DRAW_U_CLAMP | DRAW_V_CLAMP)

class Bitmap;

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

    enum class Primitive
    {
        DRAW_POINTS,
        DRAW_LINES,
        DRAW_LINE_STRIPS,
        DRAW_TRIANGLES,
        DRAW_TRIANGLE_STRIPS,
        DRAW_QUADS,
        DRAW_RECTS,
        DRAW_SPRITES,
    };

    virtual void drawBegin(Primitive, int drawFlags) = 0;
    virtual void drawEnd() = 0;
    virtual void setTexture(Bitmap* tex) = 0;
    virtual void drawColour(const Colour& colour) = 0;
    virtual void drawVertex(float x, float y, float z, float u, float v) = 0;

    virtual void drawSpriteUV(const Vector3& Position, // Hot spot (2D Left-Top), (3D Center)
                              const Vector2& WH,       // (2D pixel W&H), (3D World W&H)
                              const Vector2& UV0,      // Upper Left   UV  [0.0 - 1.0]
                              const Vector2& UV1,      // Bottom Right UV  [0.0 - 1.0]
                              const Colour&  Color) = 0;
    virtual void drawColourRect(const IntRect& pos, const Colour& colour, bool isAdditive) = 0;

    virtual void BeginRigidGeom(Geom* pGeom, int iSubMesh) = 0;
    virtual void RenderRigidInstance(render_instance& Inst) = 0;
    virtual void EndRigidGeom(void) = 0;
    virtual void BeginSkinGeom(Geom* pGeom, int iSubMesh) = 0;
    virtual void RenderSkinInstance(render_instance& Inst) = 0;
    virtual void EndSkinGeom() = 0;
};
