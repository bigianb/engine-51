#include "HudObject.h"
#include <iostream>

int   HudObject::m_PulseAlpha;
float HudObject::m_PulseRate;

class hud_object_desc : public object_desc
{
public:
    hud_object_desc()
        : object_desc(
              Object::TYPE_HUD_OBJECT,
              "Hud Object",
              "HUD",

              Object::ATTR_DRAW_2D |
                  Object::ATTR_NEEDS_LOGIC_TIME |
                  Object::ATTR_RENDERABLE,

              FLAGS_GENERIC_EDITOR_CREATE |
                  FLAGS_IS_DYNAMIC)
    {
    }

    Object* Create(ObjectManager* om, collision_mgr* cm)
    {
        return new HudObject(om);
    }
};

static hud_object_desc s_HudObject_Desc;

const object_desc& HudObject::GetObjectType()
{
    return s_HudObject_Desc;
}

const object_desc& HudObject::GetTypeDesc() const
{
    return s_HudObject_Desc;
}

HudObject::HudObject(ObjectManager* om)
    : Object(om)
{
    std::cout << "Initializing HUD" << std::endl;

    m_LogicRunning = false;
    m_Initialized = false;
    m_PulseAlpha = 255;
    m_PulseRate = 512.0f;
    m_NumHuds = 0;

    m_FPSCount15 = 0;
    m_FPSCount20 = 0;
    m_FPSCount30 = 0;
    m_Below30ImageCount = 0;

    m_bLetterBoxOn = false;
    m_LetterBoxCurrTime = 1.0f;
    m_LetterBoxTotalTime = 1.0f;

    // m_ViewDimensions.Clear();

    /*
        for(int i = 0; i < NET_MAX_PER_CLIENT; i++ )
        {
            // Initialize component pointer array.
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_MUTANT_VISION     ]  = &m_PlayerHuds[ i ].m_MutantVision;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_CONTAGIOUS_VISION ]  = &m_PlayerHuds[ i ].m_ContagiousVision;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_HEALTH_BAR        ]  = &m_PlayerHuds[ i ].m_Health;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_AMMO_BAR          ]  = &m_PlayerHuds[ i ].m_Ammo;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_RETICLE           ]  = &m_PlayerHuds[ i ].m_Reticle;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_DAMAGE            ]  = &m_PlayerHuds[ i ].m_Damage;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_SNIPER            ]  = &m_PlayerHuds[ i ].m_Sniper;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_ICON              ]  = &m_PlayerHuds[ i ].m_Icon;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_INFO_BOX          ]  = &m_PlayerHuds[ i ].m_InfoBox;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_TEXT_BOX          ]  = &m_PlayerHuds[ i ].m_Text;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_VOTE              ]  = &m_PlayerHuds[ i ].m_Vote;
            m_PlayerHuds[ i ].m_HudComponents[ HUD_ELEMENT_SCANNER           ]  = &m_PlayerHuds[ i ].m_Scanner;

            m_PlayerHuds[ i ].m_Active = false;

            // Initialize element pulse.
            for( s32 j = 0; j < HUD_ELEMENT_NUM_ELEMENTS; j++ )
            {
                m_PlayerHuds[ i ].m_HudComponents[ j ]->m_bPulsing = false;
            }
        }
    */
    // Timer
    m_TimerTriggerGuid = 0;
    m_RenderTimer = false;
    m_TimerActive = false;
    m_TimerTime = 30.0f;
    m_TimerAdd = 0.0f;
    m_TimerSub = 0.0f;
    m_TimerWarning = false;
    m_TimerCritical = false;
    m_TimerCriticalStarted = false;
    m_TimerWarningStarted = false;

    // Objective Strings
    m_ObjectiveTableNameIndex = -1;
    m_ObjectiveTitleStringIndex = -1;
    m_ObjectiveTime = 0.0f;
}

HudObject::~HudObject()
{
}

