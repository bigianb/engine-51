#pragma once

#include "../objectManager/ObjectManager.h"
#include "../render/Texture.h"

class projector_obj : public Object
{
public:
    CREATE_RTTI(projector_obj, Object, Object)

    projector_obj();
    virtual ~projector_obj();
    BBox GetLocalBBox() const override;
    int  GetMaterial() const override { return MAT_TYPE_FLESH; }
    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;

    bool                   IsDynamic() const;
    bool                   IsShadow() const;
    bool                   IsActive() const;
    bool                   IsFlashlight() const;
    Radian                 GetFOV() const;
    float                  GetLength() const;
    const texture::handle& GetTexture() const;

    void SetShadow(bool IsShadow);
    void SetActive(bool IsActive);
    void SetIsFlashlight(bool Flashlight);
    void SetFOV(Radian FOV);
    void SetLength(float Length);
    void SetTextureHandle(texture::handle Texture);

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

protected:
    void OnRender() override;

    bool            m_bIsDynamic;
    bool            m_bIsShadow;
    bool            m_bIsActive;
    bool            m_bIsFlashlight;
    Radian          m_FOV;
    float           m_Length;
    texture::handle m_hTexture; // Handle to the texture
};

inline bool projector_obj::IsDynamic() const
{
    return m_bIsDynamic;
}

//=========================================================================

inline bool projector_obj::IsShadow() const
{
    return m_bIsShadow;
}

//=========================================================================

inline bool projector_obj::IsActive() const
{
    return m_bIsActive;
}

//=========================================================================

inline bool projector_obj::IsFlashlight() const
{
    return m_bIsFlashlight;
}

//=========================================================================

inline Radian projector_obj::GetFOV() const
{
    return m_FOV;
}

//=========================================================================

inline float projector_obj::GetLength() const
{
    return m_Length;
}

//=========================================================================

inline const texture::handle& projector_obj::GetTexture() const
{
    return m_hTexture;
}

//=========================================================================

inline void projector_obj::SetShadow(bool IsShadow)
{
    m_bIsShadow = IsShadow;
}

//=========================================================================

inline void projector_obj::SetActive(bool IsActive)
{
    m_bIsActive = IsActive;
}

//=========================================================================

inline void projector_obj::SetIsFlashlight(bool Flashlight)
{
    m_bIsFlashlight = Flashlight;
}

//=========================================================================

inline void projector_obj::SetFOV(Radian FOV)
{
    m_FOV = FOV;
}

//=========================================================================

inline void projector_obj::SetLength(float Length)
{
    m_Length = Length;
}

//=========================================================================

inline void projector_obj::SetTextureHandle(texture::handle Texture)
{
    m_hTexture = Texture;
}
