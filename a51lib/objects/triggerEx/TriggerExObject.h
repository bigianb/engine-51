#pragma once

#include "../../objectManager/ObjectManager.h"
#include "TriggerExActions.h"
#include "TriggerExManager.h"

//=========================================================================
// TRIGGER_OBJECT
//  NOTE : trigger_ex_object are not contained in virtual space, only the
//          derived class spatial_trigger_ex_object is contained within virutal space...
//=========================================================================

class trigger_ex_object : public Object
{
public:
    CREATE_RTTI(trigger_ex_object, Object, Object)
    trigger_ex_object(ObjectManager* om, collision_mgr* cm);
    virtual ~trigger_ex_object();

    BBox GetLocalBBox() const override;
    int  GetMaterial() const override { return MAT_TYPE_NULL; }

    void OnEnumProp(prop_enum& rList) override;
    bool OnProperty(prop_query& rPropQuery) override;

    void OnColCheck() override;
    void OnColNotify(Object& Object) override;
    void OnActivate(bool Flag) override;

    void OnMove(const Vector3& NewPos) override;

    void LogicCheckOnActivate();

    const object_desc& GetTypeDesc() const override;
    static const object_desc&  GetObjectType();

    //=========================================================================
    // Interface functions for the various condtions and actions..
    //=========================================================================

    inline int GetActionCount() { return m_NumIfActions; }
    inline int GetElseActionCount() { return m_NumElseActions; }

    actions_ex_base* GetAction(int Index);
    actions_ex_base* GetElseAction(int Index);

    bool IsFirstInActionSet(actions_ex_base* pAction);
    int  GetActionIndex(actions_ex_base* pAction);
    void SetActionIndex(actions_ex_base* pAction, int Index);
    void RemoveAction(actions_ex_base* pAction, bool bAndDelete = true);

    bool CanSwitchElse(actions_ex_base* pAction);
    void SwitchElse(actions_ex_base* pAction);

    virtual const guid* GetTriggerActor() { return NULL; }
    void                KillTrigger();
    void                ResetTrigger();

    inline float GetUpdateRate() { return m_UpdateRate; }
    void         ReleaseBlocking();

    bool HasDialogLine();

    // meta functions
    void GetValidLabels(bool bElseAction, char* pLabels);
    bool SetTriggerActionIndexToLabel(const char* pName);
    bool RetryOnError();

    void ForceStartTrigger();

public:
    //=========================================================================
    // Various Enumerations..
    //=========================================================================

    enum trigger_ex_state
    {
        TRIGGER_STATE_INVALID = -1,
        STATE_SLEEPING,
        STATE_CHECKING,
        STATE_DELAYING,
        STATE_RECOVERY,
        STATE_RESETTING,
        TRIGGER_STATE_END
    };

    enum trigger_ex_types
    {
        TRIGGER_TYPES_INVALID = -1,
        TRIGGER_TYPE_SIMPLE,
        TRIGGER_TYPE_SPATIAL,
        TRIGGER_TYPE_VIEWABLE,
        TRIGGER_TYPES_END
    };

    enum trigger_ex_occurs
    {
        TRIGGER_BEHAVIOR_INVALID = -1,
        TRIGGER_OCCURS_ONCE,
        TRIGGER_OCCURS_REPEATING,
        TRIGGER_BEHAVIOR_END
    };

    enum trigger_ex_reset
    {
        TRIGGER_RESET_INVALID = -1,
        TRIGGER_RESET_DESTROY,
        TRIGGER_RESET_DEACTIVATE,
        TRIGGER_RESET_END
    };

    // spatial specific enums

    enum trigger_ex_spatial_types
    {
        SPATIAL_TYPES_INVALID = -1,
        SPATIAL_TYPE_AXIS_CUBE,
        SPATIAL_TYPE_SPHERICAL,
        SPATIAL_TYPES_END
    };

    enum trigger_ex_spatial_activation_types
    {
        SPATIAL_ACTIVATION_TYPES_INVALID = -1,
        SPATIAL_ACTIVATE_ON_PLAYER,
        SPATIAL_ACTIVATE_ON_NPC,
        SPATIAL_ACTIVATE_ON_BULLET,
        SPATIAL_ACTIVATE_ON_NPC_OR_PLAYER,
        SPATIAL_ACTIVATE_ON_INTERNAL,
        SPATIAL_ACTIVATION_TYPES_END
    };

