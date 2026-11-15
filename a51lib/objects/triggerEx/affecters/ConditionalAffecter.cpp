#include "ConditionalAffecter.h"
#include "../../../xfiles/xfs.h"

static const int MAX_BIT_SIZE_OF_FLAGS = 32;
static const int MAX_STRING_LEN = 255;

//=========================================================================
// INTERNAL CLASSES
//=========================================================================

conditional_affecter::condition_selector::condition_selector()
    : m_ConditionType(conditional_ex_base::TYPE_CONDITION_OBJECT_EXISTS)
    , m_Active(false)
    , m_Parent(nullptr)
{
}

//=============================================================================

void conditional_affecter::condition_selector::Init(conditional_affecter* pParent)
{
    m_Parent = pParent;
}

//=============================================================================

void conditional_affecter::condition_selector::OnEnumProp(prop_enum& rPropList)
{
    rPropList.PropEnumHeader("ConditionSelect", "Select the type of condition to add.", PROP_TYPE_HEADER);

    rPropList.PropEnumEnum("ConditionSelect\\General Conditions", conditional_ex_base::m_ConditionalGeneralEnum.BuildString(), "Types of General condtions available.", PROP_TYPE_DONT_SAVE);
    rPropList.PropEnumEnum("ConditionSelect\\Player Conditions", conditional_ex_base::m_ConditionalPlayerEnum.BuildString(), "Types of Player Specific condtions available.", PROP_TYPE_DONT_SAVE);
    rPropList.PropEnumButton("ConditionSelect\\If Condition", "Adds a new Condition into the list.", PROP_TYPE_MUST_ENUM);
    rPropList.PropEnumButton("ConditionSelect\\Else Condition", "Adds a new Condition into the list.", PROP_TYPE_MUST_ENUM);
}

//=============================================================================

bool conditional_affecter::condition_selector::OnProperty(prop_query& rPropQuery)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Selector to add conditions and actions
    /////////////////////////////////////////////////////////////////////////////////////////////////

    if (rPropQuery.IsVar("ConditionSelect\\If Condition")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarButton("Add as If Condition");
        } else {
            assert(m_Parent);
            if (m_Parent->AddCondition(m_ConditionType, m_Parent->m_NumIfConditions, false)) {
                m_Parent->m_NumIfConditions++;
            }
        }

        return true;
    }

    if (rPropQuery.IsVar("ConditionSelect\\Else Condition")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarButton("Add as Else Condition");
        } else {
            assert(m_Parent);
            if (m_Parent->AddCondition(m_ConditionType, m_Parent->m_NumElseConditions, true)) {
                m_Parent->m_NumElseConditions++;
            }
        }

        return true;
    }

    if (
        rPropQuery.IsVar("ConditionSelect\\General Conditions") |
        rPropQuery.IsVar("ConditionSelect\\Player Conditions")) {
        if (rPropQuery.IsRead()) {
            if (conditional_ex_base::m_ConditionalAllEnum.DoesValueExist(m_ConditionType)) {
                rPropQuery.SetVarEnum(conditional_ex_base::m_ConditionalAllEnum.GetString(m_ConditionType));
            } else {
                rPropQuery.SetVarEnum("INVALID");
            }
        } else {
            conditional_ex_base::conditional_ex_types ConditionType;
            if (conditional_ex_base::m_ConditionalAllEnum.GetValue(rPropQuery.GetVarEnum(), ConditionType)) {
                m_ConditionType = ConditionType;
            }
        }

        return (true);
    }

    return false;
}

//=========================================================================
// CLASS FUNCTIONS
//=========================================================================

conditional_affecter::conditional_affecter(bool bAllowElse, int PropertyLevel)
    : m_AllowElseInfo(bAllowElse)
    , m_PropIndexLevel(PropertyLevel)
    , m_NumIfConditions(0)
    , m_NumElseConditions(0)
{
    for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
        m_IfConditions[i] = nullptr;
        m_ElseConditions[i] = nullptr;
    }
}

//=========================================================================

conditional_affecter::~conditional_affecter(void)
{
    for (int j = 0; j < MAX_CONDITION_ARRAY_SIZE; j++) {
        if (m_IfConditions[j] != nullptr) {
            delete m_IfConditions[j];
            m_IfConditions[j] = nullptr;
        }

        if (m_ElseConditions[j] != nullptr) {
            delete m_ElseConditions[j];
            m_ElseConditions[j] = nullptr;
        }
    }
}

