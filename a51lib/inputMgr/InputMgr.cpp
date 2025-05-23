#include "InputMgr.h"
#include "../xfiles/xfs.h"

//#include "StateMgr\StateMgr.hpp"

input_mgr  g_InputMgr;
input_pad* input_mgr::s_pHead = nullptr;

extern bool SuppressFeedback;

static const int FIRST_INPUT_TEXT_LINE = 10;
static const int INPUT_TEXT_COLUMN = 10;

input_pad::input_pad(void)
{
    for (int i = 0; i < MAX_INPUT_PLATFORMS; i++) {
        m_nMaps[i] = 0;
        memset(m_Map[i], 0, sizeof(map) * MAX_MAPPINGS);
    }

    memset(m_Logical, 0, sizeof(logical) * MAX_LOGICAL);
    m_ControllerID = -1;
    m_ActiveContext = 0;
}

void input_pad::ClearMapping(void)
{
    for (int i = 0; i < MAX_INPUT_PLATFORMS; i++) {
        memset(m_Map[i], 0, sizeof(map) * MAX_MAPPINGS);
        m_nMaps[i] = 0;
    }
}

void input_pad::SetLogical(int ID, const char* pName)
{

    strncpy(m_Logical[ID].ActionName, pName, 48);

    m_Logical[ID].IsValue = 0;
    m_Logical[ID].WasValue = 0;
}

void input_pad::ClearAllLogical()
{
    for (int i = 0; i < MAX_LOGICAL; i++) {
        m_Logical[i].IsValue = 0;
        m_Logical[i].WasValue = 0;
    }
}

void input_pad::AddMapping(int          iPlatform,
                           int          ID,
                           input_gadget GadgetID,
                           bool         IsButton,
                           float        Scale)
{
    assert(m_nMaps[iPlatform] <= MAX_MAPPINGS);
    assert(ID <= MAX_LOGICAL);

    m_Map[iPlatform][m_nMaps[iPlatform]].bButton = IsButton;
    m_Map[iPlatform][m_nMaps[iPlatform]].GadgetID = GadgetID;
    m_Map[iPlatform][m_nMaps[iPlatform]].Scale = Scale;
    m_Map[iPlatform][m_nMaps[iPlatform]].iLogicalMapping = ID;

    m_nMaps[iPlatform]++;
}

void input_pad::GetMapping(int           iPlatform,
                           int           ID,
                           input_gadget& GadgetID,
                           bool&         IsButton,
                           float&        Scale)
{
    for (int i = 0; i < m_nMaps[iPlatform]; i++) {
        if (m_Map[iPlatform][i].iLogicalMapping == ID) {
            IsButton = m_Map[iPlatform][i].bButton;
            GadgetID = m_Map[iPlatform][i].GadgetID;
            Scale = m_Map[iPlatform][i].Scale;
            break;
        }
    }
}

void input_pad::DelMapping(int iPlatform, int iMapping)
{
    assert(iMapping <= MAX_MAPPINGS);
    assert(iMapping <= m_nMaps[iPlatform]);

    for (int i = iMapping; i < m_nMaps[iPlatform]; i++) {
        m_Map[iPlatform][i] = m_Map[iPlatform][i + 1];
    }

    m_nMaps[iPlatform]--;
}

void input_pad::EnableContext(uint32_t Context)
{
    m_ActiveContext |= Context;
}

void input_pad::DisableContext(uint32_t Context)
{
    m_ActiveContext = (m_ActiveContext & ~Context);
}