    enum trigger_ex_spatial_activation_bullet_types
    {
        SPATIAL_ACTIVATION_BULLET_TYPES_INVALID = -1,
        SPATIAL_ACTIVATE_BULLET_TYPES_ALL,
        SPATIAL_ACTIVATE_BULLET_TYPES_BULLET,
        SPATIAL_ACTIVATE_BULLET_TYPES_MES_ALT_FIRE,
        SPATIAL_ACTIVATE_BULLET_TYPES_NPC_BULLET,
        SPATIAL_ACTIVATE_BULLET_TYPES_PLAYER_BULLET,
        SPATIAL_ACTIVATION_BULLET_TYPES_END
    };

    enum trigger_ex_spatial_activation_response
    {
        SPATIAL_RESPONSE_TYPE_INVALID = -1,
        SPATIAL_RESPONSE_IF_STILL_INSIDE,
        SPATIAL_RESPONSE_RETURN_TO_SLEEP,
        SPATIAL_RESPONSE_TYPE_END
    };

    enum
    {
        MAX_ACTION_ARRAY_SIZE = 64
    };
    enum
    {
        MAX_SPATIAL_DIMENSIONS_PARAMS = 3
    };

    // specifically for description's debug rendering
    trigger_ex_types GetTriggerType() { return m_TriggerType; }
    void             OnRenderSpatial();

    inline bool IsActive()  override { return m_OnActivate; }

    trigger_ex_spatial_types GetSpatialType() { return m_SpatialType; };
    float                    GetDimension(int DimensionIndex)
    {
        if (IN_RANGE(0, DimensionIndex, 2)) {
            return m_Dimensions[DimensionIndex];
        } else {
            return 0.0;
        }
    };

    int    m_DrawActivationIcon; // Debug functionality
    bool   m_DrawError;
    Colour m_CurrentColor; // Debug functionality

protected:
    void OnInit() override;
    void OnKill() override;
    void OnRender() override;
    void OnAdvanceLogic(float DeltaTime) override;

    //=========================================================================
    // Trigger Manager Interface functions...
    // The ExecuteLogic function does all the logic for this trigger, should be only called by the g_TriggerMngr.
    //=========================================================================

    virtual void ExecuteLogic(float DeltaTime);
    bool         IsAwake();

    bool AddAction(actions_ex_base::action_ex_types ActionType, int Number, bool bAsElse);

    //=========================================================================
    // Property interface functions...
    // Functions with Dynamic in the name are used on loading to create the
    //  various triggers before they can be initailized by the property system.
    //=========================================================================

protected:
    void EnumPropDynamic(prop_enum& rList);
    void EnumPropActions(prop_enum& rList);
    void EnumPropElseActions(prop_enum& rList);
    void EnumPropSpatial(prop_enum& rPropList);

    bool OnPropertyDynamic(prop_query& rPropQuery);
    bool OnPropertyActions(prop_query& rPropQuery);
    bool OnPropertySpatial(prop_query& rPropQuery);

    //=========================================================================
    // Debug rendering function
    //=========================================================================

protected:
    void OnRenderActions();

    //=========================================================================
    // Various state execution functions and related..
    //=========================================================================

protected:
    void ExecuteSleeping(float DeltaTime);
    void ExecuteChecking(float DeltaTime);
    void ExecuteRecovery(float DeltaTime);
    void ExecuteDelaying(float DeltaTime);

    void ExecuteAllActions();
    bool ExecuteIndividualAction(actions_ex_base* pAction, int ActionIndex, float DeltaTime);

    //=========================================================================
    // Useful interface functions for derived classes and others..
    //=========================================================================

protected:
    void             SetTriggerState(const trigger_ex_state State);
    trigger_ex_state GetTriggerState() { return m_State; }
    void             ForceNextUpdate();
    bool             CanUpdate(float DeltaTime);

    void PreUpdateState();

    // spatial trigger functions

    void ActivateSpatialTrigger();
    void DeactivateSpatialTrigger();

    bool         QueryPlayerInVolume();
    bool         QueryNpcInVolume();
    bool         QueryBulletInVolume();
    virtual bool QueryObjectInVolume(Object* pObject);

protected:
    actions_ex_base* m_IfActions[MAX_ACTION_ARRAY_SIZE];   // Ptr array for actions
    actions_ex_base* m_ElseActions[MAX_ACTION_ARRAY_SIZE]; // Ptr array for actions

    // IJB conditional_affecter    m_ConditionAffecter;

    trigger_ex_types m_TriggerType; // What kind of Trigger is this, simple, spatial, viewable

    float m_UpdateRate;   // How often this trigger updates if at all
    float m_RecoveryRate; // Amount of time from executing action to returning to checking, only applies to repeating triggers..
    float m_DelayRate;    // Amount of time from triggering of condition to execution of action to wait

