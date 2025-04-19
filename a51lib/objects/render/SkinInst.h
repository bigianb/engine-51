#pragma once

#include "../../render/SkinGeom.h"
#include "RenderInst.h"
#include "VirtualTextureMask.h"

class skin_inst : public render_inst
{
public:
    skin_inst();
    ~skin_inst();

    void        OnEnumProp(prop_enum& List) override;
    bool        OnProperty(prop_query& I) override;
    Geom*       GetGeom() const override;
    std::string GetGeomName() const override;

    SkinGeom*  GetSkinGeom() const;
    std::string GetSkinGeomName() const;

    virtual void SetUpSkinGeom(std::string fileName);

    void Render(const Matrix4* pL2W,
                const Matrix4* pBone,
                int            nBone,
                uint32_t       Flags,
                uint64_t       LODMask,
                const Colour&  Ambient = Colour(64, 64, 64, 255));

    void RenderDistortion(const Matrix4* pL2W,
                          const Matrix4* pBone,
                          int            nBone,
                          uint32_t       Flags,
                          uint64_t       LODMask,
                          const Radian3& NormalRot,
                          const Colour&  Ambient = Colour(0, 0, 0, 255));

    void RenderShadowCast(const Matrix4* pL2W,
                          const Matrix4* pBone,
                          int            nBone,
                          uint32_t       Flags,
                          uint64_t       LODMask,
                          uint64_t       ProjMask);

    void SetMinAmbient(Colour MinAmbient);
    void SetOtherAmbientAmount(float OtherAmbientAmount);

    const skin_inst& operator=(const skin_inst& Skin);

    void SetVirtualTexture(const char* pVTextureName,
                           const char* pDiffuseTextureDesc);
    void SetVirtualTexture(int VTexture);

protected:
    ResourceHandle<SkinGeom> m_hSkinGeom;          // Handle to the Skin Geom
    virtual_texture_mask      m_VTextureMask;       // virtual texture mask for bitmap swapping
    Colour                    m_MinAmbient;         // ambient color this instance will always receive
    float                     m_OtherAmbientAmount; // amount that other ambient sources (such as the floor color or lights) will contribute
};

inline SkinGeom* skin_inst::GetSkinGeom() const
{
    return (m_hSkinGeom.getPointer());
}

inline Geom* skin_inst::GetGeom() const
{
    return GetSkinGeom();
}

inline std::string skin_inst::GetGeomName() const
{
    return GetSkinGeomName();
}

inline void skin_inst::SetMinAmbient(Colour MinAmbient)
{
    m_MinAmbient = MinAmbient;
}

inline void skin_inst::SetOtherAmbientAmount(float OtherAmbientAmount)
{
    m_OtherAmbientAmount = OtherAmbientAmount;
}