void input_pad::OnUpdate(float DeltaTime)
{
    if (m_ControllerID == -1) {
        return;
    }

    /* IJB

    int ControllerID = m_ControllerID;

    for (int i = 0; i < m_nMaps[iPlatform]; i++) {
        map&     Map = m_Map[iPlatform][i];
        logical& Log = m_Logical[Map.iLogicalMapping];

        if (Map.bButton) {
            //
            // If the user wants to treat some of the analog buttons of the PS2
            // as digitals, we will handle it for him.
            //

            float IsPress = (float)input_IsPressed(Map.GadgetID, ControllerID);

            if (Map.bIsTap || Map.bIsHold) {
                //
                // Handle tap and hold by setting up IsPress accordingly
                //
                bool ClearTime = !IsPress;

                if (IsPress) {
                    Log.TimePressed += DeltaTime;
                }

                static float s_TapTime = 0.3f;
                if (Map.bIsTap) {
                    // if the button was just released, and we're under the
                    // time threshold, then we just tapped
                    if (!IsPress && (Log.TimePressed > 0.0f) && (Log.TimePressed < s_TapTime)) {
                        IsPress = 1.0f; // have to force a value since the button isn't actually pressed
                    } else if (IsPress) {
                        // We haven't let go of the button yet, no tap here
                        IsPress = 0.0f;
                    }
                } else {
                    assert(Map.bIsHold);

                    // if the button is pressed, and we're over the time
                    // threshold, then we are officially holding, so leave IsPress alone
                    // If not, then cancel the IsPress
                    if (IsPress && (Log.TimePressed < s_TapTime)) {
                        IsPress = 0.0f; // not holding
                    } else if (!IsPress) {
                        // We aren't holding the button down any more, clear the timer
                        ClearTime = true;
                    }
                }

                if (ClearTime) {
                    Log.TimePressed = 0.0f;
                }
            }

            if (IsPress != Log.IsValue) {
                if (IsPress > Log.MapsWasValue) {
                    Log.MapsWasValue = IsPress;
                }
            }

            if (IsPress > Log.MapsIsValue) {
                Log.MapsIsValue = IsPress;
            }
        } else {
            float Value = 0.0f;

            if (Map.GadgetID != INPUT_UNDEFINED) {
                Value = input_GetValue(Map.GadgetID, ControllerID);
            }

            Log.MapsWasValue = Log.IsValue;
            Log.MapsIsValue = Map.Scale * Value;
        }
    }

    // Store the logical values.
    for (i = 0; i < MAX_LOGICAL; i++) {
        m_Logical[i].IsValue = m_Logical[i].MapsIsValue;
        m_Logical[i].WasValue = m_Logical[i].MapsWasValue;

        m_Logical[i].MapsIsValue = 0.0f;
        m_Logical[i].MapsWasValue = 0.0f;
    }

    */
}

void input_pad::OnInitialize()
{
}

