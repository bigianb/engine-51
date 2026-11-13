#pragma once

#include "../../Guid.h"

class trigger_ex_object;
class ObjectManager;

class TriggerExManager
{
public:
    TriggerExManager(ObjectManager* pObjectManager);

    void OnUpdate(float DeltaTime);

    void RegisterTrigger(trigger_ex_object& rTrigger);   //called by the trigger object on OnInit
    void UnregisterTrigger(trigger_ex_object& rTrigger); //called to remove triggers from the mngr..
    void TriggerSleep(trigger_ex_object& rTrigger);
    void TriggerAwake(trigger_ex_object& rTrigger);

    //void DumpData(const char* pFileName);

protected:
    enum trigger_ex_class
    {
        INVALID_TRIGGER_EX_CLASS = -1,
        TRIGGER_EX_NO_UPDATE,
        TRIGGER_EX_UPDATE,
        TRIGGER_EX_CLASS_END
    };

    ObjectManager* om;

    trigger_ex_object* GetTriggerPtr(guid TriggerGuid);
    guid               m_ClassList[TRIGGER_EX_CLASS_END]; // List of Triggers
};
