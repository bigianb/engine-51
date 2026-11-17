#include "action_ai_base.h"
#include "../../../Property.h"
#include "../../../xfiles/xfs.h"

#include "../../../objectManager/ObjectManager.h"
//#include "..\Support\Characters\Character.hpp"
//#include "Objects\group.hpp"

action_ai_base::action_ai_base(guid ParentGuid, ObjectManager* om)
    : actions_ex_base(ParentGuid, om)
    , m_bBlockUntilComplete(false)
    , m_bIsRunningAction(false)
    , m_bIsBlockingAction(false)
    , m_bMustSucceed(false)
{
}

//=============================================================================

void action_ai_base::OnActivate(bool Flag)
{
    m_bIsRunningAction = false;
    m_bIsBlockingAction = false;
}

//=============================================================================

bool action_ai_base::Execute(float DeltaTime)
{
    if (m_bIsRunningAction) {
        if (m_bIsBlockingAction) {
            return false;
        } else {
            m_bIsRunningAction = false;
            return true;
        }
    }
    
        Object* pObject = m_CharacterAffecter.GetObjectPtr(objectManager);
        /*
        if ( pObject && pObject->IsKindOf( character::GetRTTI() ) )
        {
            character* pChar = (character*)pObject;
            if (pChar->TriggerState(this))
            {
                m_bIsRunningAction = true;

                if (m_bBlockUntilComplete)
                {
                    m_bIsBlockingAction = true;
                    return false;
                }
                else
                {
                    m_bIsRunningAction = false;
                    return true;
                }
            }
        }
        else if( pObject && pObject->IsKindOf( group::GetRTTI() ) )
        {
            bool foundChar = false;
            for( s32 i = 0 ; i < ((group*)pObject)->GetNumChildren() ; i ++ )
            {
                object* pChildObject = g_ObjMgr.GetObjectByGuid (((group*)pObject)->GetChild(i));
                if( pChildObject && pChildObject->IsKindOf( character::GetRTTI() ) )
                {
                    character* pChar = (character*)pChildObject;
                    if( pChar->TriggerState(this) )
                    {
                        m_bIsRunningAction = false;
                        foundChar = true;
                    }
                }
            }
            if( foundChar )
            {
                return true;
            }
        }
    */
    m_bErrorInExecute = true;
    if (RetryOnError()) {
        return false;
    } else {
        m_bIsRunningAction = false;
        return true;
    }
}

void action_ai_base::OnEnumProp(prop_enum& rPropList)
{
    if (m_TriggerGuid) {
        //guid specific fields
        m_CharacterAffecter.OnEnumProp(rPropList, "Character");

        rPropList.PropEnumBool("Block", "Is this AI action a blocking action? If so this action will block until the ai leaves the trigger state.", 0);
        rPropList.PropEnumBool("Must Succeed", "Does this action have to succeed for us to move on?", 0);
        m_ResponseFlags.OnEnumProp(rPropList);
    }
    actions_ex_base::OnEnumProp(rPropList);
}

//=============================================================================

bool action_ai_base::OnProperty(prop_query& rPropQuery)
{
    if (m_TriggerGuid) {
        //guid specific fields
        if (m_CharacterAffecter.OnProperty(rPropQuery, "Character")) {
            return true;
        }

        if (rPropQuery.VarBool("Block", m_bBlockUntilComplete)) {
            return true;
        }
        if (rPropQuery.VarBool("Must Succeed", m_bMustSucceed)) {
            return true;
        }
        if (m_ResponseFlags.OnProperty(rPropQuery)) {
            return true;
        }
    }

    if (actions_ex_base::OnProperty(rPropQuery)) {
        return true;
    }
    return false;
}

//=============================================================================

const char* action_ai_base::GetAIName()
{
    static std::string CharData;
    if (m_TriggerGuid) {
        if (m_bBlockUntilComplete) {
            CharData = xfs("* %s", m_CharacterAffecter.GetObjectInfo(objectManager));
        } else {
            CharData = m_CharacterAffecter.GetObjectInfo(objectManager);
        }

        return CharData.c_str();
    } else {
        return "AI";
    }
}
