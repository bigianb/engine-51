#pragma once

#include "../Trigger_Actions.h"

struct lock_view_node
{
    lock_view_node()
    {
        m_Linger = 0.0f;
        m_TimeTo = -1.0f;
        m_LookAt.Zero();
    }

    float   m_TimeTo;
    float   m_Linger;
    Vector3 m_LookAt;
};

class lock_player_view : public actions_base
{
public:
    lock_player_view(guid ParentGuid);

    virtual const char* GetTypeName(void) { return "Lock Player View"; }
    virtual const char* GetTypeInfo(void) { return "Takes control of the players viewpoint."; }
    virtual void        Execute(trigger_object* pParent);
    virtual void        OnEnumProp(prop_enum& rList);
    virtual bool        OnProperty(prop_query& rPropQuery);

    virtual void OnRender(void);

    virtual action_types GetType(void) { return GetTypeStatic(); }
    static action_types  GetTypeStatic(void) { return TYPE_ACTION_LOCK_PLAYER_VIEW; }

public:
    enum
    {
        MAX_TABLE_SIZE = 5
    };

protected:
    lock_view_node m_LockViewTable[MAX_TABLE_SIZE];
};