void HudObject::OnEnumProp(prop_enum& List)
{
    Object::OnEnumProp(List);

    List.PropEnumHeader("Hud", "Stats for this item", PROP_TYPE_HEADER);

    // Enum all components.
    /*
    for (int j = 0; j < HUD_ELEMENT_NUM_ELEMENTS; j++) {
          m_PlayerHuds[ 0 ].m_HudComponents[ j ]->OnEnumProp( List );
    }
*/
    //----------------------------------------------------------------------
    // Binary text resource.
    //----------------------------------------------------------------------
    List.PropEnumHeader("Hud\\Binary Text Resource", "The binary text resource loader.", 0);
    /*
    for (int i = 0; i < MAX_BIN_TXT_RSC; i++) {
        List.PropEnumExternal(xfs("Hud\\Binary Text Resource\\Binary Text Rsc [%d]", i),
                              "Resource\0stringbin\0",
                              "Loads the binary text resource which is used "
                              "for localizing all of our text resources",
                              0);
    }
*/
    //----------------------------------------------------------------------
    // Timer
    //----------------------------------------------------------------------
    List.PropEnumHeader("Hud\\Timer", "Properties for setting up and useing a on screen timer", 0);
    List.PropEnumGuid("Hud\\Timer\\Trigger Guid", "Guid that is activated when the timer hits Zero.", PROP_TYPE_EXPOSE | PROP_TYPE_MUST_ENUM);
    List.PropEnumFloat("Hud\\Timer\\Time", "Time in seconds", PROP_TYPE_EXPOSE);
    List.PropEnumBool("Hud\\Timer\\Render Timer", "Render the Timer", PROP_TYPE_EXPOSE);
    List.PropEnumBool("Hud\\Timer\\Start Timer", "true = Start, False = Stop", PROP_TYPE_EXPOSE);
    List.PropEnumFloat("Hud\\Timer\\Add Time", "Add Time", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SHOW);
    List.PropEnumFloat("Hud\\Timer\\Sub Time", "Sub Time", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SHOW);
    List.PropEnumBool("Hud\\Timer\\Timer In Warning State", "Set Timer to a Warning State. Turn Timer Text RED", PROP_TYPE_EXPOSE);
    List.PropEnumBool("Hud\\Timer\\Timer In Critical State", "Set Timer to a Warning State. Flash Timer Text and render it RED", PROP_TYPE_EXPOSE);
}

bool HudObject::OnProperty(prop_query& rPropQuery)
{
    //----------------------------------------------------------------------
    // Base Object.
    //----------------------------------------------------------------------
    if (Object::OnProperty(rPropQuery)) {
        return true;
    }

    // Cycle through all elements.
    /*
    for( int i = 0; i < HUD_ELEMENT_NUM_ELEMENTS; i++ )
    {
        if( m_PlayerHuds[ 0 ].m_HudComponents[ i ]->OnProperty( rPropQuery ) )
        {
            return true;
        }
    }
        */

    //----------------------------------------------------------------------
    // Binary text resource.
    //----------------------------------------------------------------------
    if (rPropQuery.IsVar("Hud\\Binary Text Resource\\Binary Text Rsc []")) {
        /*
        if( rPropQuery.IsRead() )
        {
            rPropQuery.SetVarExternal( m_hBinaryTextRsc[ rPropQuery.GetIndex(0) ].GetName(), RESOURCE_NAME_SIZE );
        }
        else
        {
            const char* pStr = rPropQuery.GetVarExternal();

            if( strlen( pStr ) )
            {
                xstring FileName( pStr );

                if( FileName.Find( '.' ) == -1 )
                    FileName += ".bin";

                m_hBinaryTextRsc[ rPropQuery.GetIndex(0) ].SetName( (const char*)FileName );

                // Load the binary text resource.
                if( m_hBinaryTextRsc[ rPropQuery.GetIndex(0) ].IsLoaded() == false )
                    m_hBinaryTextRsc[ rPropQuery.GetIndex(0) ].GetPointer();
            }
        }
*/
        return true;
    }

    //----------------------------------------------------------------------
    // Timer
    //----------------------------------------------------------------------
    if (rPropQuery.VarGUID("Hud\\Timer\\Trigger Guid", m_TimerTriggerGuid)) {
        return true;
    }

    if (rPropQuery.VarFloat("Hud\\Timer\\Time", m_TimerTime)) {
        return true;
    }

    if (rPropQuery.VarBool("Hud\\Timer\\Render Timer", m_RenderTimer)) {
        return true;
    }

    if (rPropQuery.VarBool("Hud\\Timer\\Start Timer", m_TimerActive)) {
        return true;
    }

    if (rPropQuery.VarFloat("Hud\\Timer\\Add Time", m_TimerAdd)) {
        return true;
    }

    if (rPropQuery.VarFloat("Hud\\Timer\\Sub Time", m_TimerSub)) {
        return true;
    }

    if (rPropQuery.VarBool("Hud\\Timer\\Timer In Warning State", m_TimerWarning)) {
        return true;
    }
    if (rPropQuery.VarBool("Hud\\Timer\\Timer In Critical State", m_TimerCritical)) {
        return true;
    }

    return false;
}