void conditional_affecter::EnumPropConditionData(prop_enum& rPropList)
{
    int iHeader = -1;
    if (m_PropIndexLevel == 0) {
        //Root Data
        rPropList.PropEnumHeader("RootConditionData", "", PROP_TYPE_DONT_SHOW);
        iHeader = rPropList.PushPath("RootConditionData\\");
    } else {
        //Child Data
        rPropList.PropEnumHeader("MetaConditionData", "", PROP_TYPE_DONT_SHOW);
        iHeader = rPropList.PushPath("MetaConditionData\\");
    }

    for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
        if (m_IfConditions[i] != nullptr) {
            rPropList.PropEnumInt(xfs("CIfType[%d]", i), "", PROP_TYPE_DONT_SHOW);
        }

        if (m_ElseConditions[i] != nullptr) {
            rPropList.PropEnumInt(xfs("CElseType[%d]", i), "", PROP_TYPE_DONT_SHOW);
        }
    }

    rPropList.PropEnumInt("NumIfConditions", "", PROP_TYPE_DONT_SHOW);
    rPropList.PropEnumInt("NumElseConditions", "", PROP_TYPE_DONT_SHOW);

    rPropList.PopPath(iHeader);
}

//===========================================================================

bool conditional_affecter::OnPropertyConditionData(prop_query& rPropQuery)
{
    int iHeader = -1;
    if (m_PropIndexLevel == 0) {
        //Root Data
        if (rPropQuery.IsSimilarPath("RootConditionData")) {
            iHeader = rPropQuery.PushPath("RootConditionData\\");
        } else {
            return false;
        }
    } else {
        //Child Data
        if (rPropQuery.IsSimilarPath("MetaConditionData")) {
            iHeader = rPropQuery.PushPath("MetaConditionData\\");
        } else {
            return false;
        }
    }

    if (rPropQuery.IsSimilarPath("CIfType[")) {
        int iIndex = rPropQuery.GetIndex(m_PropIndexLevel);
        assert(iIndex < MAX_CONDITION_ARRAY_SIZE && iIndex >= 0);

        if (m_IfConditions[iIndex] != nullptr && rPropQuery.IsRead()) {
            rPropQuery.SetVarInt(m_IfConditions[iIndex]->GetType());
        } else if (!rPropQuery.IsRead()) {
            int ConditionType = -1;
            ConditionType = rPropQuery.GetVarInt();
            AddCondition((conditional_ex_base::conditional_ex_types)ConditionType, iIndex, false);
        }

        return true;
    }

    if (rPropQuery.IsSimilarPath("CElseType[")) {
        int iIndex = rPropQuery.GetIndex(m_PropIndexLevel);
        assert(iIndex < MAX_CONDITION_ARRAY_SIZE && iIndex >= 0);

        if (m_ElseConditions[iIndex] != nullptr && rPropQuery.IsRead()) {
            rPropQuery.SetVarInt(m_ElseConditions[iIndex]->GetType());
        } else if (!rPropQuery.IsRead()) {
            int ConditionType = -1;
            ConditionType = rPropQuery.GetVarInt();
            AddCondition((conditional_ex_base::conditional_ex_types)ConditionType, iIndex, true);
        }

        return true;
    }

    if (rPropQuery.VarInt("NumIfConditions", m_NumIfConditions)) {
        return true;
    }

    if (rPropQuery.VarInt("NumElseConditions", m_NumElseConditions)) {
        return true;
    }

    rPropQuery.PopPath(iHeader);

    return false;
}

//=============================================================================

void conditional_affecter::EnumPropConditions(prop_enum& rPropList)
{
    if (GetConditionCount() > 0) {
        //we have at least 1 condition

        if (m_PropIndexLevel == 0) {
            //only add this for the root conditions
            rPropList.PropEnumHeader("If", "All the Conditions for this Item.", PROP_TYPE_HEADER);

            for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
                if (m_IfConditions[i]) {
                    assert(m_IfConditions[i]->GetElse() == false);

                    rPropList.PropEnumString(xfs("If\\Condition[%d]", i),
                                             m_IfConditions[i]->GetTypeInfo(), PROP_TYPE_HEADER);

                    int iHeader = rPropList.PushPath(xfs("If\\Condition[%d]\\", i));

                    m_IfConditions[i]->OnEnumProp(rPropList);

                    rPropList.PopPath(iHeader);
                }
            }
        } else //non-root
        {
            for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
                if (m_IfConditions[i]) {
                    assert(m_IfConditions[i]->GetElse() == false);

                    rPropList.PropEnumString(xfs("Conditional[%d]", i),
                                             m_IfConditions[i]->GetTypeInfo(), PROP_TYPE_HEADER);

                    int iHeader = rPropList.PushPath(xfs("Conditional[%d]\\", i));

                    m_IfConditions[i]->OnEnumProp(rPropList);

                    rPropList.PopPath(iHeader);
                }
            }
        }
    }
}

