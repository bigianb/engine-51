#pragma once

#include "Object.h"
#include "../objectManager/ObjectManager.h"
#include "../VectorMath.h"

#define MAX_BIN_TXT_RSC 3

class HudObject : public Object
{
public:
    CREATE_RTTI(HudObject, Object, Object)

    HudObject(ObjectManager* om);
    virtual ~HudObject();
    int  GetMaterial() const override { return MAT_TYPE_NULL; }
    void OnRender() override;
    void OnAdvanceLogic(float DeltaTime) override;
    BBox GetLocalBBox() const override;

    void OnEnumProp(prop_enum& list) override;
    bool OnProperty(prop_query& rPropQuery) override;

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

    //void ResetFrameRateInfo();
    //void RenderFrameRateInfo();
    void RenderTimer();

    void GetBinaryResourceName(std::string& String);
    void SetElementPulseState(int ElementID, bool DoPulse);
    void InitHud();
    //     player_hud&     GetPlayerHud            ( int LocalSlot );

    void  SetupLetterBox(bool On, float ScrollTime = 0.0f);
    bool  IsLetterBoxOn() const;
    float GetLetterBoxAmount() const;
    //static  void            RenderLetterBox         ( const Rect& VP, float Percent );

    void SetObjectiveText(int TableNameIndex, int TitleStringIndex);
    void RenderObjectiveText();

    int GetNumPlayers() { return m_NumHuds; };

    // player_hud m_PlayerHuds[ NET_MAX_PER_CLIENT ];

    static int   m_PulseAlpha;
    static float m_PulseRate;
    bool         m_Initialized;

protected:
    //       rhandle<char>               m_hBinaryTextRsc[ MAX_BIN_TXT_RSC ];

    //        Rect                        m_ViewDimensions;

    bool m_LogicRunning;

    int m_NumHuds;

    int m_FPSCount15;
    int m_FPSCount20;
    int m_FPSCount30;
    int m_Below30ImageCount;

    bool  m_bLetterBoxOn;
    float m_LetterBoxTotalTime;
    float m_LetterBoxCurrTime;

    guid  m_TimerTriggerGuid;
    bool  m_RenderTimer;
    bool  m_TimerActive;
    float m_TimerTime;
    float m_TimerAdd;
    float m_TimerSub;
    bool  m_TimerWarning;
    bool  m_TimerCritical;
    bool  m_TimerCriticalStarted;
    bool  m_TimerWarningStarted;

    int   m_ObjectiveTableNameIndex;
    int   m_ObjectiveTitleStringIndex;
    float m_ObjectiveTime;
};

//=========================================================================

inline void RenderLine(const wchar_t* pLine, IntRect& iRect, uint8_t Alpha, Colour& TextColor, int FontNum, int Flags, bool bShadow = false, float FlareAmount = 0.0f)
{
    /*
        if( pLine[ 0 ] == 0 )
        {
            return;
        }

        Colour TextShadowColor  ( COLOR_BLACK );
        Colour LocalTextColor   ( TextColor );

        LocalTextColor.a  = (int)(Alpha * 0.8f);
        TextShadowColor.a = (int)(Alpha * 0.8f);

        if( bShadow )
        {
            IntRect Shadow;

            Shadow = iRect;
            Shadow.Translate( 0, 2 );
            g_UiMgr->RenderText( FontNum, Shadow, Flags, TextShadowColor, pLine, true,  false );

            Shadow = iRect;
            Shadow.Translate( -2, 0 );
            g_UiMgr->RenderText( FontNum, Shadow, Flags, TextShadowColor, pLine, true,  false );

            Shadow = iRect;
            Shadow.Translate( 2, 0 );
            g_UiMgr->RenderText( FontNum, Shadow, Flags, TextShadowColor, pLine, true,  false );

            Shadow = iRect;
            Shadow.Translate( 0, -2 );
            g_UiMgr->RenderText( FontNum, Shadow, Flags, TextShadowColor, pLine, true,  false );
        }

        g_UiMgr->RenderText( FontNum, iRect,  Flags, LocalTextColor,       pLine,     false, true );

    */
}

class flare_fader
{
public:
    void AdvanceLogic(float DeltaTime)
    {
        m_TimeLeft -= DeltaTime;
    };

    float GetFlare()
    {
        if (m_TimeLeft > (m_HoldTime + m_RampDownTime)) {
            return (m_TimeLeft - (m_HoldTime + m_RampDownTime)) / m_RampUpTime;
        } else if (m_TimeLeft > m_RampDownTime) {
            return (m_TimeLeft - m_RampDownTime) / m_HoldTime;
        } else if (m_TimeLeft > 0.0f) {
            return (m_TimeLeft / m_RampDownTime);
        }

        return 0.0f;
    }

    void SetFlare(float Time)
    {
        (void)Time;
    }

protected:
    float m_RampUpTime;
    float m_HoldTime;
    float m_RampDownTime;

    float m_TimeLeft;
};
