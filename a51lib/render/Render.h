#pragma once

#include "../resourceManager/ResourceManager.h"
#include "Material.h"
#include "Texture.h"
#include "RigidGeom.h"
#include "SkinGeom.h"
#include "../Bitmap.h"
#include "../xfiles/xHandle.h"

#include <cassert>

union sortkey
{
    struct
    {
        uint32_t GeomSubMesh : 8;
        uint32_t GeomHandle : 9;
        uint32_t GeomType : 1;
        uint32_t MatIndex : 10;
        uint32_t RenderOrder : 3;
    };
    uint32_t Bits;
};

union shad_sortkey
{
    struct
    {
        uint32_t ProjectorIndex : 6;
        uint32_t GeomSubMesh : 8;
        uint32_t GeomHandle : 9;
        uint32_t GeomType : 1;
        uint32_t ShadType : 1; // cast or receive
    };
    uint32_t Bits;
};

enum geom_type
{
    TYPE_RIGID = 0,
    TYPE_SKIN,
    TYPE_UNKNOWN
};

struct rigid_data
{
    RigidGeom*     pGeom;
    const Matrix4* pL2W;
    const void*    pColInfo;
};

struct skin_data
{
    SkinGeom*      pGeom;
    const Matrix4* pBones;
    uint32_t       Pad;
};

union instance_data
{
    rigid_data Rigid;
    skin_data  Skin;
};

struct render_instance
{
    union
    {
        shad_sortkey ShadSortKey;
        sortkey      SortKey;
    };
    uint32_t      Flags;
    void*         pLighting;
    instance_data Data;

    uint8_t UOffset;
    uint8_t VOffset;
    uint8_t Alpha;
    uint8_t OverrideMat; // such as distortion

    // information for the hash table
    int16_t Brother;
    int16_t Next;

#ifdef TARGET_PC
    xhandle hDList;
#endif // TARGET_PC
};

class Renderer;

namespace render
{

    // startup and shutdown routines
    void Init(ResourceManager*, Renderer* renderer );
    void Kill();

    // update routines (needed for uv and texture animation)
    void Update(float DeltaTime);

    // routines for rendering raw triangle data or sprite data
    enum
    {
        BLEND_MODE_ADDITIVE = 0,
        BLEND_MODE_SUBTRACTIVE,
        BLEND_MODE_NORMAL,
        BLEND_MODE_INTENSITY
    };
    int  GetHardwareBufferSize();
    void StartRawDataMode();
    void EndRawDataMode();
    void RenderRawStrips(int             nVerts,
                         const Matrix4&  L2W,
                         const Vector4*  pPos,
                         const int16_t*  pUV,
                         const uint32_t* pColor);
    void Render3dSprites(int             nSprites,
                         float           UniScale,
                         const Matrix4*  pL2W,
                         const Vector4*  pPositions, // w contains 0 if particle is active, otherwise 0x8000
                         const Vector2*  pRotScales, // array of Vector2(rotation,scale)
                         const uint32_t* pColors);
    void RenderVelocitySprites(int             nSprites,
                               float           UniScale,
                               const Matrix4*  pL2W,
                               const Matrix4*  pVelMatrix,  // a velocity matrix that will be combined with l2w
                               const Vector4*  pPositions,  // w contains 0 if particle is active, otherwise 0x8000
                               const Vector4*  pVelocities, // w contains scale
                               const uint32_t* pColors);
    void RenderHeatHazeSprites(int             nSprites,
                               float           UniScale,
                               const Matrix4*  pL2W,
                               const Vector4*  pPositions,
                               const Vector2*  pRotScales,
                               const uint32_t* pColors);
    void SetDiffuseMaterial(const Bitmap& Bitmap,
                            int           BlendMode,
                            bool          ZTestEnabled = true);
    void SetGlowMaterial(const Bitmap& Bitmap,
                         int           BlendMode,
                         bool          ZTestEnabled = true);
    void SetEnvMapMaterial(const Bitmap& Bitmap,
                           int           BlendMode,
                           bool          ZTestEnabled = true);
    void SetDistortionMaterial(int  BlendMode,
                               bool ZTestEnabled = true);

    // data registration routines
    typedef xhandle hgeom_inst;