//=============================================================================

void conditional_affecter::EnumPropElseConditions(prop_enum& rPropList)
{
    if (GetElseConditionCount() > 0) {
        //we have at least 1 condition

        if (m_PropIndexLevel == 0) {
            //only add this for the root conditions
            rPropList.PropEnumHeader("Else If", "All the Else Conditions for this Item.", PROP_TYPE_HEADER);

            for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
                if (m_ElseConditions[i]) {
                    assert(m_ElseConditions[i]->GetElse() == true);

                    rPropList.PropEnumString(xfs("Else If\\Else Condition[%d]", i),
                                             m_ElseConditions[i]->GetTypeInfo(), PROP_TYPE_HEADER);

                    int iHeader = rPropList.PushPath(xfs("Else If\\Else Condition[%d]\\", i));

                    m_ElseConditions[i]->OnEnumProp(rPropList);

                    rPropList.PopPath(iHeader);
                }
            }
        } else //non - root
        {
            for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
                if (m_ElseConditions[i]) {
                    assert(m_ElseConditions[i]->GetElse() == true);

                    rPropList.PropEnumString(xfs("Else Conditional[%d]", i),
                                             m_ElseConditions[i]->GetTypeInfo(), PROP_TYPE_HEADER);

                    int iHeader = rPropList.PushPath(xfs("Else Conditional[%d]\\", i));

                    m_ElseConditions[i]->OnEnumProp(rPropList);

                    rPropList.PopPath(iHeader);
                }
            }
        }
    }
}

//=============================================================================

