#pragma once

#include "../Object.h"
#include "../../objectManager/ObjectManager.h"

class event_sound_emitter : public Object
{
public:
    CREATE_RTTI(event_sound_emitter, Object, Object)

    enum sound_type
    {
        FIRST_TYPE,

        SINGLE_SHOT = FIRST_TYPE,
        LOOPED,
        CONVERSATION,
        CONTACT,

        LAST_TYPE = CONTACT
    };

    event_sound_emitter(ObjectManager* om);
    int  GetMaterial() const override { return MAT_TYPE_NULL; }
    void OnAdvanceLogic(float DeltaTime) override;
    void OnMove(const Vector3& NewPos) override;
    BBox GetLocalBBox() const override;

    void PlayEmitter(const char* pDescriptor, Vector3& Position, uint16_t ZoneID,
                     sound_type Type, guid ParentGuid, uint32_t Flags = 0,
                     float Delay = 0.0f, bool UseRadius = false,
                     float Radius = 1.0f, bool Play2D = false);

    guid        GetParentGuid() override { return m_ParentGuid; }
    const char* GetDescriptorName() { return m_DescriptorName; }
    const char* GetMaterialType();
    const char* GetMaterialTypeFromActor(guid Guid);
    const char* GetMaterialName(int MatType);
    int         GetVoiceID() { return m_VoiceID; }

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

protected:
    void StartEmitter();
    void StartEmitter2D();

protected:
    char       m_DescriptorName[64];
    char       m_ObjectName[64];
    char       m_ActionName[64];
    int        m_VoiceID;
    guid       m_ParentGuid;
    bool       m_Active;
    sound_type m_SoundType;
    float      m_Delay;
    uint32_t   m_Flags;
    bool       m_SphereTest;
    float      m_Radius;
    bool       m_b2D;
};