    hgeom_inst      RegisterRigidInstance(RigidGeom& geom, ResourceManager* rm);
    void            UnregisterRigidInstance(hgeom_inst hInst);
    hgeom_inst      RegisterSkinInstance(SkinGeom& geom, ResourceManager* rm);
    void            UnregisterSkinInstance(hgeom_inst hInst);
    const Geom*     GetGeom(hgeom_inst hInst);

    // functions for setting a projected texture (only one allowed--use it for
    // a flashlight!)
    void SetTextureProjection(const Matrix4&         L2W,
                              Radian                 FOV,
                              float                  Length,
                              const texture::handle& Texture);

    // projected shadows that cast onto both players and the world
    enum
    {
        MAX_SHADOW_PROJECTORS = 2
    };
    void SetShadowProjection(const Matrix4&         L2W,
                             Radian                 FOV,
                             float                  Length,
                             const texture::handle& Texture);

    // send these flags to describe how the object should be rendered.
    // most of the time you should just be using 0 and possibly CLIPPED,
    // as the other flags are not guaranteed to work on all platforms (they
    // are intended for use by the editor)
    enum
    {
        WIREFRAME = 0x00000001,
        WIREFRAME2 = 0x00000002,
        PULSED = 0x00000004,
        SHADOW_PASS = 0x00000008,
        GLOWING = 0x00000010,
        FADING_ALPHA = 0x00000020,
        CLIPPED = 0x00000040,
        FORCE_LAST = 0x01000000,
        DISABLE_SPOTLIGHT = 0x02000000,
        DISABLE_FILTERLIGHT = 0x04000000,
        DISABLE_PROJ_SHADOWS = 0x08000000,
        DO_SIMPLE_LIGHTING = 0x10000000, // for optimization--enables the lighting for AddRigidInstanceSimple

        // Xbox per pixel lighting:
        PERPIXEL_POINTLIGHT = 0x20000000,

        // TODO: Our flags have started clashing. For now, if we make INSTFLAG_CLIPPED
        // the same as CLIPPED we are fine, but these flags really need to be re-thought.
        // Preferably, the render flags should be completely separated from the instance
        // flags, and the render system can internally do whatever it needs to do to make
        // things work.
        // these instance flags are considered private. don't look!
        INSTFLAG_CLIPPED = 0x00000080,       // Does the instance intersect with the frustum?
        INSTFLAG_GLOWING = 0x00000100,       // we have forced something that doesn't normally glow to glow
        INSTFLAG_SHADOW_PASS = 0x00000200,   // we are receiving dynamic shadows
        INSTFLAG_FILTERLIGHT = 0x00000400,   // modulate vertex lighting (i.e. emergency red light situation)
        INSTFLAG_SPOTLIGHT = 0x00000800,     // The instance receives a spotlight projection (flashlight)
        INSTFLAG_FADING_ALPHA = 0x00001000,  // the geometry is fading out
        INSTFLAG_DYNAMICLIGHT = 0x00002000,  // dynamic lighting is on (point or directional
        INSTFLAG_DETAIL = 0x00004000,        // detail mapping is on (material still has it, but the object is distant)
        INSTFLAG_PROJ_SHADOW_1 = 0x00010000, // we are receiving the first projected shadow
        INSTFLAG_PROJ_SHADOW_2 = 0x00020000, // we are receiving the second projected shadow
    };

    // shadow creation routines--we can handle up to 64 shadow textures, that way
    // each caster should be able to have it's own texture to reduce aliasing
    // The projection mask says which receivers receive which textures,
    // and also which casters cast into which textures.
    enum
    {
        MAX_SHADOW_CASTERS = 64,
    };
    void BeginShadowCreation();
    void EndShadowCreation();
    void AddPointShadowProjection(const Matrix4& L2W,
                                  Radian         FOV,
                                  float          NearZ,
                                  float          FarZ);
    void AddDirShadowProjection(const Matrix4& L2W,
                                float          Width,
                                float          Height,
                                float          NearZ,
                                float          FarZ);
    void AddRigidCasterSimple(hgeom_inst     hInst,
                              const Matrix4* pL2W,
                              uint64_t       ProjMask);
    void AddRigidCaster(hgeom_inst     hInst,
                        const Matrix4* pL2W,
                        uint64_t       Mask,
                        uint64_t       ProjMask);
    void AddSkinCaster(hgeom_inst     hInst,
                       const Matrix4* pBone,
                       uint64_t       Mask,
                       uint64_t       ProjMask);
    void AddRigidReceiverSimple(hgeom_inst     hInst,
                                const Matrix4* pL2W,
                                uint32_t       Flags,
                                uint64_t       ProjMask);
    void AddRigidReceiver(hgeom_inst     hInst,
                          const Matrix4* pL2W,
                          uint64_t       Mask,
                          uint32_t       Flags,
                          uint64_t       ProjMask);
    void AddSkinReceiver(hgeom_inst     hInst,
                         const Matrix4* pBone,
                         uint64_t       Mask,
                         uint32_t       Flags,
                         uint64_t       ProjMask);