bool conditional_affecter::OnPropertyConditions(prop_query& rPropQuery)
{
    if (m_PropIndexLevel == 0) {
        if (rPropQuery.IsSimilarPath("If\\Condition")) {
            int iIndex = rPropQuery.GetIndex(m_PropIndexLevel);

            assert(iIndex < MAX_CONDITION_ARRAY_SIZE && iIndex >= 0 && m_IfConditions[iIndex]);

            if (rPropQuery.IsVar("If\\Condition[]")) {
                if (rPropQuery.IsRead()) {
                    if (IsFirstInConditionSet(m_IfConditions[iIndex])) {
                        rPropQuery.SetVarString(m_IfConditions[iIndex]->GetDescription(), MAX_STRING_LEN);
                    } else {
                        rPropQuery.SetVarString(xfs("%s %s", m_IfConditions[iIndex]->GetFlagStr(), m_IfConditions[iIndex]->GetDescription()), MAX_STRING_LEN);
                    }
                }

                return true;
            }

            if (m_IfConditions[iIndex]) {
                int iHeader = rPropQuery.PushPath("If\\Condition[]\\");

                if (m_IfConditions[iIndex]->OnProperty(rPropQuery)) {
                    rPropQuery.PopPath(iHeader);
                    return true;
                }

                rPropQuery.PopPath(iHeader);
            }
        }

        if (rPropQuery.IsSimilarPath("Else If\\Else Condition")) {
            int iIndex = rPropQuery.GetIndex(m_PropIndexLevel);

            assert(iIndex < MAX_CONDITION_ARRAY_SIZE && iIndex >= 0 && m_ElseConditions[iIndex]);

            if (rPropQuery.IsVar("Else If\\Else Condition[]")) {
                if (rPropQuery.IsRead()) {
                    if (IsFirstInConditionSet(m_ElseConditions[iIndex])) {
                        rPropQuery.SetVarString(m_ElseConditions[iIndex]->GetDescription(), MAX_STRING_LEN);
                    } else {
                        rPropQuery.SetVarString(xfs("%s %s", m_ElseConditions[iIndex]->GetFlagStr(), m_ElseConditions[iIndex]->GetDescription()), MAX_STRING_LEN);
                    }
                }

                return true;
            }

            if (m_ElseConditions[iIndex]) {
                int iHeader = rPropQuery.PushPath("Else If\\Else Condition[]\\");

                if (m_ElseConditions[iIndex]->OnProperty(rPropQuery)) {
                    rPropQuery.PopPath(iHeader);
                    return true;
                }

                rPropQuery.PopPath(iHeader);
            }
        }
    } else //non-root
    {
        if (rPropQuery.IsSimilarPath("Conditional")) {
            int iIndex = rPropQuery.GetIndex(m_PropIndexLevel);

            assert(iIndex < MAX_CONDITION_ARRAY_SIZE && iIndex >= 0 && m_IfConditions[iIndex]);

            if (rPropQuery.IsVar("Conditional[]")) {
                if (rPropQuery.IsRead()) {
                    if (IsFirstInConditionSet(m_IfConditions[iIndex])) {
                        rPropQuery.SetVarString(m_IfConditions[iIndex]->GetDescription(), MAX_STRING_LEN);
                    } else {
                        rPropQuery.SetVarString(xfs("%s %s", m_IfConditions[iIndex]->GetFlagStr(), m_IfConditions[iIndex]->GetDescription()), MAX_STRING_LEN);
                    }
                }

                return true;
            }

            if (m_IfConditions[iIndex]) {
                int iHeader = rPropQuery.PushPath("Conditional[]\\");

                if (m_IfConditions[iIndex]->OnProperty(rPropQuery)) {
                    rPropQuery.PopPath(iHeader);
                    return true;
                }

                rPropQuery.PopPath(iHeader);
            }
        }

        if (rPropQuery.IsSimilarPath("Else Conditional")) {
            int iIndex = rPropQuery.GetIndex(m_PropIndexLevel);

            assert(iIndex < MAX_CONDITION_ARRAY_SIZE && iIndex >= 0 && m_ElseConditions[iIndex]);

            if (rPropQuery.IsVar("Else Conditional[]")) {
                if (rPropQuery.IsRead()) {
                    if (IsFirstInConditionSet(m_ElseConditions[iIndex])) {
                        rPropQuery.SetVarString(m_ElseConditions[iIndex]->GetDescription(), MAX_STRING_LEN);
                    } else {
                        rPropQuery.SetVarString(xfs("%s %s", m_ElseConditions[iIndex]->GetFlagStr(), m_ElseConditions[iIndex]->GetDescription()), MAX_STRING_LEN);
                    }
                }

                return true;
            }

            if (m_ElseConditions[iIndex]) {
                int iHeader = rPropQuery.PushPath("Else Conditional[]\\");

                if (m_ElseConditions[iIndex]->OnProperty(rPropQuery)) {
                    rPropQuery.PopPath(iHeader);
                    return true;
                }

                rPropQuery.PopPath(iHeader);
            }
        }
    }

    return false;
}

//=============================================================================

bool conditional_affecter::CheckElseState(void)
{
    //return true if we are a trigger which uses an else block...
    if (m_NumElseConditions > 0) {
        return true;
    } else {
        return false;
    }
}

//=============================================================================

conditional_affecter::evaluation conditional_affecter::EvaluateConditions(guid TriggerGuid)
{
    //evaluate all conditions with respect to the else flags...
    if (EvaluateMainConditions(TriggerGuid)) {
        return EVAL_COMMIT_IF;
    } else if (EvaluateElseConditions(TriggerGuid)) {
        return EVAL_COMMIT_ELSE;
    }

    return EVAL_NOT_MET;
}

//=============================================================================

bool conditional_affecter::EvaluateMainConditions(guid TriggerGuid)
{
    bool Rval = true;

    for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
        assert(i < MAX_BIT_SIZE_OF_FLAGS);

        if (m_IfConditions[i] == nullptr) {
            continue;
        }

        assert(m_IfConditions[i]->GetElse() == false);

        if ((!Rval) && (m_IfConditions[i]->GetFlag() == conditional_ex_base::FLAG_AND)) {
            //this is an and condition, and we the current state is failure, so
            //no reason to perform this condition check (save some cpu)
            continue;
        }

        if (m_IfConditions[i]->Execute(TriggerGuid)) {
            //condition met, if and leave alone, if or the set return val true
            if (m_IfConditions[i]->GetFlag() == conditional_ex_base::FLAG_OR) {
                Rval = true;
            }
        } else {
            //condition failure, if and set false, if or no change unless it is first in set
            if ((m_IfConditions[i]->GetFlag() == conditional_ex_base::FLAG_AND) ||
                ((i == 0) && (m_IfConditions[i]->GetFlag() == conditional_ex_base::FLAG_OR))) {
                Rval = false;
            }
        }
    }

    return Rval;
}

