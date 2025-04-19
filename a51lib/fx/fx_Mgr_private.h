#pragma once

#include <cassert>

typedef void fx_ctrl_ctor_fn(void* pAddress);

struct fx_ctrl_reg
{
public:
    fx_ctrl_reg(const char*      pName,
                fx_ctrl_ctor_fn* pFactorFn);
};

#define REGISTER_FX_CONTROLLER_CLASS(T, pName)   \
                                                 \
    inline void fx_Construct_##T(void* pAddress) \
    {                                            \
        new (pAddress) T;                        \
    }                                            \
                                                 \
    fx_ctrl_reg CtrlReg_##T(pName, fx_Construct_##T);

typedef void fx_element_ctor_fn(void* pAddress);

struct fx_element_reg
{
public:
    fx_element_reg(const char*           pName,
                   fx_element_ctor_fn*   pFactoryFn,
                   fx_element_memory_fn* pMemoryFn);
};

//==============================================================================

#undef REGISTER_FX_ELEMENT_CLASS

#define REGISTER_FX_ELEMENT_CLASS(T, pName, pMemoryFn) \
                                                       \
    inline void fx_Construct_##T(void* pAddress)       \
    {                                                  \
        new (pAddress) T;                              \
    }                                                  \
                                                       \
    fx_element_reg ElementReg_##T(pName, fx_Construct_##T, pMemoryFn);

//==============================================================================
//  TYPES
//==============================================================================

class fx_mgr;
class fx_handle;
class fx_effect_clone;

//==============================================================================

enum fx_loop
{
    FX_CLAMP,
    FX_TILE,
    FX_MIRROR
};

//==============================================================================

enum fx_flags
{
    FX_SINGLETON = (1 << 0),

    FX_DEFERRED_DELETE = (1 << 1),

    FX_MASTER_LOGIC = (1 << 30),
    FX_MASTER_COPY = (1 << 31),
};

//==============================================================================

struct fx_ctrl_type
{
    char             Name[32];
    fx_ctrl_ctor_fn* pFactoryFn;
};

//==============================================================================

struct fx_element_type
{
    char                  Name[32];
    fx_element_ctor_fn*   pFactoryFn;
    fx_element_memory_fn* pMemoryFn; // How much memory does element need?
};

//==============================================================================

struct fx_ctrl_def
{
    int     TotalSize;     // In 32-bit values.
    int     TypeIndex;     //
    int     NOutputValues; // How many output channels of data?
    fx_loop LeadIn;        // How to evaluate before DataBegin?
    fx_loop LeadOut;       // How to evaluate after  DataEnd?
    float   DataBegin;     // Within effect.
    float   DataEnd;       // Within effect.
    int     OutputIndex;   // Where in StagingArea to write output.
};

//==============================================================================

struct fx_element_def
{
    int      TotalSize;       // In 32-bit values.
    int      TypeIndex;       //
    bool     ReadZ;           //
    int      CombineMode;     // -1:Subtract  0:Multiple  +1:Add
    int      CtrlOffsets[13]; // For SRT/C values from Staging Area.
    Vector3p Scale;           // Constant values for S.
    Radian3  Rotate;          // Constant values for R.
    Vector3p Translate;       // Constant values for T.
    float    Color[4];        // Constant values for C (as floats).
    float    TimeStart;       // Within effect.
    float    TimeStop;        // Within effect.
    // Replications?  And ranges for constant values?
};

//==============================================================================

struct fx_def
{
    int              TotalSize; // In 32-bit values.  (Excludes names.)
    mutable uint32_t      Flags;     // See fx_flags.
    int              NSAValues; // Staging area values.
    int              NControllers;
    int              NElements;
    int              NBitmaps;
    int              MasterCopy;
    mutable int      NInstances; // Number of effects of this def.
    char*            pEffectName;
    fx_ctrl_def**    pCtrlDef;
    fx_element_def** pElementDef;
    xhandle*         pDiffuseMap;
    xhandle*         pAlphaMap;
};

//==============================================================================
//  class fx_effect_base
//==============================================================================

class fx_effect_base
{
    //------------------------------------------------------------------------------

public:
    void Initialize(const fx_def* pEffectDef);

    const Matrix4& GetL2W() const;
    const Vector3& GetScale() const;
    const Radian3& GetRotation() const;
    const Vector3& GetTranslation() const;
    Colour         GetColor() const;
    float          GetUniformScale() const;

    void SetScale(const Vector3& Scale);
    void SetRotation(const Radian3& Rotation);
    void SetTranslation(const Vector3& Translation);
    void SetColor(const Colour Color);

    void         AddReference();
    void         RemoveReference();
    int          GetReferences();
    fx_element** GetElementList();

    void Render();

    void GetBitmaps(int            Index,
                    const Bitmap*& pDiffuseMap,
                    const Bitmap*& pAlphaMap) const;

    //------------------------------------------------------------------------------

