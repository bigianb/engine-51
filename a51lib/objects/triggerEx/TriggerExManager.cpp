#include "TriggerExManager.h"
#include "TriggerExObject.h"
#include "../Object.h"

TriggerExManager::TriggerExManager(ObjectManager* pObjectManager) : om(pObjectManager)
{
    m_ClassList[TRIGGER_EX_NO_UPDATE] = NULL_GUID;
    m_ClassList[TRIGGER_EX_UPDATE] = NULL_GUID;
}

trigger_ex_object* TriggerExManager::GetTriggerPtr(guid TriggerGuid)
{
    Object* pObject = nullptr;

    if (!om->IsGuidOfType(&pObject, TriggerGuid, trigger_ex_object::GetRTTI())) {
        return nullptr;
    }

    return (trigger_ex_object*)pObject;
}

//=============================================================================

void TriggerExManager::OnUpdate(float DeltaTime)
{

    //note this must be const reference, as its possible in some situations for the list to become
    //empty due to all active triggers being removed...
    const guid& rHeadGuid = m_ClassList[TRIGGER_EX_UPDATE];

    if (rHeadGuid == NULL_GUID) {
        return;
    }

    trigger_ex_object* pCurrentTrigger = GetTriggerPtr(rHeadGuid);

    assert(pCurrentTrigger);

    while (1) {
        guid Next = pCurrentTrigger->m_Next;

        //Note within ExecuteLogic the pCurrentTrigger can be destroyed and unregistered, so
        //that is why we store away the guids Next and not access pCurrentTrigger after calling
        //ExecuteLogic...

        pCurrentTrigger->ExecuteLogic(DeltaTime);

        pCurrentTrigger = nullptr;

        //HeadGuid can change after ExecuteLogic because triggers can be
        //removed, or destroyed from the active list..

        if (rHeadGuid == Next || rHeadGuid == NULL_GUID) {
            break;
        }

        pCurrentTrigger = GetTriggerPtr(Next);

        if (pCurrentTrigger == nullptr) {
            break;
        }
    }
}

void TriggerExManager::TriggerSleep(trigger_ex_object& rTrigger)
{
    if (rTrigger.m_TriggerSlot == TRIGGER_EX_NO_UPDATE) {
        return;
    }

    UnregisterTrigger(rTrigger);
    RegisterTrigger(rTrigger);
}

void TriggerExManager::TriggerAwake(trigger_ex_object& rTrigger)
{
    if (rTrigger.m_TriggerSlot == TRIGGER_EX_UPDATE) {
        return;
    }

    UnregisterTrigger(rTrigger);
    RegisterTrigger(rTrigger);
}

void TriggerExManager::RegisterTrigger(trigger_ex_object& rTrigger)
{
    //PDL::Insert the new trigger into the proper update list, depending upon
    //  their update need. Sleeping triggers go into the non-update list.

    //TODO: implement a priority queue for the updating triggers much more effiecnt
    //  than iterating through all updateable triggers.... -ddn

    //The list is cyclic.....

    guid* pHeadGuid = nullptr;

    if (rTrigger.IsAwake()) {
        pHeadGuid = &m_ClassList[TRIGGER_EX_UPDATE];
        rTrigger.m_TriggerSlot = TRIGGER_EX_UPDATE;
    } else {
        pHeadGuid = &m_ClassList[TRIGGER_EX_NO_UPDATE];
        rTrigger.m_TriggerSlot = TRIGGER_EX_NO_UPDATE;
    }

    guid NewTriggerGuid = rTrigger.GetGuid();

    if (*pHeadGuid == NULL_GUID) {
        rTrigger.m_Next = NewTriggerGuid;
        rTrigger.m_Prev = NewTriggerGuid;
        *pHeadGuid = NewTriggerGuid;

        return;
    }

    trigger_ex_object* pTriggerHead = GetTriggerPtr(*pHeadGuid);
    trigger_ex_object* pTriggerHeadPrev = nullptr;

    assert(pTriggerHead);

    pTriggerHeadPrev = GetTriggerPtr(pTriggerHead->m_Prev);

    assert(pTriggerHeadPrev);

    //check if we have a single element cyclic list
    if (pTriggerHead == pTriggerHeadPrev) {
        pTriggerHead->m_Next = NewTriggerGuid;
        pTriggerHead->m_Prev = NewTriggerGuid;

        rTrigger.m_Next = *pHeadGuid;
        rTrigger.m_Prev = *pHeadGuid;

        return;
    }

    //Insert the new trigger into the tail end of the cyclic list
    rTrigger.m_Next = *pHeadGuid;
    rTrigger.m_Prev = pTriggerHeadPrev->GetGuid();

    pTriggerHeadPrev->m_Next = NewTriggerGuid;
    pTriggerHead->m_Prev = NewTriggerGuid;

    return;
}

void TriggerExManager::UnregisterTrigger(trigger_ex_object& rTrigger)
{
    //PDL::Delete the trigger from the list
    //The list is cyclic...

    guid  DeleteTriggerGuid = rTrigger.GetGuid();
    guid* pHeadGuid = nullptr;

    if (rTrigger.m_TriggerSlot >= 0 && rTrigger.m_TriggerSlot < TRIGGER_EX_CLASS_END) {
        pHeadGuid = &m_ClassList[rTrigger.m_TriggerSlot];
    } else {
        //x_DebugMsg("TriggerExManager::UnregisterTrigger, Cannot find slot for given trigger.");
        assert(0);
        return;
    }

    if (*pHeadGuid == NULL_GUID) {
        return;
    }

    trigger_ex_object* pDeleteTrigger = GetTriggerPtr(DeleteTriggerGuid);
    trigger_ex_object* pDeleteTriggerPrev = nullptr;
    trigger_ex_object* pDeleteTriggerNext = nullptr;

    assert(pDeleteTrigger);

    pDeleteTriggerPrev = GetTriggerPtr(pDeleteTrigger->m_Prev);
    pDeleteTriggerNext = GetTriggerPtr(pDeleteTrigger->m_Next);

    //Check for single element cyclic link list
    if (pDeleteTrigger == pDeleteTriggerPrev) {
        *pHeadGuid = NULL_GUID;
        return;
    }

    //HACK - until we get someone to figure these triggers out, don't want them crashes Level Black

    //Check for 2nd to last element
    if (pDeleteTriggerNext == nullptr) {
        pDeleteTriggerNext = pDeleteTriggerPrev;
    }

    if (pDeleteTriggerPrev && pDeleteTriggerNext) {
        //Remove the delete trigger from the cyclic list...
        pDeleteTriggerPrev->m_Next = pDeleteTriggerNext->GetGuid();
        pDeleteTriggerNext->m_Prev = pDeleteTriggerPrev->GetGuid();

        if (*pHeadGuid == DeleteTriggerGuid) {
            *pHeadGuid = pDeleteTriggerNext->GetGuid();
        }
    } else {
        *pHeadGuid = NULL_GUID;
    }

    return;
}