//=============================================================================

bool conditional_affecter::EvaluateElseConditions(guid TriggerGuid)
{
    bool Rval = true;

    for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
        assert(i < MAX_BIT_SIZE_OF_FLAGS);

        if (m_ElseConditions[i] == nullptr) {
            continue;
        }

        assert(m_ElseConditions[i]->GetElse() == true);

        if ((!Rval) && (m_ElseConditions[i]->GetFlag() == conditional_ex_base::FLAG_AND)) {
            //this is an and condition, and we the current state is failure, so
            //no reason to perform this condition check (save some cpu)
            continue;
        }

        if (m_ElseConditions[i]->Execute(TriggerGuid)) {
            //condition met, if and leave alone, if or the set return val true
            if (m_ElseConditions[i]->GetFlag() == conditional_ex_base::FLAG_OR) {
                Rval = true;
            }
        } else {
            //condition failure, if and set false, if or no change unless it is first in set
            if ((m_ElseConditions[i]->GetFlag() == conditional_ex_base::FLAG_AND) ||
                ((i == 0) && (m_ElseConditions[i]->GetFlag() == conditional_ex_base::FLAG_OR))) {
                Rval = false;
            }
        }
    }

    return Rval;
}

//=============================================================================

bool conditional_affecter::AddCondition(conditional_ex_base::conditional_ex_types ConditionType, int Number, bool bElse)
{
    if (Number >= 0 && Number < MAX_CONDITION_ARRAY_SIZE) {
        if (!bElse) {
            if (m_IfConditions[Number]) {
                delete m_IfConditions[Number];
                m_IfConditions[Number] = nullptr;
            }

            m_IfConditions[Number] = conditional_ex_base::CreateCondition(ConditionType, this);
            assert(m_IfConditions[Number]);
            m_IfConditions[Number]->SetElse(false);
            m_IfConditions[Number]->AllowElse(m_AllowElseInfo);
        } else {
            if (m_ElseConditions[Number]) {
                delete m_ElseConditions[Number];
                m_ElseConditions[Number] = nullptr;
            }

            m_ElseConditions[Number] = conditional_ex_base::CreateCondition(ConditionType, this);
            assert(m_ElseConditions[Number]);
            m_ElseConditions[Number]->SetElse(true);
            m_ElseConditions[Number]->AllowElse(m_AllowElseInfo);
        }
        return true;
    } else {
        //x_try;
        //x_throw("You can not add any more conditions to this item!");
        //x_catch_display;
        return false;
    }
}

//=============================================================================

int conditional_affecter::GetConditionIndex(conditional_ex_base* pCondition)
{
    for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
        if (!pCondition->GetElse()) {
            if (m_IfConditions[i] == pCondition) {
                return i;
            }
        } else {
            if (m_ElseConditions[i] == pCondition) {
                return i;
            }
        }
    }

    return -1;
}

//=============================================================================

void conditional_affecter::SetConditionIndex(conditional_ex_base* pCondition, int Index)
{
    if (Index < 0 || Index >= MAX_CONDITION_ARRAY_SIZE) {
        assert(false);
        return;
    }

    if (!pCondition) {
        return;
    }

    if (!pCondition->GetElse()) {
        //if the index is too high, set it to the highest used index
        if (Index >= m_NumIfConditions) {
            Index = m_NumIfConditions - 1;
        }

        for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
            if (m_IfConditions[i] == pCondition) {
                //found the right condition, first shift the array to fill in the empty spot
                for (i++; i < MAX_CONDITION_ARRAY_SIZE; i++) {
                    m_IfConditions[i - 1] = m_IfConditions[i];
                }
                m_IfConditions[MAX_CONDITION_ARRAY_SIZE - 1] = nullptr;

                //now clear out the desired index
                for (i = MAX_CONDITION_ARRAY_SIZE - 1; i > Index; i--) {
                    m_IfConditions[i] = m_IfConditions[i - 1];
                }
                m_IfConditions[Index] = pCondition;

                return;
            }
        }
    } else {
        //if the index is too high, set it to the highest used index
        if (Index >= m_NumElseConditions) {
            Index = m_NumElseConditions - 1;
        }

        for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
            if (m_ElseConditions[i] == pCondition) {
                //found the right condition, first shift the array to fill in the empty spot
                for (i++; i < MAX_CONDITION_ARRAY_SIZE; i++) {
                    m_ElseConditions[i - 1] = m_ElseConditions[i];
                }
                m_ElseConditions[MAX_CONDITION_ARRAY_SIZE - 1] = nullptr;

                //now clear out the desired index
                for (i = MAX_CONDITION_ARRAY_SIZE - 1; i > Index; i--) {
                    m_ElseConditions[i] = m_ElseConditions[i - 1];
                }
                m_ElseConditions[Index] = pCondition;

                return;
            }
        }
    }

    assert(false);
}