void input_pad::OnEnumProp(prop_enum& List)
{
    for (int iPlatform = 0; iPlatform < MAX_INPUT_PLATFORMS; iPlatform++) {
        int iHeader = 0;

        switch (iPlatform) {
        default:
            assert(false);
            break;
        }

        List.PropEnumHeader("InputPad", "This is a manager used to attack physical devices to logical actions", PROP_TYPE_HEADER);

        List.PropEnumInt("InputPad\\NMaps", "This is for loading and saving this should not be expouse to users", PROP_TYPE_DONT_SHOW);

        for (int i = 0; i < MAX_LOGICAL; i++) {

            if (m_Logical[i].ActionName[0] == 0) {
                continue;
            }

            List.PropEnumString(xfs("InputPad\\LogicalMap[%d]", i), "A logical map is like an specific command for the player such Jump. Where multiple keys could be mapped to it in order to activate that command", PROP_TYPE_HEADER);
            List.PropEnumButton(xfs("InputPad\\LogicalMap[%d]\\AddMapping", i), "Creates a new mapping", PROP_TYPE_DONT_SAVE | PROP_TYPE_MUST_ENUM);

            for (int j = 0; j < m_nMaps[iPlatform]; j++) {
                if (m_Map[iPlatform][j].iLogicalMapping == i) {
                    List.PropEnumString(xfs("InputPad\\LogicalMap[%d]\\Map[%d]", i, j), "A map is use to attach a physical device 'key' to a logical action", PROP_TYPE_HEADER);
                    List.PropEnumInt(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\iLogicalMapping", i, j), "This is use for save and load.", PROP_TYPE_DONT_SHOW);
                    List.PropEnumButton(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\DelMapping", i, j), "Deletes this mapping", PROP_TYPE_DONT_SAVE | PROP_TYPE_MUST_ENUM);
//                    List.PropEnumEnum(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\GadgetID", i, j), GetGadgetIDNames(iPlatform), "This is the physical device which the logical action is map to", 0);

                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\IsButton", i, j), "Tells whether the gadget in question is setup as a button", PROP_TYPE_MUST_ENUM);
                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\IsTap", i, j), "Tells whether the gadget in question is setup as a button tap", PROP_TYPE_MUST_ENUM);
                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\IsHold", i, j), "Tells whether the gadget in question is setup as a button hold", PROP_TYPE_MUST_ENUM);
                    List.PropEnumFloat(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\Scale", i, j), "Tells Whether it needs any scale for the input. Usually is just (1 or -1).", 0);

                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\InGame Context", i, j), "This map has In game context", 0);
                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\Inventory Context", i, j), "This map has inventory context", 0);
                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\Pause Context", i, j), "This map has pause context", 0);
                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\FrontEnd Context", i, j), "This map has front end context", 0);

                    // TO_ADD_CONTEXT
                    List.PropEnumBool(xfs("InputPad\\LogicalMap[%d]\\Map[%d]\\Ladder Context", i, j), "This map has ladder context", 0);
                }
            }
        }

        List.PopPath(iHeader);
    }
}

bool input_pad::OnProperty(prop_query& I)
{
    bool Boolean;

    for (int i = 0; i < MAX_INPUT_PLATFORMS; i++) {
        int iHeader = 0;
        int iIndex0 = I.GetIndex(0);
        int iIndex1 = I.GetIndex(1);

        switch (i) {
        default:
            assert(false);
            break;
        }

        if (I.VarInt("InputPad\\NMaps", m_nMaps[i])) {
            return true;
        }

        if (I.VarString("InputPad\\LogicalMap[]", m_Logical[iIndex0].ActionName, 48)) {
            return true;
        }

        if (I.IsVar("InputPad\\LogicalMap[]\\AddMapping")) {
            if (I.IsRead()) {
                I.SetVarButton("AddMapping");
            } else {
/* IJB
                if (i == INPUT_PLATFORM_PS2) {
                    AddMapping(i, iIndex0, INPUT_PS2_STICK_LEFT_X, false);
                } else {
                    AddMapping(i, iIndex0, INPUT_XBOX_STICK_LEFT_X, false);
                }
*/
            }

            return true;
        }

        if (I.IsVar("InputPad\\LogicalMap[]\\Map[]\\DelMapping")) {
            if (I.IsRead()) {
                I.SetVarButton("Delete");
            } else {
                DelMapping(i, iIndex1);
            }

            return true;
        }

        if (I.VarInt("InputPad\\LogicalMap[]\\Map[]\\iLogicalMapping", m_Map[i][iIndex1].iLogicalMapping)) {
            return true;
        }

        if (I.VarString("InputPad\\LogicalMap[]\\Map[]", (char*)GetNameFromGadgetID(i, m_Map[i][iIndex1].GadgetID), 256)) {
            // You can only read this guy
            assert(I.IsRead() == true);

            return true;
        }

        if (I.IsVar("InputPad\\LogicalMap[]\\Map[]\\GadgetID")) {
            if (I.IsRead()) {
                //int Index = m_Map[iIndex1].GadgetID - INPUT_PS2_BTN_L2;
                I.SetVarEnum(GetNameFromGadgetID(i, m_Map[i][iIndex1].GadgetID));
            }

            return true;
        }

        {
            bool bButton = m_Map[i][iIndex1].bButton;
            bool Result = I.VarBool("InputPad\\LogicalMap[]\\Map[]\\IsButton", bButton);
            m_Map[i][iIndex1].bButton = bButton;
            if (Result) {
                // ensure that related properties are valid
                if (!m_Map[i][iIndex1].bButton) {
                    m_Map[i][iIndex1].bIsTap = false;
                    m_Map[i][iIndex1].bIsHold = false;
                }
                return true;
            }
        }

        {
            bool bIsTap = m_Map[i][iIndex1].bIsTap;
            bool Result = I.VarBool("InputPad\\LogicalMap[]\\Map[]\\IsTap", bIsTap);
            m_Map[i][iIndex1].bIsTap = bIsTap;
            if (Result) {
                // ensure that related properties are valid
                if (m_Map[i][iIndex1].bIsTap) {
                    m_Map[i][iIndex1].bIsHold = false;
                    m_Map[i][iIndex1].bButton = true;
                }
                return true;
            }
        }

        {
            bool bIsHold = m_Map[i][iIndex1].bIsHold;
            bool Result = I.VarBool("InputPad\\LogicalMap[]\\Map[]\\IsHold", bIsHold);
            m_Map[i][iIndex1].bIsHold = bIsHold;
            if (Result) {
                // ensure that related properties are valid
                if (m_Map[i][iIndex1].bIsHold) {
                    m_Map[i][iIndex1].bIsTap = false;
                    m_Map[i][iIndex1].bButton = true;
                }
                return true;
            }
        }

        if (I.VarFloat("InputPad\\LogicalMap[]\\Map[]\\Scale", m_Map[i][iIndex1].Scale)) {
            return true;
        }

        if (I.IsVar("InputPad\\LogicalMap[]\\Map[]\\InGame Context")) {
            if (I.IsRead()) {
                I.SetVarBool((m_Map[i][iIndex1].MapContext & INGAME_CONTEXT));
            } else {
                if (I.GetVarBool()) {
                    m_Map[i][iIndex1].MapContext |= INGAME_CONTEXT;
                } else {
                    m_Map[i][iIndex1].MapContext = (m_Map[i][iIndex1].MapContext & ~INGAME_CONTEXT);
                }
            }

            return true;
        }

        if (I.VarBool("InputPad\\LogicalMap[]\\Map[]\\Inventory Context", Boolean)) {
            if (I.IsRead()) {
                I.SetVarBool((m_Map[i][iIndex1].MapContext & INVENTORY_CONTEXT));
            } else {
                if (I.GetVarBool()) {
                    m_Map[i][iIndex1].MapContext |= INVENTORY_CONTEXT;
                } else {
                    m_Map[i][iIndex1].MapContext = (m_Map[i][iIndex1].MapContext & ~INVENTORY_CONTEXT);
                }
            }

            return true;
        }

        if (I.VarBool("InputPad\\LogicalMap[]\\Map[]\\Pause Context", Boolean)) {
            if (I.IsRead()) {
                I.SetVarBool((m_Map[i][iIndex1].MapContext & PAUSE_CONTEXT));
            } else {
                if (I.GetVarBool()) {
                    m_Map[i][iIndex1].MapContext |= PAUSE_CONTEXT;
                } else {
                    m_Map[i][iIndex1].MapContext = (m_Map[i][iIndex1].MapContext & ~PAUSE_CONTEXT);
                }
            }

            return true;
        }

        if (I.VarBool("InputPad\\LogicalMap[]\\Map[]\\FrontEnd Context", Boolean)) {
            if (I.IsRead()) {
                I.SetVarBool((m_Map[i][iIndex1].MapContext & FRONTEND_CONTEXT));
            } else {
                if (I.GetVarBool()) {
                    m_Map[i][iIndex1].MapContext |= FRONTEND_CONTEXT;
                } else {
                    m_Map[i][iIndex1].MapContext = (m_Map[i][iIndex1].MapContext & ~FRONTEND_CONTEXT);
                }
            }

            return true;
        }

        // TO_ADD_CONTEXT
        if (I.VarBool("InputPad\\LogicalMap[]\\Map[]\\Ladder Context", Boolean)) {
            if (I.IsRead()) {
                I.SetVarBool((m_Map[i][iIndex1].MapContext & LADDER_CONTEXT));
            } else {
                if (I.GetVarBool()) {
                    m_Map[i][iIndex1].MapContext |= LADDER_CONTEXT;
                } else {
                    m_Map[i][iIndex1].MapContext = (m_Map[i][iIndex1].MapContext & ~LADDER_CONTEXT);
                }
            }

            return true;
        }

        I.PopPath(iHeader);
    }

    return false;
}

void input_pad::SetAllLogical(logical* pLogical)
{
    assert(pLogical);
    int i;

    for (i = 0; i < MAX_LOGICAL; ++i) {
        m_Logical[i] = pLogical[i];
    }
}

//==============================================================================

void input_pad::SetAllMap(int iPlatform, map* pMap, int nMaps)
{
    assert(pMap);
    int i;
    for (i = 0; i < nMaps; ++i) {
        m_Map[iPlatform][i] = pMap[i];
    }
    m_nMaps[iPlatform] = nMaps;
}

const char* input_pad::GetNameFromGadgetID(int iPlatform, input_gadget GadgetID)
{
    /* IJB
    for (int i = 0; i < MAX_GADGET_MAPS; i++) {
        if (GadgetNameMap[iPlatform][i].GadgetID == GadgetID) {
            return (GadgetNameMap[iPlatform][i].pName);
        }
    }
*/
    return "";
}

input_gadget input_pad::GetGadgetIDFromName(int iPlatform, const char* pGadgetName)
{
    /* IJB
    for (int i = 0; i < MAX_GADGET_MAPS; i++) {
        if (strcmp(GadgetNameMap[iPlatform][i].pName, pGadgetName) == 0) {
            return (GadgetNameMap[iPlatform][i].GadgetID);
        }
    }
*/
    return INPUT_UNDEFINED;
}

//==============================================================================

input_mgr::input_mgr(void)
{
}

//==============================================================================

void input_mgr::RegisterPad(input_pad& Pad)
{
    Pad.m_pNext = s_pHead;
    s_pHead = &Pad;

    Pad.OnInitialize();
}

//==============================================================================

bool input_mgr::Update(float DeltaTime)
{
    //
    // First lets go throw all the queue.
    //
/* IJB
    while (input_UpdateState()) {
        if (input_IsPressed(INPUT_MSG_EXIT)) {
            return true;
        }
    };

    //
    // Now lets read our input.
    //

    for (input_pad* pPad = s_pHead; pPad != nullptr; pPad = pPad->m_pNext) {
        pPad->OnUpdate(iPlatform, DeltaTime);
    }
*/
    return false;
}

//==============================================================================
int input_mgr::WasPausePressed(void)
{
    for (input_pad* pPad = s_pHead; pPad != nullptr; pPad = pPad->m_pNext) {
        int ControllerID = pPad->m_ControllerID;

        if (ControllerID != -1) {
        }
    }
    return -1;
}
