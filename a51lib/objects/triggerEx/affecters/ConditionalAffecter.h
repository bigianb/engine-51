#pragma once

#include "../../../RTTI.h"
#include "../../../Property.h"

#include "../TriggerExConditionals.h"


class trigger_ex_object;
class ObjectManager;

class conditional_affecter
{
public:
    CREATE_RTTI_BASE(conditional_affecter);

    enum evaluation
    {
        EVAL_COMMIT_IF,
        EVAL_COMMIT_ELSE,
        EVAL_NOT_MET
    };

    conditional_affecter(ObjectManager*, bool bAllowElse, int PropertyLevel);
    virtual ~conditional_affecter();

    void EnumPropSelector(prop_enum& rList);
    bool OnPropertySelector(prop_query& rPropQuery);

    void EnumPropConditionData(prop_enum& rList);
    bool OnPropertyConditionData(prop_query& rPropQuery);

    void EnumPropConditions(prop_enum& rList);
    void EnumPropElseConditions(prop_enum& rList);
    bool OnPropertyConditions(prop_query& rPropQuery);

    bool       CheckElseState();
    evaluation EvaluateConditions(guid TriggerGuid);

    inline int GetConditionCount() { return m_NumIfConditions; }
    inline int GetElseConditionCount() { return m_NumElseConditions; }
    int        GetConditionIndex(conditional_ex_base* pCondition);
    void       SetConditionIndex(conditional_ex_base* pCondition, int Index);
    void       RemoveCondition(conditional_ex_base* pCondition, bool bAndDelete = true);
    bool       IsFirstInConditionSet(conditional_ex_base* pCondition);

    bool CanSwitchElse(conditional_ex_base* pCondition);
    void SwitchElse(conditional_ex_base* pCondition);

    void RemoveAllConditions();

    ObjectManager* objectManager;
    
protected:
    enum
    {
        MAX_CONDITION_ARRAY_SIZE = 12
    };

    bool AddCondition(conditional_ex_base::conditional_ex_types ConditionType, int Number, bool bElse);

    bool EvaluateMainConditions(guid TriggerGuid);
    bool EvaluateElseConditions(guid TriggerGuid);

protected:
    //=========================================================================
    // Internal data structures and classes...
    //=========================================================================

    class condition_selector : public prop_interface
    {
    public:
        condition_selector();
        void         Init(conditional_affecter* pParent);
        virtual void OnEnumProp(prop_enum& rList);
        virtual bool OnProperty(prop_query& rPropQuery);

        conditional_ex_base::conditional_ex_types m_ConditionType; // Condtion type currently selected
        bool                                      m_Active;        // State Flag to determine if the selector is active..
        conditional_affecter*                     m_Parent;        // Parent of the selector
    };

    friend condition_selector;

protected:
    bool m_AllowElseInfo;  //does this condition allow else info?
    int  m_PropIndexLevel; //in the property grid, low many arrays deep is this?
    
    conditional_ex_base* m_IfConditions[MAX_CONDITION_ARRAY_SIZE];   // Ptr array for condtions
    conditional_ex_base* m_ElseConditions[MAX_CONDITION_ARRAY_SIZE]; // Ptr array for condtions
    int                  m_NumIfConditions;                          // Number of if conditions..
    int                  m_NumElseConditions;                        // Number of else conditions..
};
