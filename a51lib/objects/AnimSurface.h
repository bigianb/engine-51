
#pragma once

#include "PlaySurface.h"
#include "../animation/AnimPlayer.h"
#include "../zoneManager/ZoneManager.h"

class anim_surface : public play_surface
{
public:
    CREATE_RTTI(anim_surface, play_surface, Object)

    anim_surface();
    ~anim_surface();

    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;

    void                      OnMove(const Vector3& NewPos) override;
    void                      OnTransform(const Matrix4& L2W) override;
    void                      OnAdvanceLogic(float DeltaTime) override;
    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();
    BBox                      GetLocalBBox() const override;
    virtual void              GetBoneL2W(int iBone, Matrix4& L2W);
    void                      OnEvent(const event& Event) override;

    void OnPolyCacheGather() override;

    void         EnumAttachPoints(std::string& String) const override;
    int          GetAttachPointIDByName(const char* pName) const override;
    std::string  GetAttachPointNameByID(int iAttachPt) const override;
    bool         GetAttachPointData(int      iAttachPt,
                                    Matrix4& L2W,
                                    uint32_t Flags = 0) override;
    virtual void OnAttachedMove(int      iAttachPt,
                                Matrix4& L2W);

    simple_anim_player* GetSimpleAnimPlayer() override { return &m_AnimPlayer; }
    AnimGroup::handle*  GetAnimGroupHandlePtr() override { return &m_hAnimGroup; }

protected:
    void OnRender() override;
    void OnColCheck() override;

    const Matrix4* GetBoneL2Ws() override;
    void           UpdateZoneTrack();

protected:
    AnimGroup::handle    m_hAnimGroup;
    ResourceHandle<char> m_hAudioPackage;
    simple_anim_player   m_AnimPlayer;
    int16_t              m_iBackupAnimString;
    zone_mgr::tracker    m_ZoneTracker;
};
