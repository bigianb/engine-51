#pragma once

#include "../Property.h"

#define INGAME_CONTEXT (1 << 1)
#define INVENTORY_CONTEXT (1 << 2)
#define PAUSE_CONTEXT (1 << 3)
#define FRONTEND_CONTEXT (1 << 4)
#define LADDER_CONTEXT (1 << 5)

#define MAX_CONTEXT 5

#define INPUT_PLATFORM_PS2 0
#define INPUT_PLATFORM_XBOX 1
#define MAX_INPUT_PLATFORMS 2

// input values
#define INPUT_UNDEFINED 0
#define INPUT_MOUSE_BTN_L 1

typedef int input_gadget;

class input_pad : public prop_interface
{
public:
    struct logical
    {
        char  ActionName[48]; // Action Name
        float IsValue;        // Tells what value the current input gadget has
        float WasValue;       // Tells what value the gadget in a debounce state
        float MapsIsValue;
        float MapsWasValue;
        float TimePressed; // How long this gadget has been pressed (used for tap and hold)
    };

protected:
    struct map
    {

        uint32_t MapContext; // Which maps context does this mapping belongs to

        uint32_t     bButton : 1;     // Use to indicate wether is a stick or a button
        uint32_t     bIsTap : 1;      // Is the map for a button tap?
        uint32_t     bIsHold : 1;     // Is the map for a button hold?
        input_gadget GadgetID;        // Use to map to the physical input gadget
        float        Scale;           // Use for sensibility and revert axis
        int          iLogicalMapping; // This tells which logical slot does this value goes into
    };

public:
    input_pad();
    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);
    logical*     GetLogical(void) { return m_Logical; }
    map*         GetMap(int iPlatform) { return m_Map[iPlatform]; }
    int          GetNMaps(int iPlatform) { return m_nMaps[iPlatform]; }
    void         SetAllLogical(logical* Logical);
    void         SetAllMap(int  iPlatform,
                           map* Map,
                           int  nMaps);

    logical& GetLogical(int I)
    {
        assert(I >= 0 && I < MAX_LOGICAL);
        return m_Logical[I];
    }

    void SetLogical(int ID, const char* pName);
    void SetLogical(int iPlatform, input_gadget GadgetID);
    void AddMapping(int iPlatform, int ID, input_gadget GadgetID, bool IsButton, float Scale = 1);
    void GetMapping(int iPlatform, int ID, input_gadget& GadgetID, bool& IsButton, float& Scale);
    void DelMapping(int iPlatform, int iMapping);

    void EnableContext(uint32_t Context);
    void DisableContext(uint32_t Context);

    void SetControllerID(int ControllerID) { m_ControllerID = ControllerID; }
    int  GetControllerID() const { return m_ControllerID; }

    input_gadget GetGadgetIDFromName(int iPlatform, const char* pGadgetName);
    const char*  GetNameFromGadgetID(int iPlatform, input_gadget GadgetID);
    const char*  GetGadgetIDNames(int iPlatform);

    void ClearAllLogical();

protected:
    enum
    {
        MAX_LOGICAL = 48,
        MAX_MAPPINGS = 64,
    };

protected:
    void ClearMapping();

    virtual void OnUpdate(float DeltaTime);
    virtual void OnInitialize();

protected:
    logical    m_Logical[MAX_LOGICAL];                   // Logical data
    map        m_Map[MAX_INPUT_PLATFORMS][MAX_MAPPINGS]; // Logical To Physical Mapping
    int        m_nMaps[MAX_INPUT_PLATFORMS];             // Number of physical map input gagets
    input_pad* m_pNext;                                  // List of register input controlers
    uint32_t   m_ActiveContext;                          // All the contexts that are currently Active.
    int        m_ControllerID;                           // Which controller are reading the input from.
    const char*      m_pName;

protected:
    friend class input_mgr;
};

class input_mgr
{
public:
    input_mgr(void);
    bool Update(float DeltaTime);
    void RegisterPad(input_pad& Pad);
    int  WasPausePressed(void);

protected:
    static input_pad* s_pHead;
};

extern input_mgr g_InputMgr;
