#pragma once

#include "../../PropertyEnum.h"
#include "../../Property.h"
#include "../../RTTI.h"
#include "../Object.h"

class trigger_ex_object;
class character_task;
class object_affecter;

struct action_ex_create_function;

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

class actions_ex_base : public prop_interface
{  
public:

    CREATE_RTTI_BASE( actions_ex_base );

    enum action_ex_flags
    {
        NULL_ACTION_FLAG            = 0,
        ELSE_ACTION_FLAG            = BIT(1),
        PRE_ACTIVATE_ACTION_FLAG    = BIT(2),
        POST_ACTIVATE_ACTION_FLAG   = BIT(3)
    };
    
    //Note ** Do not remove depreciated enums from list. The trigger object saves out a list
    //of actions it contains and uses the enum cast to a s32 to identify the action. Changing
    //the order of the enums or removing enums in the middle will break this listing..**//

    enum action_ex_types
    {
        INVALID_ACTION_TYPES = -1,
 
        TYPE_ACTION_DAMAGE_OBJECT,                   //0                
        TYPE_ACTION_DESTORY_OBJECT,                  //1
        TYPE_ACTION_OBJECT_ACTIVATION,               //2
        TYPE_ACTION_DOOR_LOGIC,                      //3
        TYPE_ACTION_CREATE_TEMPLATE,                 //4
        TYPE_ACTION_MOVE_OBJECT,                     //5
        TYPE_ACTION_PLAYER_INVENTORY,                //6
        TYPE_ACTION_PLAYER_CAMERA_SHAKE,             //7
        TYPE_ACTION_MUSIC_INTENSITY,                 //8
        TYPE_ACTION_DISPLAY_TEXT,                    //9
        TYPE_ACTION_MOVE_OBJECT_RELATIVE,            //10
        TYPE_ACTION_PORTAL_ACTIVATE,                 //11
        TYPE_ACTION_PLAYER_HUD,                      //12
        TYPE_ACTION_SAVE_GAME,                       //13
        TYPE_ACTION_FADE_GEOMETRY,                   //14
        TYPE_ACTION_SCREEN_FADE,                     //15
        TYPE_ACTION_MAN_TURRET,                      //16
        TYPE_ACTION_EXIT_TURRET,                     //17
        TYPE_ACTION_MISSION_FAILED,                  //18
 
        TYPE_ACTION_AI_ATTACK_GUID = 20,             //20
        TYPE_ACTION_AI_PATHTO_GUID,                  //21
        TYPE_ACTION_AI_LOOKAT_GUID,                  //22
        TYPE_ACTION_AI_DIALOG_LINE,                  //23
        TYPE_ACTION_AI_PLAY_ANIM,                    //24
        TYPE_ACTION_AI_SEARCHTO_GUID,                //25
        TYPE_ACTION_AI_INVENTORY,                    //26
        TYPE_ACTION_AI_NAV_ACTIVATION,               //27
        TYPE_ACTION_AI_DEATH,                        //28

        TYPE_ACTION_AFFECT_GLOBAL_VAR = 50,          //50
        TYPE_ACTION_AFFECT_PROPERTY,                 //51

        TYPE_META_LABEL = 100,                       //100
        TYPE_META_DELAY,                             //101
        TYPE_META_BLOCK,                             //102
        TYPE_META_GOTO,                              //103
        TYPE_META_BREAKPOINT,                        //104
        TYPE_META_CINEMA_BLOCK,                      //105

        TYPE_ACTION_LOAD_LEVEL = 120,                //120
        TYPE_ACTION_CHECKPOINT,                      //121

        TYPE_ACTION_PLAY_2D_SOUND = 130,             //130
        TYPE_ACTION_CHANGE_PERCEPTION,               //131

        ACTION_TYPES_END
    };
    
                    actions_ex_base                         ( guid ParentGuid );
    virtual        ~actions_ex_base                         ();

    virtual         action_ex_types     GetType             () { return INVALID_ACTION_TYPES;}
    virtual         const char*         GetTypeInfo         () { return "Base action class, null funtionality"; } 
    virtual         const char*         GetDescription      () { return "\0"; }
    virtual         bool               Execute             ( float DeltaTime ) = 0;    
    virtual			void	            OnEnumProp	        ( prop_enum& rList );
    virtual			bool	            OnProperty	        ( prop_query& rPropQuery );
    virtual         void                OnActivate          ( bool Flag ) { (void)Flag; }
    
    inline          bool               GetElse             () { return m_ElseFlag; }
    inline          void                SetElse             ( bool ElseFlag ) { m_ElseFlag = ElseFlag; }
    inline          guid                GetTriggerGuid      () { return m_TriggerGuid; }
    inline          void                SetTaskOwner        ( character_task* pTask ) { m_pTaskOwner = pTask; }

    inline          bool               WasExecuteAnError   () { return m_bErrorInExecute; }
    inline          void                ResetErrorRegister  () { m_bErrorInExecute = false; }
    
                    trigger_ex_object*  GetTriggerPtr       ();
                    bool               RetryOnError        ();

public:
    
    static          actions_ex_base*        CreateAction    ( const action_ex_types& rType , const guid& rParentGuid );
    
public:
 
    static enum_table<action_ex_types>      m_ActionsAllEnum;               // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsGeneralEnum;           // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsAIEnum;                // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsPlayerEnum;            // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsSpecificEnum;          // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsMetaEnum;              // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsInterfaceEnum;         // Enumeration of the action types..
    static enum_table<action_ex_types>      m_ActionsTaskEnum;              // Enumeration of the action types..

    static          void                    RegisterCreationFunction( action_ex_create_function* pCreate );

protected:

                    Vector3                 GetPositionOwner();
               
protected:
    
   guid                                     m_TriggerGuid;      // Guid of the object which holds this action
   character_task*                          m_pTaskOwner;       // pointer to the object which holds this action
   bool                                    m_ElseFlag;         // Flag if this action is an else action
   action_ex_flags                          m_Flags;            // Flags which stores the various properties of this action
   bool                                    m_bErrorInExecute;  // was an error detected?

protected:
    
   static action_ex_create_function*        m_CreateHead;       // Autotmatic registration list head node

};

//=========================================================================
// ACTION_CREATE_FUNCTION : used for automatic registeration of creation function for actions..
//=========================================================================

typedef actions_ex_base* create_action_ex_fn ( guid ParentGuid );

struct action_ex_create_function
{
    action_ex_create_function( actions_ex_base::action_ex_types Type, create_action_ex_fn* pCreateAction )
    {
        m_Type          = Type;
        m_pCreateAction = pCreateAction;
        m_Next          = NULL;
    }

    actions_ex_base::action_ex_types    m_Type;
    create_action_ex_fn*                m_pCreateAction;
    action_ex_create_function*          m_Next;
};

template < class ActionExClass > 
    struct automatic_action_ex_registeration
{
    automatic_action_ex_registeration()
    {
        static action_ex_create_function m_CreationObject( ActionExClass::GetTypeStatic(), Create );

        actions_ex_base::RegisterCreationFunction( &m_CreationObject );
    }
    
    static actions_ex_base*  Create( guid ParentGuid ) { return new ActionExClass(ParentGuid); }
};