    // basic instance-rendering routines
    // you should call them in this order:
    // BeginNormalRender()
    //   for all instances:
    //   AddRigidInstanceSimple  OR
    //   AddRigidInstance        OR
    //   AddSkinInstance         OR
    //   AddSkinInstanceDistorted
    // EndNormalRender()
    // BeginCustomRender()
    //   Render custom 3d stuff that will write to the z-buffer (such as decals)
    //   BeginMidPostEffects()
    //     Render any post-effects that you want to take place before distortion.
    //     For example, if you want distorted materials to distort the fogged objects
    //     behind them, render the fog.
    //   EndMidPostEffects()
    // EndCustomRender()
    // Render custom 3d stuff that will not write to the z-buffer (such as particles)
    // BeginPostEffects()
    //   Render any other post effects that you like
    // EndPostEffects()

    void BeginNormalRender();
    void EndNormalRender();
    void BeginCustomRender();
    void EndCustomRender(); // was render_deferred...moved so render_deferred fits in i-cache
    void ResetAfterException();
    void AddRigidInstanceSimple(hgeom_inst     hInst,
                                const void*    pCol,
                                const Matrix4* pL2W, // will be DMA ref'd to!
                                const BBox&    WorldBBox,
                                uint32_t       Flags);
    void AddRigidInstance(hgeom_inst     hInst,
                          const void*    pCol,
                          const Matrix4* pL2W,
                          uint64_t       Mask,
                          uint32_t       Flags,
                          int            Alpha);
    void AddRigidInstance(hgeom_inst     hInst,
                          const void*    pCol,
                          const Matrix4* pL2W,
                          uint64_t       Mask,
                          uint32_t       VTextureMask,
                          uint32_t       Flags,
                          int            Alpha);
    void AddSkinInstance(hgeom_inst     hInst,
                         const Matrix4* pBone,
                         uint64_t       Mask,
                         uint32_t       VTextureMask,
                         uint32_t       Flags,
                         const Colour&  Ambient);
    void AddSkinInstanceDistorted(hgeom_inst     hInst,
                                  const Matrix4* pBone,
                                  uint64_t       Mask,
                                  uint32_t       Flags,
                                  const Radian3& NormalRot,
                                  Colour         Ambient);

    // material access
    material& GetMaterial(hgeom_inst hInst,
                          int        iSubMesh);
    texture*  GetVTexture(const Geom* pGeom,
                          int         iMaterial,
                          int         VTextureMask);

    // env. map specification routines
    void SetAreaCubeMap(const cubemap::handle& CubeMap);

    // post-effect rendering routines--the begin and end pair are required and
    // allow the platforms to do some optimization by sharing major screen
    // buffer work between the post-effects.
    enum post_screen_blend
    {
        SOURCE_MINUS_DEST = 0,
    };

    enum post_falloff_fn
    {
        FALLOFF_CONSTANT = 0,
        FALLOFF_LINEAR,
        FALLOFF_EXP,
        FALLOFF_EXP2,
        FALLOFF_CUSTOM,

        ///////////////////////////////////////////////////////////////////////
        // For all types of fogging, the final color is:
        // C = f*OrigColor + (1-f)*FogColor
        //
        // CONSTANT:
        //  f=(z>start)?1.0f-color.a:0
        //  Param1 = start
        //
        // LINEAR:
        //  f=(end-z)/(end-start)
        //  Param1 = start
        //  Param2 = end
        //
        // EXP
        //  (d=(z-n)/(f-n), f=1/e^(d*density)
        //  Param1 = density
        //  Param2 = unused
        //
        // EXP2
        //  (d=(z-n)/(f-n), f=1/e^((d*density)*(d*density))
        //  Param1 = density
        //  Param2 = unused
        //
        // CUSTOM
        //  A bitmap palette is used to describe the fog pattern.
        ///////////////////////////////////////////////////////////////////////
    };