//=============================================================================

void conditional_affecter::RemoveCondition(conditional_ex_base* pCondition, bool bAndDelete)
{
    int i = 0;

    if (!pCondition) {
        return;
    }

    if (!pCondition->GetElse()) {
        for (i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
            if (m_IfConditions[i] == pCondition) {
                if (bAndDelete) {
                    delete m_IfConditions[i];
                }

                m_IfConditions[i] = nullptr;
                break;
            }
        }

        if (i == MAX_CONDITION_ARRAY_SIZE) {
            //x_DebugMsg("conditional_affecter::RemoveCondition, Cannot find condition in table.");
            assert(0);
            return;
        }

        //Shift the array to remove the empty slot...
        for (i++; i < MAX_CONDITION_ARRAY_SIZE; i++) {
            m_IfConditions[i - 1] = m_IfConditions[i];
        }

        m_IfConditions[MAX_CONDITION_ARRAY_SIZE - 1] = nullptr;
        m_NumIfConditions--;
    } else {
        for (i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
            if (m_ElseConditions[i] == pCondition) {
                if (bAndDelete) {
                    delete m_ElseConditions[i];
                }

                m_ElseConditions[i] = nullptr;
                break;
            }
        }

        if (i == MAX_CONDITION_ARRAY_SIZE) {
            //x_DebugMsg("conditional_affecter::RemoveCondition, Cannot find condition in table.");
            assert(0);
            return;
        }

        //Shift the array to remove the empty slot...
        for (i++; i < MAX_CONDITION_ARRAY_SIZE; i++) {
            m_ElseConditions[i - 1] = m_ElseConditions[i];
        }

        m_ElseConditions[MAX_CONDITION_ARRAY_SIZE - 1] = nullptr;
        m_NumElseConditions--;
    }
}

//=============================================================================

bool conditional_affecter::IsFirstInConditionSet(conditional_ex_base* pCondition)
{
    if (pCondition) {
        if (!pCondition->GetElse()) {
            if (m_IfConditions[0] == pCondition) {
                return true;
            }
        } else {
            if (m_ElseConditions[0] == pCondition) {
                return true;
            }
        }

        return false;
    }

    assert(false);
    return false;
}

//=============================================================================

bool conditional_affecter::CanSwitchElse(conditional_ex_base* pCondition)
{
    assert(pCondition);
    if (pCondition) {
        if (!pCondition->GetElse()) {
            if (GetElseConditionCount() < MAX_CONDITION_ARRAY_SIZE) {
                return true;
            }
        } else {
            if (GetConditionCount() < MAX_CONDITION_ARRAY_SIZE) {
                return true;
            }
        }
    }
    return false;
}

//=============================================================================

void conditional_affecter::SwitchElse(conditional_ex_base* pCondition)
{
    assert(pCondition);
    if (pCondition && CanSwitchElse(pCondition)) {
        RemoveCondition(pCondition, false);
        pCondition->SetElse(!pCondition->GetElse());

        if (!pCondition->GetElse()) {
            assert(!m_IfConditions[m_NumIfConditions]);
            m_IfConditions[m_NumIfConditions] = pCondition;
            m_NumIfConditions++;
        } else {
            assert(!m_ElseConditions[m_NumElseConditions]);
            m_ElseConditions[m_NumElseConditions] = pCondition;
            m_NumElseConditions++;
        }
    }
}

//=============================================================================

void conditional_affecter::RemoveAllConditions(void)
{
    for (int i = 0; i < MAX_CONDITION_ARRAY_SIZE; i++) {
        if (m_IfConditions[i]) {
            delete m_IfConditions[i];
            m_IfConditions[i] = nullptr;
        }

        if (m_ElseConditions[i]) {
            delete m_ElseConditions[i];
            m_ElseConditions[i] = nullptr;
        }
    }

    m_NumElseConditions = 0;
    m_NumIfConditions = 0;
}

