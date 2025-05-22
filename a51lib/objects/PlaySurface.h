
#pragma once

#include "../objectManager/ObjectManager.h"
#include "../resourceManager/ResourceManager.h"
#include "render/RigidInst.h"

class play_surface : public Object
{
public:
    CREATE_RTTI(play_surface, Object, Object)

    play_surface(ObjectManager* om, ResourceManager* rm);
    ~play_surface();

    BBox GetLocalBBox() const override;
    int  GetMaterial() const override { return MAT_TYPE_NULL; }

    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;
    void OnTransform(const Matrix4& L2W) override;

    bool           GetColDetails(int Key, detail_tri& Tri) override;
    virtual const Matrix4* GetBoneL2Ws();
    rigid_inst&            GetRigidInst() { return (m_Inst); }

    // virtual render_inst*        GetRenderInstPtr() { return &m_Inst; }

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

    void OnPolyCacheGather() override;

protected:
    virtual void OnImport(text_in& TIn);
    void         OnRender() override;
    void         OnColCheck() override;
    void         OnKill() override;

    void DoColCheck(const Matrix4* pBones);

protected:
    rigid_inst m_Inst; // Render Instance for the Play Surface
};