    virtual float       GetAge() const = 0;
    virtual const BBox& GetBounds() const = 0;
    virtual bool        IsSuspended() const = 0;
    virtual bool        IsFinished() const = 0;
    virtual bool        IsInstanced() const = 0;
    virtual void        Restart() = 0;
    virtual void        SetSuspended(bool Suspended) = 0;
    virtual void        AdvanceLogic(float DeltaTime) = 0;

    //------------------------------------------------------------------------------

protected:
    Vector3 m_Scale;
    Radian3 m_Rotation;
    Vector3 m_Translation;

    Colour m_Color;
    bool   m_ColorDirty;

    mutable Matrix4 m_L2W;
    mutable bool    m_L2WDirty;
    mutable bool    m_EL2WDirty;

    mutable BBox m_BBox;
    mutable bool m_BBoxDirty;

    uint32_t      m_Flags;
    int           m_NReferences;
    fx_element**  m_pElement;
    const fx_def* m_pEffectDef;

    friend fx_mgr;
    friend fx_effect_clone;
};

//==============================================================================
//  class fx_effect
//==============================================================================

class fx_effect : public fx_effect_base
{
    //------------------------------------------------------------------------------

public:
    void        Initialize(const fx_def* pEffectDef);
    static void ForceConstruct(void* pAddress);

    //------------------------------------------------------------------------------

    virtual float       GetAge() const;
    virtual const BBox& GetBounds() const;
    virtual bool        IsSuspended() const;
    virtual bool        IsFinished() const;
    virtual bool        IsInstanced() const;
    virtual void        Restart();
    virtual void        SetSuspended(bool Suspended);
    virtual void        AdvanceLogic(float DeltaTime);

    //------------------------------------------------------------------------------

protected:
    float m_Age;
    bool  m_Done;
    bool  m_Suspended;

    float*    m_pStagingArea;
    fx_ctrl** m_pCtrl;
};

//==============================================================================
//  class fx_effect_clone
//==============================================================================

class fx_effect_clone : public fx_effect_base
{
public:
    void        Initialize(fx_effect_base* pMasterEffect,
                           const fx_def*   pEffectDef);
    static void ForceConstruct(void* pAddress);

    //------------------------------------------------------------------------------

    virtual float       GetAge() const;
    virtual const BBox& GetBounds() const;
    virtual bool        IsSuspended() const;
    virtual bool        IsFinished() const;
    virtual bool        IsInstanced() const;
    virtual void        Restart();
    virtual void        SetSuspended(bool Suspended);
    virtual void        AdvanceLogic(float DeltaTime);

    //------------------------------------------------------------------------------

protected:
    fx_effect_base* m_pEffect;
};

//==============================================================================
//  Inlines for class fx_effect_base
//==============================================================================

inline const Matrix4& fx_effect_base::GetL2W() const
{
    if (m_L2WDirty) {
        m_L2W.Setup(m_Scale, m_Rotation, m_Translation);
        m_L2WDirty = false;
    }
    return (m_L2W);
}

//==============================================================================

inline float fx_effect_base::GetUniformScale() const
{
    return ((m_Scale.GetX() + m_Scale.GetY() + m_Scale.GetZ()) / 3.0f);
}

//==============================================================================

inline const Vector3& fx_effect_base::GetScale() const
{
    return (m_Scale);
}

//==============================================================================

inline const Radian3& fx_effect_base::GetRotation() const
{
    return (m_Rotation);
}

//==============================================================================

inline const Vector3& fx_effect_base::GetTranslation() const
{
    return (m_Translation);
}

//==============================================================================

inline Colour fx_effect_base::GetColor() const
{
    return (m_Color);
}

//==============================================================================

inline void fx_effect_base::SetScale(const Vector3& Scale)
{
    assert(Scale.IsValid());
    m_Scale = Scale;
    m_L2WDirty = true;
    m_EL2WDirty = true;
}

//==============================================================================

inline void fx_effect_base::SetRotation(const Radian3& Rotation)
{
    //assert(Rotation.IsValid());
    m_Rotation = Rotation;
    m_L2WDirty = true;
    m_EL2WDirty = true;
}

//==============================================================================

inline void fx_effect_base::SetTranslation(const Vector3& Translation)
{
    assert(Translation.IsValid());
    m_Translation = Translation;
    m_L2WDirty = true;
    m_EL2WDirty = true;
}

//==============================================================================

inline void fx_effect_base::SetColor(const Colour Color)
{
    m_Color = Color;
    m_ColorDirty = true;
}

//==============================================================================

inline void fx_effect_base::AddReference()
{
    m_NReferences++;
}

//==============================================================================

inline void fx_effect_base::RemoveReference()
{
    m_NReferences--;
}

//==============================================================================

inline int fx_effect_base::GetReferences()
{
    return (m_NReferences);
}

//==============================================================================

inline fx_element** fx_effect_base::GetElementList()
{
    return (m_pElement);
}