void HudObject::OnAdvanceLogic(float DeltaTime)
{
    // Currently, the HUD needs to initialize before it can do the logic,
    // and it needs to attempt to do the logic before it can initialize.
    // This is due to the orders in which the game and the editor create the
    // objects. It should only take a couple of frames to work itself out.
    m_LogicRunning = true;
    if (!m_Initialized) {
        return;
    }

    // Timer
    if (m_TimerWarning && m_TimerWarningStarted == false) {
        //g_AudioMgr.Play("Objective_Timer_Reset", true );
        m_TimerWarningStarted = true;
    } else if (m_TimerWarning == false) {
        m_TimerWarningStarted = false;
    }

    if (m_TimerCritical && m_TimerCriticalStarted == false) {
        // g_AudioMgr.Play("Objective_Timer_Reset", true );
        m_TimerCriticalStarted = true;
    } else if (m_TimerCritical == false) {
        m_TimerCriticalStarted = false;
    }

    if (m_TimerAdd > 0.0f) {
        m_TimerTime += m_TimerAdd;
        m_TimerAdd = 0.0f;
    }

    if (m_TimerSub < 0.0f) {
        m_TimerTime -= m_TimerSub;
        m_TimerSub = 0.0f;
    }

    if (m_TimerActive) {
        if (m_TimerTime <= 0.0f) {
            // Trigger Guid
            Object* pObject = objectManager->GetObjectByGuid(m_TimerTriggerGuid);
            if (pObject) {
                pObject->OnActivate(true);
            }

            // Time is at zero stop the clock.
            m_TimerActive = false;
        }
        m_TimerTime -= DeltaTime;
    }

    // Objective Text update
    if (m_ObjectiveTime > 0.0f) {
        m_ObjectiveTime -= DeltaTime;
    }

    // Update pulsing elements.
    m_PulseAlpha += (int)(m_PulseRate * DeltaTime);

    if (m_PulseAlpha > 255) {
        m_PulseAlpha = 255;
        m_PulseRate = -m_PulseRate;
    }

    if (m_PulseAlpha < 64) {
        m_PulseAlpha = 64;
        m_PulseRate = -m_PulseRate;
    }

    // Do logic for all components.
    for (int i = 0; i < m_NumHuds; i++) {
        //     m_PlayerHuds[ i ].OnAdvanceLogic( DeltaTime );
    }

    // Update the mutant vision effect manually. It is a special case because it uses
    // a single static effect for all the player huds.
    // We also do the contagious vision at the same time.
    // hud_mutant_vision::UpdateEffects( DeltaTime );
    // hud_contagious_vision::UpdateEffects( DeltaTime );

    // Do the widescreen bars.
    // m_LetterBoxCurrTime = MINMAX(0.0f , m_LetterBoxCurrTime + DeltaTime, m_LetterBoxTotalTime );
}

