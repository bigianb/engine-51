#pragma once

#include "Object.h"
#include "../objectManager/ObjectManager.h"

class particle_event_emitter : public Object
{
public:
    CREATE_RTTI(particle_event_emitter, Object, Object)

    particle_event_emitter(ObjectManager* om);
    int  GetMaterial() const override { return MAT_TYPE_NULL; }
    void OnAdvanceLogic(float DeltaTime) override;
    void OnMove(const Vector3& NewPos) override;
    void OnTransform(const Matrix4& L2W) override;
    BBox GetLocalBBox() const override;

    void StartEmitter(const char*    pFx,
                      const Vector3& Position,
                      const Vector3& Rotation,
                      uint16_t       ZoneID,
                      guid           ParentGuid,
                      int            EventID,
                      bool           EventActive);

    guid        GetParentGuid() override { return m_ParentGuid; }
    guid        GetParticleGuid() { return m_ParticleGuid; }
    const char* GetFxName() { return m_FxName; }
    int         GetEventID() { return m_EventID; }
    void        EnableUpdate() { m_LogicRunning = true; }

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

protected:
    char m_FxName[64];
    guid m_ParentGuid;
    guid m_ParticleGuid;
    int  m_EventID;
    bool m_EventActive;
    bool m_LogicRunning;
};
