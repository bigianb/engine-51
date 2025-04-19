#pragma once

#include "../objectManager/ObjectManager.h"
#include "../Bitmap.h"
#include "render/RigidInst.h"

enum select_type
{
    SELECT_NONE,
    SELECT_BRACKETS,
    SELECT_SQUARE,
    SELECT_TRIANGLE,
    SELECT_CIRCLE
};

#define NONLORE_OBJECT_OFFSET 1000

class player;

class lore_object : public Object
{
public:
    CREATE_RTTI(lore_object, Object, Object)

    lore_object();
    ~lore_object();
    int  GetMaterial() const override { return MAT_TYPE_NULL; }
    void OnRender() override;
    void OnRenderTransparent() override;

    // collision/polycache stuff
    bool                   GetColDetails(int Key, detail_tri& Tri) override;
    virtual const Matrix4* GetBoneL2Ws();
    rigid_inst&            GetRigidInst() { return (m_RigidInst); }
    void                   OnPolyCacheGather() override;
    void                   OnTransform(const Matrix4& L2W) override;

    render_inst* GetRenderInstPtr() override { return &m_RigidInst; }

    BBox GetLocalBBox() const override;
        BBox GetFocusBBox() const;

    void OnEnumProp(prop_enum& list) override;
    bool OnProperty(prop_query& rPropQuery) override;

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

    void         OnActivate(bool Flag) override;
    virtual void OnAcquire(bool bIsRestoring = false);
    void         OnAdvanceLogic(float DeltaTime) override;
    virtual bool IsTrueLoreObject() { return m_LoreID < NONLORE_OBJECT_OFFSET; }

    bool HasFocus();
    bool IsActivated() { return m_bActive; }
    bool TestPress();
    int  GetLoreID() { return m_LoreID; }
    bool IsStandingIn() { return m_bStandingIn; }

    void DoCollisionCheck(player* pPlayer, Vector3& StartPos, Vector3& EndPos);

    // collision stuff
    void OnColCheck() override;

    guid m_TriggerGuid;
    guid m_SpatialGuid;

    BBox m_FocusBox;

    float m_ViewDist;

    int m_DisplayStrTable;
    int m_DisplayStrTitle;

    float m_TextDist;
    float m_ScanDist;
    bool  m_bScanLOS;

    char*       m_pScanAudio;
    select_type m_ViewType;
    bool        m_bAutoOff;

protected:
    bool m_bActive;
    bool m_bDestroyAfterAcquired; // do we kick off the trigger (if any) again on restore?
    bool m_bActivateTriggerOnRestore;

    // Internal Stuff
    float m_AnimState;
    float m_TextAlphaState;
    bool  m_bLookingAt;
    bool  m_bRendered;

    bool m_bHasLOS;
    bool m_bInViewRange;
    bool m_bInTextRange;
    bool m_bInScanRange;
    bool m_bStandingIn;

    uint8_t m_MaxAlpha;
    float   m_ColorPhase;
    bool    m_bUseGeometrySize; // use geometry size or use size variable?
    int     m_LoreID;

    static ResourceHandle<Bitmap> m_Bracket;

    Matrix4    m_RenderL2W;
    rigid_inst m_RigidInst; // Instance for rendering object.
    uint32_t   m_VMeshMask;

};
