#pragma once

#include "../../render/RigidGeom.h"
#include "RenderInst.h"

class rigid_inst : public render_inst
{
public:
    rigid_inst(ResourceManager*);
    ~rigid_inst();

    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);

    bool SetUpRigidGeom(const char* pFileName);

    virtual Geom*       GetGeom() const;
    virtual std::string GetGeomName() const;

    RigidGeom*  GetRigidGeom() const;
    int         GetNumColors() const;
    const void* GetColorTable() const;
    //    const void*         GetColorTable       ( platform ) const;
    std::string GetRigidGeomName() const;

    void         SetColorTable(const void* pColorTable, int iColor, int nColors);
    virtual void LoadColorTable(const char* pFileName, ResourceManager* );

    void Render(const Matrix4* pL2W, uint32_t Flags);
    void Render(const Matrix4* pL2W, uint32_t Flags, uint64_t Mask);
    void Render(const Matrix4* pL2W, uint32_t Flags, uint64_t Mask, uint8_t Alpha);
    void Render(const Matrix4* pL2W, uint32_t Flags, uint32_t VTextureMask, int Alpha = 255);

protected:
    ResourceHandle<RigidGeom> m_hRigidGeom;  // Handle to the Rigid Geom
    const void*               m_pRigidColor; // Ptr to colors

    int m_nColors; // Number of colours used by instance
    int m_iColor;  // Index into colour table
};

//=========================================================================

inline RigidGeom* rigid_inst::GetRigidGeom() const
{
    return (m_hRigidGeom.getPointer());
}

//=============================================================================

inline Geom* rigid_inst::GetGeom() const
{
    return GetRigidGeom();
}

//=========================================================================

inline std::string rigid_inst::GetGeomName() const
{
    return GetRigidGeomName();
}
