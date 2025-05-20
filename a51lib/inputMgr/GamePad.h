#pragma once

#include "InputMgr.h"

#define MAX_LOCAL_PLAYERS 1

class ingame_pad : public input_pad
{
public:

    enum logical_id
    {
        ACTION_NULL = -1,
        MOVE_STRAFE,                    
        MOVE_FOWARD_BACKWARDS,          
        LOOK_HORIZONTAL,                
        LOOK_VERTICAL,                  
        ACTION_JUMP,                    
        ACTION_CROUCH,                  
        ACTION_PRIMARY,                 
        ACTION_SECONDARY,               
        ACTION_RELOAD,                  
		ACTION_MUTATION,
		ACTION_CYCLE_RIGHT,             
        ACTION_USE,                     
        ACTION_FLASHLIGHT,              

        // Friendly interaction controls.
        ACTION_SPEAK_FOLLOW_STAY,       
        ACTION_SPEAK_USE_ACTIVATE,      
        ACTION_SPEAK_COVER_ME,          
        ACTION_SPEAK_ATTACK_COVER,      

        //Interface controls
        ACTION_HUD_CONTEXT,             
        ACTION_HUD_MOVEMENT_HORIZONTAL, 
        ACTION_HUD_MOVEMENT_VERTICAL,   
        ACTION_HUD_SET_HOTKEY_0,        
        ACTION_HUD_SET_HOTKEY_1,        
        // Hot key stuff
        ACTION_USE_HOTKEY_0,            
        ACTION_USE_HOTKEY_1,            

        ACTION_PAUSE_CONTEXT,           

        ACTION_FRONTEND_CONTEXT,        

        ACTION_RIFT,                    

        ACTION_WEAPON_ITEM_SWITCH,      
        
        ACTION_THROW_GRENADE,           
        ACTION_CYCLE_GRENADE_TYPE,      

        ACTION_MELEE_ATTACK,            
        ACTION_CYCLE_LEFT,              
        ACTION_TOGGLE_PRECISE_AIM,      

        ACTION_VOTE_MENU_ON,
        ACTION_VOTE_MENU_OFF,
        ACTION_VOTE_YES,
        ACTION_VOTE_NO,
        ACTION_VOTE_ABSTAIN,

        ACTION_CHAT,

        LEAN_LEFT,
        LEAN_RIGHT,

        ACTION_TALK_MODE_TOGGLE,
        ACTION_FIRE_PARASITES,
        ACTION_FIRE_CONTAGION,
        ACTION_MUTANT_MELEE,

        ACTION_MP_FLASHLIGHT,
        ACTION_MP_MUTATE,
        ACTION_DROP_FLAG,

        MAX_ACTION                      
    };

public:

                        ingame_pad      (  );
    virtual void        OnUpdate        ( float DeltaTime );
    virtual void        OnInitialize    (  );

    // These routines allow us to expose properties to the editor.
    static  const char* GetLogicalIDName    ( int Index ) ;            
    static  const char* GetLogicalIDEnum    ( )  ;
    static  logical_id GetLogicalIDByName  ( const char* pName ) ;
};

extern ingame_pad g_IngamePad[ MAX_LOCAL_PLAYERS ];