void HudObject::OnRender(void)
{

    if (m_LogicRunning == false) {
        return;
    }

    // Try to initialize the hud every frame until it works.
    if (!m_Initialized) {
        InitHud();

        // Check and see if it worked, if not, come back later.
        if (!m_Initialized) {
            return;
        }
    }

    /*
    // Draw the cinematic bars?
    if ((g_first_person) && (IsLetterBoxOn())) {
        RenderLetterBox(m_ViewDimensions, GetLetterBoxAmount());
        ((hud_renderable*)(GetPlayerHud(g_RenderContext.LocalPlayerIndex).m_HudComponents[HUD_ELEMENT_TEXT_BOX]))->OnRender(SMP_UTIL_GetActivePlayer());
    } else {
        if (g_StateMgr.GetState() != SM_PLAYING_GAME) {
            return;
        }

        GetPlayerHud(g_RenderContext.LocalPlayerIndex).OnRender();

        // Timer
        if (m_RenderTimer) {
            RenderTimer();
        }
    }

    // Clear any icons that might have accumulated this frame.
    ((hud_icon*)(GetPlayerHud(g_RenderContext.LocalPlayerIndex).m_HudComponents[HUD_ELEMENT_ICON]))->m_NumActiveIcons = 0;
*/
}

void HudObject::InitHud()
{
    m_NumHuds = 1;
    /*
        int i = 0;
        slot_id PlayerSlot = g_ObjMgr.GetFirst( Object::TYPE_PLAYER );

        // If a player hasn't been created yet, abort.
        if( PlayerSlot == SLOT_NULL )
            return;

        while( PlayerSlot != SLOT_NULL )
        {
            assert( i < m_NumHuds );

            // Get the players view port.
            player* pPlayer = (player*)g_ObjMgr.GetObjectBySlot( PlayerSlot );

            if(
                (pPlayer != NULL)                     && // Is the player valid?
                (pPlayer->GetLocalSlot() != -1) )         // Is the player local?
            {
                player_hud& PlayerHud   = m_PlayerHuds[ i ];
                PlayerHud.m_PlayerSlot  = PlayerSlot;
                PlayerHud.m_LocalSlot   = pPlayer->GetLocalSlot();

                PlayerHud.m_NetSlot     = pPlayer->net_GetSlot();
                PlayerHud.m_Active      = TRUE;

                // Find out what portion of the screen the player owns.
                //rect m_ViewDimensions;
                view& rView = pPlayer->GetView();
                rView.GetViewport( m_ViewDimensions );

                // Set Hud dimensions!
                switch( m_NumHuds )
                {
                    // One player.
                    case 1:
                        PlayerHud.m_XPos    = LEFTMARGIN    + 2.0f;
                        PlayerHud.m_YPos    = TOPMARGIN     + 2.0f;
                        PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  - LEFTMARGIN - RIGHTMARGIN - 4.0f;
                        PlayerHud.m_Height  = m_ViewDimensions.GetHeight() - TOPMARGIN  - BOTTOMMARGIN - 4.0f;
                        break;


                    // Two players.
                    case 2:
                        switch( PlayerHud.m_LocalSlot )
                        {
                            // Top Player.
                            case 0:
                                PlayerHud.m_XPos    = m_ViewDimensions.Min.X + LEFTMARGIN    + 2.0f;
                                PlayerHud.m_YPos    = m_ViewDimensions.Min.Y + TOPMARGIN     + 2.0f;
                                PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  - LEFTMARGIN - RIGHTMARGIN - 4.0f;
                                PlayerHud.m_Height  = m_ViewDimensions.GetHeight() - TOPMARGIN  - 4.0f;
                                break;

                            // Bottom Player.
                            case 1:
                                PlayerHud.m_XPos    = m_ViewDimensions.Min.X + LEFTMARGIN    + 2.0f;
                                PlayerHud.m_YPos    = m_ViewDimensions.Min.Y + 2.0f;
                                PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  - LEFTMARGIN - RIGHTMARGIN - 4.0f;
                                PlayerHud.m_Height  = m_ViewDimensions.GetHeight() - BOTTOMMARGIN - 4.0f;
                                break;

                            default:
                                break;
                        }
                        break;

                    // Three/Four players.
                    case 3:
                    {
                        switch( PlayerHud.m_LocalSlot )
                        {
                            // Top-left player.
                        case 0:
                            PlayerHud.m_XPos    = m_ViewDimensions.Min.X       + LEFTMARGIN + 2.0f;
                            PlayerHud.m_YPos    = m_ViewDimensions.Min.Y       +  TOPMARGIN + 2.0f;
                            PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  - LEFTMARGIN - 4.0f;
                            PlayerHud.m_Height  = m_ViewDimensions.GetHeight() -  TOPMARGIN - 4.0f;
                            break;

                            // Top-right player.
                        case 1:
                            PlayerHud.m_XPos    = m_ViewDimensions.Min.X                      + 2.0f;
                            PlayerHud.m_YPos    = m_ViewDimensions.Min.Y       +    TOPMARGIN + 2.0f;
                            PlayerHud.m_Width   = m_ViewDimensions.GetWidth () -  RIGHTMARGIN - 4.0f;
                            PlayerHud.m_Height  = m_ViewDimensions.GetHeight() -    TOPMARGIN - 4.0f;
                            break;

                            // Bottom player.
                        case 2:
                            PlayerHud.m_XPos    = m_ViewDimensions.Min.X + LEFTMARGIN    + 2.0f;
                            PlayerHud.m_YPos    = m_ViewDimensions.Min.Y + 2.0f;
                            PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  - LEFTMARGIN - RIGHTMARGIN - 4.0f;
                            PlayerHud.m_Height  = m_ViewDimensions.GetHeight() - BOTTOMMARGIN - 4.0f;
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                    case 4:
                    {
                        //f32 X    = m_ViewDimensions.Min.X;
                        //f32 Y    = m_ViewDimensions.Min.Y;
                        //f32 MidX = m_ViewDimensions.Min.X + ((m_ViewDimensions.Min.X + m_ViewDimensions.Max.X)/2.0f);
                        //f32 MidY = m_ViewDimensions.Min.Y + ((m_ViewDimensions.Min.Y + m_ViewDimensions.Max.Y)/2.0f);

                        switch( PlayerHud.m_LocalSlot )
                        {
                            // Top-left player.
                            case 0:
                                PlayerHud.m_XPos    = m_ViewDimensions.Min.X       + LEFTMARGIN + 2.0f;
                                PlayerHud.m_YPos    = m_ViewDimensions.Min.Y       +  TOPMARGIN + 2.0f;
                                PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  - LEFTMARGIN - 4.0f;
                                PlayerHud.m_Height  = m_ViewDimensions.GetHeight() -  TOPMARGIN - 4.0f;
                                break;

                            // Top-right player.
                            case 1:
                                PlayerHud.m_XPos    = m_ViewDimensions.Min.X                      + 2.0f;
                                PlayerHud.m_YPos    = m_ViewDimensions.Min.Y       +    TOPMARGIN + 2.0f;
                                PlayerHud.m_Width   = m_ViewDimensions.GetWidth () -  RIGHTMARGIN - 4.0f;
                                PlayerHud.m_Height  = m_ViewDimensions.GetHeight() -    TOPMARGIN - 4.0f;
                                break;

                            // Bottom-left player.
                            case 2:
                                PlayerHud.m_XPos    = m_ViewDimensions.Min.X       +   LEFTMARGIN + 2.0f;
                                PlayerHud.m_YPos    = m_ViewDimensions.Min.Y                      + 2.0f;
                                PlayerHud.m_Width   = m_ViewDimensions.GetWidth()  -   LEFTMARGIN - 4.0f;
                                PlayerHud.m_Height  = m_ViewDimensions.GetHeight() - BOTTOMMARGIN - 4.0f;
                                break;

                            // Bottom-right player (or not).
                            case 3:
                                PlayerHud.m_XPos    = m_ViewDimensions.Min.X                      + 2.0f;
                                PlayerHud.m_YPos    = m_ViewDimensions.Min.Y                      + 2.0f;
                                PlayerHud.m_Width   = m_ViewDimensions.GetWidth () -  RIGHTMARGIN - 4.0f;
                                PlayerHud.m_Height  = m_ViewDimensions.GetHeight() - BOTTOMMARGIN - 4.0f;
                                break;

                            default:
                                break;
                        }
                        break;
                    }

                    default:
                        ASSERT( FALSE );
                        break;
                }

                PlayerHud.m_CenterX = ((f32)m_ViewDimensions.Min.X+(m_ViewDimensions.GetWidth())/2.0f);
                PlayerHud.m_CenterY = ((f32)m_ViewDimensions.Min.Y+(m_ViewDimensions.GetHeight())/2.0f);

                // Ok, now lets initialize the hud elements.
                {
                    // Centered.
                    PlayerHud.m_Reticle.m_XPos  = PlayerHud.m_CenterX;
                    PlayerHud.m_Reticle.m_YPos  = PlayerHud.m_CenterY;

                    PlayerHud.m_Damage.m_XPos   = PlayerHud.m_CenterX;
                    PlayerHud.m_Damage.m_YPos   = PlayerHud.m_CenterY;

                    PlayerHud.m_Icon.m_XPos     = PlayerHud.m_CenterX;
                    PlayerHud.m_Icon.m_YPos     = PlayerHud.m_CenterY;

                    // Top left.
                    PlayerHud.m_Text.m_XPos     = PlayerHud.m_XPos;
                    PlayerHud.m_Text.m_YPos     = PlayerHud.m_YPos;

                    // Bottom left.
                    PlayerHud.m_Health.m_XPos   = PlayerHud.m_XPos;
                    PlayerHud.m_Health.m_YPos   = PlayerHud.m_Height + PlayerHud.m_YPos;

                    PlayerHud.m_Vote.m_XPos     = 210;
                    PlayerHud.m_Vote.m_YPos     = 332;

                    // Bottom right.
                    PlayerHud.m_Ammo.m_XPos     = PlayerHud.m_XPos + PlayerHud.m_Width;
                    PlayerHud.m_Ammo.m_YPos     = PlayerHud.m_YPos + PlayerHud.m_Height;

                    PlayerHud.m_Scanner.m_XPos  = PlayerHud.m_XPos + PlayerHud.m_Width;
                    PlayerHud.m_Scanner.m_YPos  = PlayerHud.m_YPos + PlayerHud.m_Height;

                    // Top right.
                    PlayerHud.m_InfoBox.m_XPos  = PlayerHud.m_XPos + PlayerHud.m_Width;
                    PlayerHud.m_InfoBox.m_YPos  = PlayerHud.m_YPos;

                    // Sniper
                    PlayerHud.m_Sniper.m_ViewDimensions = m_ViewDimensions;

                    PlayerHud.m_Text.SetMaxWidth( s32(PlayerHud.m_Width) - 4 );
                }


                i++;
            }

            // Get the next player.
            PlayerSlot = g_ObjMgr.GetNext( PlayerSlot ) ;
        }
    */
    m_Initialized = true;
}

BBox HudObject::GetLocalBBox() const
{
    return BBox(GetPosition(), 50.0f);
}

void HudObject::SetupLetterBox(bool On, float SlideTime)
{
    // We don't want any nasty division by zero bugs!
    if (SlideTime == 0.0f) {
        m_LetterBoxCurrTime = 1.0f;
        m_LetterBoxTotalTime = 1.0f;
    }

    float CurrPercentage = m_LetterBoxCurrTime / m_LetterBoxTotalTime;

    // If its on already, just change the effective speed of sliding.
    if (On == m_bLetterBoxOn) {
        m_LetterBoxCurrTime = SlideTime * CurrPercentage;
        m_LetterBoxTotalTime = SlideTime;
    }

    // Looks like we have to reverse its direction too.
    else {
        CurrPercentage = 1.0f - CurrPercentage;
    }

    m_LetterBoxCurrTime = SlideTime * CurrPercentage;
    m_LetterBoxTotalTime = SlideTime;
    m_bLetterBoxOn = On;
}
