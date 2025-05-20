#include "GamePad.h"
#include "../xfiles/x_plus.h"

ingame_pad g_IngamePad[ MAX_LOCAL_PLAYERS ];

ingame_pad::ingame_pad(  )
{
    m_pName = "Ingame Pad";
    g_InputMgr.RegisterPad( *this );
}

void ingame_pad::OnInitialize(  )
{
    //
    // Set all my logical actions
    //
    SetLogical( MOVE_STRAFE,            "Strafe" );
    SetLogical( MOVE_FOWARD_BACKWARDS,  "Fowards/Backwards" );
    SetLogical( LOOK_HORIZONTAL,        "Horizontral Look" );
    SetLogical( LOOK_VERTICAL,          "Vertical Look" );
    SetLogical( LEAN_LEFT,              "Lean Left" );
    SetLogical( LEAN_RIGHT,             "Lean Right" );

    SetLogical( ACTION_RELOAD,          "Reload" );
    SetLogical( ACTION_PRIMARY,         "Primary" );
    SetLogical( ACTION_SECONDARY,       "Secondary" );
    SetLogical( ACTION_JUMP,            "Jump" );
    SetLogical( ACTION_CROUCH,          "Crouch" );
    SetLogical( ACTION_MUTATION,        "Toggle Mutation");
    SetLogical( ACTION_FIRE_PARASITES,  "Fire Parasites" );
    SetLogical( ACTION_FIRE_CONTAGION,  "Fire Contagion" );
    SetLogical( ACTION_MUTANT_MELEE,    "Mutant Melee" );
    SetLogical( ACTION_CYCLE_RIGHT,     "Cycle Weapons Right" );
    SetLogical( ACTION_CYCLE_LEFT,      "Cycle Weapons Left" );
    SetLogical( ACTION_USE,             "Use Object");
    SetLogical( ACTION_FLASHLIGHT,      "Toggle Flashlight");
    SetLogical( ACTION_TOGGLE_PRECISE_AIM, "Toggle Precise Aim");
    
    // Conversation mappings.
    SetLogical( ACTION_SPEAK_FOLLOW_STAY,   "Speak: Follow Me" );
    SetLogical( ACTION_SPEAK_USE_ACTIVATE,  "Speak: Use / Activate" );
    SetLogical( ACTION_SPEAK_COVER_ME,      "Speak: Cover Me" );
    SetLogical( ACTION_SPEAK_ATTACK_COVER,  "Speak: Attack / Take Cover" );


    SetLogical( ACTION_HUD_CONTEXT,         "Hud Context switch" );
    SetLogical( ACTION_PAUSE_CONTEXT,       "Pause menu context switch" );

    SetLogical( ACTION_HUD_MOVEMENT_HORIZONTAL, "Move selection cursor horizontal" );
    SetLogical( ACTION_HUD_MOVEMENT_VERTICAL,   "Move selection cursor vertical" );
    SetLogical( ACTION_HUD_SET_HOTKEY_0,        "Assign the item as hot key 0" );
    SetLogical( ACTION_HUD_SET_HOTKEY_1,        "Assign the item as hot key 1" );
    SetLogical( ACTION_USE_HOTKEY_0,            "Use the item in hot key 0" );
    SetLogical( ACTION_USE_HOTKEY_1,            "Use the item in hot key 1" );
    SetLogical( ACTION_WEAPON_ITEM_SWITCH,      "Toggle between item and weapons" );
    SetLogical( ACTION_THROW_GRENADE,           "Throw a grenade" );
    SetLogical( ACTION_CYCLE_GRENADE_TYPE,      "Cycle grenade type" );
    SetLogical( ACTION_MELEE_ATTACK,            "Melee attack" );


    SetLogical( ACTION_VOTE_MENU_ON,            "Vote: Menu On" );
    SetLogical( ACTION_VOTE_MENU_OFF,           "Vote: Menu Off" );
    SetLogical( ACTION_VOTE_YES,                "Vote: Yes" );
    SetLogical( ACTION_VOTE_NO,                 "Vote: No" );
    SetLogical( ACTION_VOTE_ABSTAIN,            "Vote: Abstain" );

    SetLogical( ACTION_CHAT,                    "Voice Chat" );
    SetLogical( ACTION_TALK_MODE_TOGGLE,        "Talk: Mode Toggle" );

    SetLogical( ACTION_MP_FLASHLIGHT,           "Multiplayer Toggle Flashlight" );
    SetLogical( ACTION_MP_MUTATE,               "Multiplayer Toggle Mutation" );
    SetLogical( ACTION_DROP_FLAG,               "Drop Flag" );
}

void ingame_pad::OnUpdate( float DeltaTime )
{
    input_pad::OnUpdate(DeltaTime);
}

