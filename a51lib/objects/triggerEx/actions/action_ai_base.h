#pragma once

#include "../TriggerExActions.h"
#include "../affecters/ObjectAffecter.h"
#include "../../../characters/ResponseList.h"

class character_trigger_state;

class action_ai_base : public actions_ex_base
{
public:
    // NOTE: Always add new enums to the end of the list or break existing game data!!!
    enum ai_action_types
    {
        AI_ACTION_NULL,
        AI_PATHFIND_TO_GUID,
        AI_ATTACK_GUID,
        AI_LOOK_AT_GUID,
        AI_DIALOG_LINE,
        AI_PLAY_ANIMATION,
        AI_SEARCHTO_GUID,
        AI_INVENTORY,
        AI_DEATH,
    };

    CREATE_RTTI(action_ai_base, actions_ex_base, actions_ex_base)

    action_ai_base(guid ParentGuid, ObjectManager* om);

    virtual bool Execute(float DeltaTime);

    virtual void OnEnumProp(prop_enum& rList);
    virtual bool OnProperty(prop_query& rPropQuery);
    virtual void OnActivate(bool Flag);

    virtual ai_action_types GetAIActionType() = 0;
    inline void             ReleaseBlocking() { m_bIsBlockingAction = false; }

    void     SetResponseFlags(uint32_t newFlags) { m_ResponseFlags.SetFlags(newFlags); }
    uint32_t GetResponseFlags() { return m_ResponseFlags.GetFlags(); }
    void     SetCharacterAffecter(object_affecter newAffecter) { m_CharacterAffecter = newAffecter; }
    bool     GetBlockUntilComplete() { return m_bBlockUntilComplete; }
    bool     GetIsRunningAction() { return m_bIsRunningAction; }
    bool     GetIsBlockingAction() { return m_bIsBlockingAction; }
    bool     GetMustSucceed() { return m_bMustSucceed; }

protected:
    const char* GetAIName();

    object_affecter m_CharacterAffecter;
    bool            m_bBlockUntilComplete;
    bool            m_bIsRunningAction;
    bool            m_bIsBlockingAction;
    bool            m_bMustSucceed;

    response_list m_ResponseFlags;

    friend character_trigger_state;
};