    // For the post-effects, colors are considered to be 128==1, and 255==2 so that we
    // can get oversaturate. The post effects are listed here in the order they will
    // occur in. This is done to maximize performance, since some operations can be
    // shared between post effects (such as filtering a screen down). If you need
    // to do the post-effects in a specific order, you'll need to put them in
    // their own Begin/End blocks, but be warned that performance won't be as good.

    // If you plan on using the zfog filter, but want to manually fog draw items
    // that do not write to the z-buffer or happen outside of the post-effects, use
    // these functions.
    void   SetCustomFogPalette(const texture::handle& Texture,
                               bool                   ImmediateSwitch,
                               int                    PaletteIndex);
    Colour GetFogValue(const Vector3& WorldPos,
                       int            PaletteIndex);

    // Use these functions for doing the actual post-effect
    void BeginMidPostEffects(); // see comments above render::BeginNormalRender
    void EndMidPostEffects();   // see comments above render::BeginNormalRender
    void BeginPostEffects();
    void EndPostEffects();
    void AddScreenWarp(const Vector3& WorldPos,
                       float          Radius,
                       float          WarpAmount);
    void MotionBlur(float Intensity);
    void ApplySelfIllumGlows(float MotionBlurIntensity = 0.0f,
                             int   GlowCutoff = 255);
    void MultScreen(Colour            MultColor,
                    post_screen_blend FinalBlend);
    void RadialBlur(float  Zoom,
                    Radian Angle,
                    float  AlphaSub,
                    float  AlphaScale);
    void ZFogFilter(post_falloff_fn Fn,
                    Colour          Color,
                    float           Param1,
                    float           Param2);
    void ZFogFilter(post_falloff_fn Fn,
                    int             PaletteIndex);
    void MipFilter(int             nFilters,
                   float           Offset,
                   post_falloff_fn Fn,
                   Colour          Color,
                   float           Param1,
                   float           Param2,
                   int             PaletteIndex);
    void MipFilter(int                    nFilters,
                   float                  Offset,
                   post_falloff_fn        Fn,
                   const texture::handle& Texture,
                   int                    PaletteIndex);
    void NoiseFilter(Colour Color);
    void ScreenFade(Colour Color);

    // Filter lighting functions
    void   EnableFilterLight(bool bEnable);
    bool   IsFilterLightEnabled();
    void   SetFilterLightColor(Colour Color);
    Colour GetFilterLightColor();
}

struct color_info
{
    //color_info(fileio& File);

    color_info()
    {
        Init();
    }

    //  -----------------------------------------------------------------------

    enum usage
    {
        kUse32,
        kUse16,
        kUnknown
    };

    static usage Usage()
    {
        return m_Usage;
    }

    //  -----------------------------------------------------------------------

    void SetCount(uint32_t Count)
    {
        m_nColors = Count;
    }

    uint32_t GetCount() const
    {
        return m_nColors;
    }

    //  -----------------------------------------------------------------------

    void Set(uint32_t* pColor32)
    {
        m_pColor32 = pColor32;
        m_Usage = kUse32;
    }

    void Set(uint16_t* pColor16)
    {
        m_pColor16 = pColor16;
        m_Usage = kUse16;
    }

    void Set(void* pVoid)
    {
        m_Usage = kUnknown;
        m_pVoid = pVoid;
    }

    //  -----------------------------------------------------------------------

    operator void*()
    {
        return m_pVoid;
    }

    operator uint32_t*()
    {
        return m_pColor32;
    }

    operator uint16_t*()
    {
        return m_pColor16;
    }

    //  -----------------------------------------------------------------------

    //void FileIO(fileio&);
    void Init();

    //  -----------------------------------------------------------------------

private:
    int          m_nColors;
    static usage m_Usage;
    union
    {
        uint32_t* m_pColor32; // 32-bit color
        uint16_t* m_pColor16; // 16-bit color
        void*     m_pVoid;
    };
};

inline void color_info::Init()
{
    m_Usage = kUnknown;
    m_nColors = 0;
    m_pVoid = 0;
}
