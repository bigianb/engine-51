#pragma once

#include "../VectorMath.h"
#include "../Colour.h"
#include "../Bitmap.h"
#include "../xfiles/xHandle.h"

#define MAX_CTRL_TYPES 32
#define MAX_ELEMENT_TYPES 32

#define MAX_EFFECT_DEFINITIONS 256
#define MAX_EFFECT_INSTANCES 512

class fx_mgr;
struct fx_def;
class fx_effect_base;
struct fx_ctrl_def;
struct fx_element_def;

class fx_ctrl
{
protected:
    const fx_ctrl_def* m_pCtrlDef;
    float              m_AgeRate;  // Rate DeltaTime advances a cycle.
    int                m_Cycle;    // Which cycle are we on?
    float              m_CycleAge; // Where in current cycle (0.0 - 1.0)?
    float*             m_pOutput;

public:
    void  Initialize(fx_ctrl_def* pCtrlDef,
                     float*       pOutput);
    void  AdvanceLogic(float DeltaTime);
    float ComputeLogicalTime() const;

    virtual void Evaluate(float LogicalTime) const = 0;
};

class fx_element
{
    friend fx_effect_base;
    //------------------------------------------------------------------------------
protected:
    mutable Matrix4 m_Local;      // Local matrix from SRT of element.
    mutable Matrix4 m_L2W;        // L2W of element.
    mutable bool    m_LocalDirty; // Need to recompute L2W because local part is dirty?
    mutable bool    m_BaseDirty;  // Need to recompute L2W because base part if dirty?

    const fx_element_def* m_pElementDef;
    const float*          m_pInput;

    Vector3p m_Scale;         // From here...
    Radian3  m_Rotate;        //       ...
    Vector3p m_Translate;     //    ...
    float    m_FloatColor[4]; // ... to here MUST BE ADJECENT.

    Colour m_BaseColor;  // Color of parent effect.
    Colour m_Color;      // Color of element.
    bool   m_ColorDirty; // Need to reapply colors?

    BBox m_BBox;

    //------------------------------------------------------------------------------
public:
    void           BaseL2W();
    void           BaseLogic();
    void           BaseColor(const Colour Color);
    void           BaseColor();
    const Matrix4& GetL2W(const fx_effect_base* pEffect) const;
    float          GetUniformScale() const;
    const BBox&    GetBBox() const;

    //------------------------------------------------------------------------------
public: // THESE FUNCTIONS ARE AVAILABLE TO BE REDEFINED BY DESCENDANT CLASSES
    virtual void Initialize(const fx_element_def* pElementDef,
                            float*                pInput);

    virtual void AdvanceLogic(const fx_effect_base* pEffect,
                              float                 DeltaTime);

    virtual void Render(const fx_effect_base* pEffect) const;

    virtual bool IsFinished(const fx_effect_base* pEffect) const;

    virtual void Reset();

    virtual void ApplyColor();
};

//==============================================================================
//  ELEMENT REGISTRATION MACROS
//==============================================================================
//
//  The following macros is used to register elements.
//
//  You only need to bother with this macro if you are going to do a new or
//  customized element.
//
//  If you create a new fx_element descendant class named "my_element", the
//  you can place the following:
//
//      REGISTER_FX_ELEMENT_CLASS( my_element, "MyElement", MyElementMemoryFn );
//
//  withing the "my_element.cpp" file (outside of any function).  Note that
//  the string "MyElement" would be the value given in the "Custom Type" field
//  in the fx_Editor.  Also note that MyElementMemoryFn must be a function which
//  satisfies the "fx_element_memory_fn" function signature.
//
//==============================================================================

typedef int fx_element_memory_fn(const fx_element_def& ElementDef);

//------------------------------------------------------------------------------

fx_element_memory_fn DefaultElementMemoryFn;

//------------------------------------------------------------------------------

#define REGISTER_FX_ELEMENT_CLASS(T, pName, pMemoryFn) //** Details hidden.

//==============================================================================
//  HOOKS FOR LOADING AND UNLOADING BITMAPS
//==============================================================================

typedef bool           fx_load_bitmap_fn(const char* pBitmapName, xhandle&    Handle);
typedef void           fx_unload_bitmap_fn(xhandle Handle);
typedef const Bitmap* fx_resolve_bitmap_fn(xhandle Handle);

//==============================================================================
//  CLASS FX_HANDLE
//==============================================================================

class fx_handle
{

    //------------------------------------------------------------------------------
public:
    fx_handle();
    fx_handle(const fx_handle& Handle);
    ~fx_handle();

    bool InitInstance(const char* pName);
    void KillInstance();

    void AdvanceLogic(float DeltaTime);
    void Restart();

    void Render() const;

    void    SetScale(const Vector3& Scale);
    void    SetRotation(const Radian3& Rotation);
    void    SetTranslation(const Vector3& Translation);
    Vector3 GetTranslation() const;
    void    SetTransform(const Matrix4& L2W);
    void    SetColor(const Colour& Color);
    Colour  GetColor();

    void SetSuspended(bool Suspended);

    const BBox& GetBounds() const;

    bool Validate() const;
    bool IsFinished() const;
    bool IsInstanced() const;

    const fx_handle& operator=(const fx_handle& Handle);

    //------------------------------------------------------------------------------
protected:
    mutable int Index;
    friend fx_mgr;
};

//==============================================================================
//  PRIVATE STUFF
//==============================================================================

#include "fx_Mgr_private.h"

//==============================================================================
//  CLASS FX_MGR
//==============================================================================

class fx_mgr
{

    //------------------------------------------------------------------------------
public:
    fx_mgr();
    ~fx_mgr();

    void SetBitmapFns(fx_load_bitmap_fn*    pLoadFn,
                      fx_unload_bitmap_fn*  pUnloadFn,
                      fx_resolve_bitmap_fn* pResolveFn);

    //bool LoadEffect(const char* pEffectName, X_FILE* pFile);
    bool LoadEffect(const char* pEffectName, const char* pFileName);
    bool UnloadEffect(const char* pEffectName);

    void    SetSpriteBudget(int MaxSprites);
    int     GetSpriteCount();
    Vector3 GetTranslation(const fx_handle& Handle);
    void    EndOfFrame();

    //------------------------------------------------------------------------------

#include "fx_Mgr_insert.h"
};

//==============================================================================
//  STORAGE ANNOUNCEMENTS
//==============================================================================

extern fx_mgr FXMgr;

//==============================================================================
// Inlines
//==============================================================================

inline float fx_element::GetUniformScale() const
{
    return ((m_Scale.x + m_Scale.y + m_Scale.z) / 3.0f);
}

//==============================================================================

inline const BBox& fx_element::GetBBox() const
{
    return (m_BBox);
}

//==============================================================================

inline void fx_element::BaseL2W()
{
    m_BaseDirty = true;
}

//==============================================================================

inline void fx_element::BaseColor(const Colour Color)
{
    m_BaseColor = Color;
    m_ColorDirty = true;
}

//==============================================================================

inline void fx_element::BaseColor()
{
    if (m_ColorDirty) {
        ApplyColor();
        m_ColorDirty = false;
    }
}

//==============================================================================

inline const Matrix4& fx_element::GetL2W(const fx_effect_base* pEffect) const
{
    if (m_LocalDirty) {
        m_Local.Setup(m_Scale, m_Rotate, m_Translate);
    }

    if (m_BaseDirty || m_LocalDirty) {
        m_L2W = pEffect->GetL2W() * m_Local;
    }

    m_LocalDirty = false;
    m_BaseDirty = false;

    return (m_L2W);
}
