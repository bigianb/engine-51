#pragma once

#include "../objectManager/ObjectManager.h"
//#include "..\Support\Globals\Global_Variables_Manager.hpp"
//#include "..\MiscUtils\SimpleUtils.hpp"
#include "../PropertyEnum.h"

class trigger_object;

extern const float k_rand_draw_displace_amt;

struct action_create_function;

class actions_base : public prop_interface
{

public:
    enum action_flags
    {
        NULL_ACTION_FLAG = 0,
        ELSE_ACTION_FLAG = 0b0001,
        PRE_ACTIVATE_ACTION_FLAG = 0b0010,
        POST_ACTIVATE_ACTION_FLAG = 0b0100,
    };

    //Note ** Do not remove depreciated enums from list. The trigger object saves out a list
    //of actions it contains and uses the enum cast to a s32 to identify the action. Changing
    //the order of the enums or removing enums in the middle will break this listing..**//

    enum action_types
    {
        INVALID_ACTION_TYPES = -1,

        TYPE_ACTION_PLAY_SOUND,                  //0
        TYPE_ACTION_ACTIVATE_OBJECT,             //1
        TYPE_ACTION_DEACTIVATE_OBJECT,           //2
        TYPE_ACTION_CREATE_OBJECT_FROM_TEMPLATE, //3
        TYPE_ACTION_MOVE_OBJECT,                 //4
        TYPE_ACTION_CHANGE_PLAYER_HEALTH,        //5
        TYPE_ACTION_CHANGE_STATE_VARIABLE,       //6
        TYPE_ACTION_GIVE_PLAYER_ITEM,            //7
        TYPE_ACTION_PLAY_SCRIPT,                 //8
        TYPE_ACTION_DESTORY_OBJECT,              //9
        TYPE_ACTION_CHANGE_AI_STATE,             //10 **deprectiated**
        TYPE_ACTION_AI_MODIFY_BEHAVIOR,          //11
        TYPE_ACTION_AI_MODIFY_BEHAVIOR_TARGETED, //12
        TYPE_ACTION_AI_MOVE_TO,                  //13
        TYPE_ACTION_SET_TIMER,                   //14
        TYPE_ACTION_DESTORY_THIS_TRIGGER,        //15
        TYPE_ACTION_MUSIC_INTENSITY,             //16
        TYPE_ACTION_CHANGE_OBJECT_HEALTH,        //17
        TYPE_ACTION_LOCK_PLAYER_VIEW,            //18
        TYPE_ACTION_PLAY_CINEMATIC_SCRIPT,       //19
        TYPE_ACTION_SAFE_SPOT_TRIGGER,           //20
        TYPE_ACTION_PLAY_CONVERSATION,           //21
        TYPE_ACTION_ACTIVATE_TASK,               //22
        TYPE_ACTION_CAUSE_DAMAGE,                //23
        TYPE_ACTION_CHANGE_PLAYER_STRAIN,        //24

        TYPE_ACTION_OPEN_AND_LOCK_DOOR,  //25 ** depreciated **
        TYPE_ACTION_CLOSE_AND_LOCK_DOOR, //26 ** depreciated **
        TYPE_ACTION_RESTORE_DOOR,        //27 ** depreciated **
        TYPE_ACTION_DOOR_LOGIC,          //28

        TYPE_ACTION_SET_ACTOR_FRIENDS, // 29
        TYPE_ACTION_SET_ACTOR_FACTION, // 30

        ACTION_TYPES_END
    };

    actions_base(guid ParentGuid, ObjectManager* om);
    virtual ~actions_base();

    virtual action_types GetType() { return INVALID_ACTION_TYPES; }
    virtual const char*  GetTypeName() { return "Action Base"; }
    virtual const char*  GetTypeInfo() { return "Base action class, null funtionality"; }
    virtual void         Execute(trigger_object* pParent) = 0;
    virtual void         OnEnumProp(prop_enum& rList);
    virtual bool         OnProperty(prop_query& rPropQuery);
    virtual void         OnRender() { /*no-op*/ }

    bool GetElse(void) { return m_ElseFlag; }
    void SetElse(bool ElseFalg) { m_ElseFlag = ElseFalg; }

public:
    static actions_base* CreateAction(const action_types& rType, const guid& rParentGuid);

public:
    static enum_table<action_types> m_ActionsAllEnum;       // Enumeration of the action types..
    static enum_table<action_types> m_ActionsMiscEnum;      // Enumeration of the action types..
    static enum_table<action_types> m_ActionsAIEnum;        // Enumeration of the action types..
    static enum_table<action_types> m_ActionsPlayerEnum;    // Enumeration of the action types..
    static enum_table<action_types> m_ActionsVariablesEnum; // Enumeration of the action types..
    static enum_table<action_types> m_ActionsDoorEnum;      // Enumeration of the action types..
    static enum_table<action_types> m_ActionsFactionsEnum;  // Enumeration of the action types..

    static void RegisterCreationFunction(action_create_function* pCreate);

protected:
    Vector3 GetPositionOwner();

protected:
    guid         m_ParentGuid; // Guid of the object which holds this action
    bool         m_ElseFlag;   // Flag if this action is an else action
    action_flags m_Flags;      // Flags which stores the various properties of this action

protected:
    ObjectManager*                 objectManager;
    static action_create_function* m_CreateHead; // Autotmatic registration list head node
};

//=========================================================================
// ACTION_CREATE_FUNCTION : used for automatic registeration of creation function for actions..
//=========================================================================

typedef actions_base* create_action_fn(guid ParentGuid);

struct action_create_function
{
    action_create_function(actions_base::action_types Type, create_action_fn* pCreateAction)
    {
        m_Type = Type;
        m_pCreateAction = pCreateAction;
        m_Next = nullptr;
    }

    actions_base::action_types m_Type;
    create_action_fn*          m_pCreateAction;
    action_create_function*    m_Next;
};

template <class ActionClass>
struct automatic_action_registeration
{
    automatic_action_registeration()
    {
        static action_create_function m_CreationObject(ActionClass::GetTypeStatic(), Create);

        actions_base::RegisterCreationFunction(&m_CreationObject);
    }

    static actions_base* Create(guid ParentGuid) { return new ActionClass(ParentGuid); }
};

//=========================================================================
/*//All of the actions basically have only 5 unique functions..
example_class::example_class ( guid ParentGuid ) : actions_base( ParentGuid )
{
}

//=========================================================================

void                example_class::Execute ( trigger_object* pParent )
{
    TRIGGER_CONTEXT( "ACTION * ai_modify_behavior_targeted::Execute" );
}

//=========================================================================

void                example_class::OnRender ( void )
{
}

//=========================================================================

void	            example_class::OnEnumProp ( prop_enum& rPropList )
{
    actions_base::OnEnumProp( rPropList );
}

//=========================================================================

xbool	            example_class::OnProperty ( prop_query& rPropQuery )
{
    if( actions_base::OnProperty( rPropQuery ) )
        return TRUE;

    return FALSE;
}
*/
