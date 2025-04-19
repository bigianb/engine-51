#pragma once

#include "Texture.h"
#include "Material_Prefs.h"

class material
{
public:
    struct uvanim
    {
        float   CurrentFrame; // current frame as float
        int16_t iKey;         // offset into geometry
        int8_t  iFrame;       // current frame as int
        int8_t  Dir;          // direction of animation {-1,0,1}
        int8_t  Type;         // type of animation
        int8_t  nFrames;      // total number of frames in animation
        int8_t  FPS;          // frames per second to run this animation
        int8_t  StartFrame;   // starting frame for this animation
    };

    material();
    ~material();
    bool operator==(const material& RHS) const;

    bool HasUVAnimation() const;
    int  AddRef();
    int  Release();
    int  GetRefCount() const;

    int8_t   m_Type;
    float    m_DetailScale;
    float    m_FixedAlpha;
    uint16_t m_Flags; // flags

    texture::handle m_DiffuseMap;
    texture::handle m_EnvironmentMap;
    texture::handle m_DetailMap;
    uvanim          m_UVAnim;
    int             m_RefCount;
};

inline bool material::HasUVAnimation() const
{
    return (m_UVAnim.nFrames > 0);
}

inline int material::AddRef()
{
    return (++m_RefCount);
}

inline int material::Release()
{
    return (--m_RefCount);
}

inline int material::GetRefCount() const
{
    return m_RefCount;
}