    int m_NumIfActions;   // Number of if actions...
    int m_NumElseActions; // Number of else actions...

    trigger_ex_occurs m_Behavior;  // The trigger behavior type
    trigger_ex_reset  m_ResetType; // What to do once the trigger has completed

    bool m_OnActivate;    // Flag which controls whether an object should update..
    int  m_RepeatCount;   // Max times a trigger is allowed to repeat..
    int  m_ActivateCount; // Number of times a trigger has been activated..
    bool m_bTriggerFired;

    enum action_status
    {
        ACTION_ONHOLD,
        ACTION_RUN_IF,
        ACTION_RUN_ELSE
    };

    action_status m_CurrentActionStatus;
    int           m_iCurrentActionIndex; // Are we doing advance logic's?

    // Spatial Variables
protected:
    float m_Dimensions[MAX_SPATIAL_DIMENSIONS_PARAMS]; // Dimensions of various types of triggers

    bool                                       m_IsSpatialActivated;   // Flag if the trigger has been activated...
    trigger_ex_spatial_activation_types        m_ActivationType;       // Types of objects which will turn on this trigger
    trigger_ex_spatial_activation_bullet_types m_ActivationBulletType; // Type of bullet that will trigger a bulleted spatial trigger
    trigger_ex_spatial_types                   m_SpatialType;          // The type of the trigger
    trigger_ex_spatial_activation_response     m_ResponseType;         // How the trigger responds to activation
    int                                        m_ActorGuidVarName;     // Name of the variable to store actor guid in
    xhandle                                    m_ActorGuidVarHandle;   // Global variable handle..
    bool                                       m_bRequiresLineOfSight; // Requires a ray test to validate view check
    unsigned int                               m_bLOSVerified;         // TRUE if a viewable trigger that requires LOS
                                                                       //   has been activated via a clear line of sight

    typedef enum_pair<trigger_ex_spatial_types>  spatial_pair;
    typedef enum_table<trigger_ex_spatial_types> spatial_table;

    static spatial_pair  s_SpatialPairTable[];
    static spatial_table s_SpatialEnumTable;

    typedef enum_pair<trigger_ex_spatial_activation_types>  activation_pair;
    typedef enum_table<trigger_ex_spatial_activation_types> activation_table;

    static activation_pair  s_ActivationPairTable[];
    static activation_table s_ActivationEnumTable;

    typedef enum_pair<trigger_ex_spatial_activation_bullet_types>  activation_bullet_pair;
    typedef enum_table<trigger_ex_spatial_activation_bullet_types> activation_bullet_table;

    static activation_bullet_pair  s_ActivationBulletTypePairTable[];
    static activation_bullet_table s_ActivationBulletTypeEnumTable;

    typedef enum_pair<trigger_ex_spatial_activation_response>  response_pair;
    typedef enum_table<trigger_ex_spatial_activation_response> response_table;

    static response_pair  s_ResponsePairTable[];
    static response_table s_ResponseEnumTable;

private:
    // the reason for making these private is because these functions affect variables
    // which the base trigger class fundementally depend upon to update correctly and should not be
    // changed directly but instead through the interface functions in order for the state machine
    // to correcly operate..

    void UpdateNextTime(float Time);
    bool CheckNextTime(float DeltaTime);

    trigger_ex_state m_State;
    float            m_NextUpdateTime; // Next time for this trigger to update...

    // Trigger manager vars..
    guid m_Next;        // Link list for the triggers only to be used by TriggerManager
    guid m_Prev;        // Link list for the triggers only to be used by TriggerManager
    int  m_TriggerSlot; // Slot the trigger is sorted in the TriggerManager

    // State flags used to init correctly when we enter those states..
    bool m_EnteringDelay;    // Flags which get set upon entering Delay state used to keep update time correct
    bool m_EnteringRecovery; // Flags which get set upon entering Recovery state used to keep update time correct
    bool m_bQuitRecovery;

    collision_mgr* collisionManager;

    //////////////////////////////////////////////////////////////////////////////////////////////
    // FRIEND CLASSES

    friend TriggerExManager;
    // IJB friend focus_object;
    // IJB friend check_point_mgr;
    friend void RunGame(void);
};

//=========================================================================

inline actions_ex_base* trigger_ex_object::GetAction(int Index)
{
    assert(Index >= 0);
    assert(Index < m_NumIfActions);

    return m_IfActions[Index];
}

//=========================================================================

inline actions_ex_base* trigger_ex_object::GetElseAction(int Index)
{
    assert(Index >= 0);
    assert(Index < m_NumElseActions);

    return m_ElseActions[Index];
}
