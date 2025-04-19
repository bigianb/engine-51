#pragma once

#include "../../Property.h"
#include "../../resourceManager/ResourceManager.h"
#include "../../render/Render.h"
#include "VirtualMeshMask.h"

class render_inst : public prop_interface
{
public:
    render_inst();
    ~render_inst();

    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);

    void                StartFade(int8_t Direction, float TimeToFade);
    void                OnAdvanceLogic(float DeltaTime);
    uint8_t             GetAlpha() const { return m_Alpha; }
    virtual void        LoadColorTable(const char* pFileName) {}
    virtual Geom*       GetGeom() const = 0;
    virtual std::string GetGeomName() const { return ""; }

    render::hgeom_inst       GetInst() const;
    BBox&                    GetBBox() const;
    uint64_t                 GetLODMask(const Matrix4& L2W);
    uint64_t                 GetLODMask(uint16_t ScreenSize);
    void                     SetVMeshBit(int  Index,
                                         bool OnOff);
    void                     SetVMeshBit(const char* pName,
                                         bool        OnOff);
    const virtual_mesh_mask& GetVMeshMask(void) const;
    void                     SetVMeshMask(uint32_t Mask);
    int                      GetNActiveBones(const uint64_t& LODMask) const;

protected:
    render::hgeom_inst m_hInst; // Handle to the instance in the Render Manager

    virtual_mesh_mask m_VMeshMask;       // which vmeshes are turned on?
    float             m_FadeTimeElapsed; // how much time since we've started fading?
    float             m_FadeTime;        // how long to fade in/out?
    int8_t            m_FadeDirection;   // -1 fade-out, 0 no fading, 1 fade-in
    uint8_t           m_Alpha;           // alpha value based on fading data
};

//=========================================================================

inline render::hgeom_inst render_inst::GetInst(void) const
{
    return m_hInst;
}

//=========================================================================

inline uint64_t render_inst::GetLODMask(uint16_t ScreenSize)
{
    Geom* pGeom = GetGeom();
    if (pGeom) {
        return pGeom->GetLODMask(m_VMeshMask, ScreenSize);
    } else {
        return (uint64_t)-1;
    }
}

//=========================================================================

inline void render_inst::SetVMeshBit(int Index, bool OnOff)
{
    if (OnOff) {
        m_VMeshMask.VMeshMask |= (1 << Index);
    } else {
        m_VMeshMask.VMeshMask &= ~(1 << Index);
    }
}

//=========================================================================

inline void render_inst::SetVMeshBit(const char* pName, bool OnOff)
{
    Geom* pGeom = GetGeom();
    if (pGeom) {
        int Index = pGeom->GetVMeshIndex(pName);
        if (Index != -1) {
            if (OnOff) {
                m_VMeshMask.VMeshMask |= (1 << Index);
            } else {
                m_VMeshMask.VMeshMask &= ~(1 << Index);
            }
        }
    }
}

//=========================================================================

inline const virtual_mesh_mask& render_inst::GetVMeshMask(void) const
{
    return m_VMeshMask;
}

//=========================================================================

inline void render_inst::SetVMeshMask(uint32_t Mask)
{
    m_VMeshMask.VMeshMask = Mask;
}