const char* ingame_pad::GetLogicalIDName( int Index )
{
    // Which state?
    switch( Index )
    {
        default:
            assert(false); //ASSERTS(0, "Add your new state to this list or properties will not work!");


        case MOVE_STRAFE:               return "Strafe";
        case MOVE_FOWARD_BACKWARDS:     return "Move";
        case LOOK_HORIZONTAL:           return "Look Horiz";
        case LOOK_VERTICAL:             return "Look Vert";
        case LEAN_LEFT:                 return "Lean Left";
        case LEAN_RIGHT:                return "Lean Right";
        case ACTION_JUMP:               return "Jump";
        case ACTION_CROUCH:             return "Crouch";
        case ACTION_PRIMARY:            return "Primary Fire";
        case ACTION_SECONDARY:          return "Secondary Fire";
        case ACTION_RELOAD:             return "Reload";
        case ACTION_MUTATION:           return "Mutation";
        case ACTION_FIRE_PARASITES:     return "Fire Parasites";
        case ACTION_FIRE_CONTAGION:     return "Fire Contagion";
        case ACTION_MUTANT_MELEE:       return "Mutant Melee";
        case ACTION_CYCLE_RIGHT:        return "Cycle Right";
        case ACTION_CYCLE_LEFT:         return "Cycle Left";
        case ACTION_USE:                return "Use";
        case ACTION_FLASHLIGHT:         return "Flashlight";
        case ACTION_TOGGLE_PRECISE_AIM: return "Toggle Precise Aim";
        case ACTION_MP_FLASHLIGHT:      return "Multiplayer Flashlight";
        case ACTION_MP_MUTATE:          return "Multiplayer Mutation";
        case ACTION_DROP_FLAG:          return "Drop Flag";


        // Friendly interaction controls.
        case ACTION_SPEAK_FOLLOW_STAY:  return "Talk: Follow";
        case ACTION_SPEAK_USE_ACTIVATE: return "Talk: Activate";
        case ACTION_SPEAK_COVER_ME:     return "Talk: Cover";
        case ACTION_SPEAK_ATTACK_COVER: return "Talk: Attack";

        //Interface controls
        case ACTION_HUD_CONTEXT:            return "";
        case ACTION_HUD_MOVEMENT_HORIZONTAL:return "";
        case ACTION_HUD_MOVEMENT_VERTICAL:  return "";
        case ACTION_HUD_SET_HOTKEY_0:       return "Set Hkey 0";
        case ACTION_HUD_SET_HOTKEY_1:       return "Set Hkey 1";

        // Hot key stuff
        case ACTION_USE_HOTKEY_0:           return "Use Hkey 0";
        case ACTION_USE_HOTKEY_1:           return "Use Hkey 1";
        case ACTION_PAUSE_CONTEXT:          return "Pause";
        case ACTION_FRONTEND_CONTEXT:       return "";
        case ACTION_RIFT:                   return "";

        case ACTION_WEAPON_ITEM_SWITCH:     return "";
        case ACTION_THROW_GRENADE:          return "";
        case ACTION_CYCLE_GRENADE_TYPE:     return "";
        case ACTION_MELEE_ATTACK:           return "";

        case ACTION_TALK_MODE_TOGGLE:       return "Talk: Mode Toggle";
        case ACTION_VOTE_MENU_ON:           return "Vote: Menu On";
        case ACTION_VOTE_MENU_OFF:          return "Vote: Menu Off";
        case ACTION_VOTE_YES:               return "Vote: Yes";
        case ACTION_VOTE_NO:                return "Vote: No";
        case ACTION_VOTE_ABSTAIN:           return "Vote: Abstain";

        case ACTION_CHAT:                   return "Voice Chat";

    }
}

//===========================================================================

const char* ingame_pad::GetLogicalIDEnum( void )
{
    // Build enum list
    static char s_Enum[1024] = {0};
        
    // Already built?
    if (s_Enum[0])
        return s_Enum;

    // Add all states to enum
    char* pDest = s_Enum;
    for (int i = 0; i < MAX_ACTION; i++)
    {
        // Lookup state name
        const char* pState = GetLogicalIDName(i);

        // Add to enum list
        strcpy(pDest, pState);

        // Next
        pDest += strlen(pState)+1;
    }

    // Make sure we didn't overrun the array!
    assert(pDest <= &s_Enum[1024]);

    // Done
    return s_Enum;
}

//=========================================================================

ingame_pad::logical_id ingame_pad::GetLogicalIDByName( const char* pName )
{
    // Check all states
    for (int i = 0; i < MAX_ACTION; i++)
    {
        // Found?
        if (x_stricmp(pName, GetLogicalIDName(i)) == 0)
            return (ingame_pad::logical_id)i;
    }

    // Not found
    return ACTION_NULL;
}