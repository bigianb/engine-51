#pragma once

#include "../objectManager/ObjectManager.h"
#include "Trigger_Conditionals.h"
#include "Trigger_Actions.h"

class trigger_mngr;

//=========================================================================
// TRIGGER_OBJECT
//  NOTE : trigger_object are not contained in virtual space, only the
//          derived class spatial_trigger_object is contained within virutal space...
//=========================================================================

class trigger_object : public Object
{
public:
    CREATE_RTTI(trigger_object, Object, Object)
    trigger_object();
    ~trigger_object();

    BBox GetLocalBBox() const override;
    int  GetMaterial() const override { return MAT_TYPE_NULL; }

    void OnEnumProp(prop_enum& rList) override;
    bool OnProperty(prop_query& rPropQuery) override;
    void OnActivate(bool Flag) override;

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

    //=========================================================================
    // Interface functions for the various condtions and actions..
    //=========================================================================

    void                RemoveCondition(conditional_base* pCondition);
    void                RemoveAction(actions_base* pAction);
    virtual const guid* GetTriggerActor() { return nullptr; }
    void                KillTrigger();

protected:
    //=========================================================================
    // Various Enumerations..
    //=========================================================================

    enum trigger_state
    {
        TRIGGER_STATE_INVALID = -1,

        STATE_SLEEPING,
        STATE_CHECKING,
        STATE_DELAYING,
        STATE_RECOVERY,
        STATE_DYING,

        TRIGGER_STATE_END

    };

    enum trigger_type
    {
        TRIGGER_TYPE_INVALID = -1,

        TRIGGER_ONCE,
        TRIGGER_REPEATING,
        TRIGGER_REPEATING_COUNTED,

        TRIGGER_TYPE_END
    };

    enum
    {
        MAX_PTR_ARRAY_SIZE = 32
    };

protected:
    //=========================================================================
    // Internal data structures and classes...
    //=========================================================================

    class trigger_selector : public prop_interface
    {
    public:
        trigger_selector();
        void         Init(trigger_object* pParent);
        virtual void OnEnumProp(prop_enum& rList);
        virtual bool OnProperty(prop_query& rPropQuery);

        conditional_base::conditional_types m_ConditionType; // Condtion type currently selected
        actions_base::action_types          m_ActionType;    // Action type currently selected
        bool                                m_Active;        // State Flag to determine if the selector is active..
        trigger_object*                     m_Parent;        // Parent of the selector
        std::string                         m_VariableName;  // Name of the variable
    };

protected:
    void OnInit() override;
    void OnKill() override;

    //=========================================================================
    // Trigger Manager Interface functions...
    // The ExecuteLogic function does all the logic for this trigger, should be only called by the g_TriggerMngr.
    //=========================================================================

    virtual void ExecuteLogic(float DeltaTime);
    bool         IsAwake();

    //=========================================================================
    // Editor functionality..
    //=========================================================================

protected:
    void AddCondition(conditional_base::conditional_types ConditionType, int Number);
    void AddAction(actions_base::action_types ActionType, int Number);

    //=========================================================================
    // Property interface functions...
    // Functions with Dynamic in the name are used on loading to create the
    //  various triggers before they can be initailized by the property system.
    //=========================================================================

protected:
    void EnumPropDynamic(prop_enum& rList);
    void EnumPropConditions(prop_enum& rList);
    void EnumPropActions(prop_enum& rList);
    void EnumPropElseConditions(prop_enum& rList);
    void EnumPropElseActions(prop_enum& rList);

    bool OnPropertyDynamic(prop_query& rPropQuery);
    bool OnPropertyConditions(prop_query& rPropQuery);
    bool OnPropertyActions(prop_query& rPropQuery);

    void CalculateAndFlags();
    void CalculateOrFlags();

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

    bool EvaulateCondtions();
    bool EvaulateMainCondtions();
    bool EvaulateElseCondtions();
    void ExecuteAllActions();

    bool CheckElseState();

    //=========================================================================
    // Useful interface functions for derived classes and others..
    //=========================================================================

protected:
    void          SetTriggerState(const trigger_state State);
    trigger_state GetTriggerState() { return m_State; }
    void          ForceNextUpdate();
    bool          CanUpdate(float DeltaTime);

protected:
    conditional_base* m_Conditions[MAX_PTR_ARRAY_SIZE]; // Ptr array for condtions
    actions_base*     m_Actions[MAX_PTR_ARRAY_SIZE];    // Ptr array for actions

    float m_UpdateRate;   // How often this trigger updates if at all
    float m_RecoveryRate; // Amount of time from executing action to returning to checking, only applies to repeating triggers..
    float m_DelayRate;    // Amount of time from triggering of condition to execution of action to wait

    uint32_t m_AndFlags;     // And flags, all these bits must be true.
    uint32_t m_OrFlags;      // OR  flags, any of these bits can be true..
    uint32_t m_ElseAndFlags; // And flags for Else block, all these bits must be true.
    uint32_t m_ElseOrFlags;  // OR  flags for Else block, any of these bits can be true..

    int m_NumConditons; // Number of total conditions..
    int m_NumActions;   // Number of total actions...

    trigger_type m_Type; // The trigger type

    bool   m_DrawActivationSphere; // Debug functionality
    Colour m_CurrentColor;         // Debug functionality

    bool m_OnActivate;         // Flag which controls whether an object should update..
    int  m_RepeatCount;        // Max times a trigger is allowed to repeat..
    int  m_ActivateCount;      // Number of times a trigger has been activated..
    bool m_UseElse;            // Flag is on if there  are any else conditions or actions..
    bool m_ExecuteElseActions; // If the condtions of the else block resolve to true this flag will be true...

private:
    //the reason for making these private is because these functions affect variables
    //which the base trigger class fundementally depend upon to update correctly and should not be
    //changed directly but instead through the interface functions in order for the state machine
    //to correcly operate..

    void UpdateNextTime(float Time);
    bool CheckNextTime(float DeltaTime);

    trigger_state m_State;
    float         m_NextUpdateTime; // Next time for this trigger to update...

    //Trigger manager vars..
    guid m_Next;        // Link list for the triggers only to be used by TriggerManager
    guid m_Prev;        // Link list for the triggers only to be used by TriggerManager
    int  m_TriggerSlot; // Slot the trigger is sorted in the TriggerManager

    //State flags used to init correctly when we enter those states..
    bool m_EnteringDelay;    // Flags which get set upon entering Delay state used to keep update time correct
    bool m_EnteringRecovery; // Flags which get set upon entering Recovery state used to keep update time correct

    //////////////////////////////////////////////////////////////////////////////////////////////
    //FRIEND CLASSES

    friend trigger_selector;
    friend trigger_mngr;
};
