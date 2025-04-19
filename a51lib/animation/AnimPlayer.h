#pragma once

#include "../VectorMath.h"
#include "../dataUtil/Bitstream.h"
#include "animData.h"
#include "../resourceManager/ResourceManager.h"
#include "AnimTrack.h"
#include "SMemMatrixCache.h"
#include "BasePlayer.h"

#include <cassert>

class simple_anim_player : public base_player
{

    //-------------------------------------------------------------------------
public:
    simple_anim_player();
    virtual ~simple_anim_player();

    void SetAnimGroup(const AnimGroup::handle& hGroup);
    void SetSimpleAnim(int iAnim);
    void SetSimpleAnim(const char* pAnimName);
    void SetAnim(int iAnim, bool IsLooping = true);
    void SetAnim(const char* pAnimName, bool IsLooping = true);
    void SetStopAtEnd(bool StopAtEnd);
    void SetLooping(bool Looping);
    bool IsLooping() const { return m_IsLooping; }
    void SetLoopDelay(float LoopDelay);

    void           Advance(float nSeconds);
    const Vector3& GetRootNodePos() const;

    const Matrix4& GetL2W() const { return m_L2W; }
    void           SetL2W(const Matrix4& L2W);

    void              SetTrackController(int iTrackController, track_controller* pTrackController);
    track_controller* GetTrackController(int iTrackController);

private:
    float ComputeInterpFrame();
    void  GetInterpKeys(AnimKey* pKey);

    bool IsCachedL2WValid(bool bApplyTheBindPose);

    void           DirtyCachedL2W();
    const Matrix4* UpdateCachedL2W(bool bApplyTheBindPose);

public:
    // Returns the cached L2W matrix for that bone
    const Matrix4* GetBoneL2Ws(bool bApplyTheBindPose = true);

    const Matrix4* GetBoneL2W(int iBone, bool bApplyTheBindPose = true);

    const Vector3 GetBoneBindPosition(int iBone); // Bone bind position

    void SetFrame(float Frame);
    void SetFrame(int iFrame);
    void SetFrameParametric(float Frame);
    void SetTime(float Time);
    bool IsAtEnd() const;

    float GetFrame() const;
    float GetPrevFrame() const;
    int   GetCycle() const;
    int   GetPrevCycle() const;
    float GetRate() const;
    void  SetRate(float Rate);
    float GetFrameParametric();
    int   GetNFrames() const;
    int   GetAnimIndex() const;
    int   GetBoneIndex(const char* pBoneName, bool bFindAnywhere = false);
    int   GetNBones();
    int   GetNAnims();
    int   GetAnimIndex(const char* pAnimName);
    float GetTimeLeft();

    virtual int               GetNEvents();
    virtual bool              IsEventActive(int iEvent);
    virtual const anim_event& GetEvent(int iEvent);

    int GetCollisionBone();

    float GetStartSpeed(float Time = 1.0f);
    float GetEndSpeed(float Time = 1.0f);

    bool AnimDone() const;

    const AnimInfo& GetAnimInfo(); // Warning - leaves lock

    int                 GetEventIndex(const char* pEventName);
    virtual Vector3     GetEventPosition(int iEvent);
    virtual Radian3     GetEventRotation(int iEvent);
    virtual Vector3     GetEventPosition(const anim_event& Event);
    virtual Radian3     GetEventRotation(const anim_event& Event);
    virtual const char* GetAnimName() { return GetAnimInfo().GetName(); }

    const AnimGroup*        GetAnimGroup();
    const AnimGroup::handle GetAnimGroupHandle() { return m_hGroup; }

    // Returns the combined local space bbox of all the current playing animations
    BBox ComputeBBox() const;

    //-------------------------------------------------------------------------
private:
    AnimGroup::handle m_hGroup;      // Handle for group anim
    int               m_iAnim;       // Index of anim we are playing
    int               m_nFrames;     // # of frames in animation
    float             m_Frame;       // Frame number
    int               m_Cycle;       // Cycle number
    float             m_PrevFrame;   // Previous Frame number
    int               m_PrevCycle;   // Previous Cycle number
    float             m_Rate;        // Playback rate (defaults to 1)
    Matrix4           m_L2W;         // Local -> world matrix
    bool              m_IsLooping;   // Loop flag
    bool              m_StopAtEnd;   // Stop anim after 1 cycle flag
    bool              m_bAtEnd;      // true if anim has played 1 cycle
    Vector3           m_RootNodePos; // Position of root node
    float             m_LoopDelay;   // Loop delay when reaching end of anim

    smem_matrix_cache m_CachedL2W;           // Cached matrices
    const AnimGroup*  m_pCachedGroup;        // Anim group currently cached
    int               m_iCachedAnim;         // Index of animation that cached
    float             m_CachedFrame;         // Current frame that is cached
    bool              m_bCachedApplyBind;    // Apply bind cached value
    track_controller* m_pTrackController[4]; // Pointer to last used track controllers

};

//=========================================================================
// INLINES
//=========================================================================

inline const AnimGroup* simple_anim_player::GetAnimGroup()
{
    return m_hGroup.getPointer();
}

//=========================================================================

inline const AnimInfo& simple_anim_player::GetAnimInfo()
{
    // Lookup group
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);

    // Valid anim?
    assert(m_iAnim >= 0);
    assert(m_iAnim < pGroup->GetNAnims());

    // Lookup anim
    const AnimInfo& AnimInfo = pGroup->GetAnimInfo(m_iAnim);
    return AnimInfo;
}

//=========================================================================

inline const Vector3& simple_anim_player::GetRootNodePos() const
{
    return m_RootNodePos;
}

//=========================================================================

inline void simple_anim_player::SetFrameParametric(float Frame)
{
    // Range check
    if (Frame < 0) {
        Frame = 0;
    } else if (Frame > 1) {
        Frame = 1;
    }

    // NOTE: The -2 is because the last 2 frames are the same - they are used
    //       to calculate a correct delta for the positions eg. looping run anim
    //       DO NOT CHANGE THIS. If you use -1, then an incorrect delta gets calculated
    //       upon the loop of the animation, resulting in a position pop!

    // Set
    m_Frame = Frame * ((float)m_nFrames - 2);
}

//=========================================================================

inline float simple_anim_player::GetFrameParametric()
{
    // NOTE: The -2 is because the last 2 frames are the same - they are used
    //       to calculate a correct delta for the positions eg. looping run anim
    //       DO NOT CHANGE THIS. If you use -1, then an incorrect delta gets calculated
    //       upon the loop of the animation, resulting in a position pop!

    float Frame = m_Frame / (m_nFrames - 2);

    // Range check
    if (Frame < 0) {
        Frame = 0;
    } else if (Frame > 1) {
        Frame = 1;
    }

    return Frame;
}

//=========================================================================

inline bool simple_anim_player::IsAtEnd() const
{
    return ((m_Cycle > 0) || (m_Frame >= (m_nFrames - 2)));
}

//=========================================================================

inline void simple_anim_player::SetFrame(float Frame)
{
    assert(Frame >= 0);
    assert(Frame < m_nFrames);

    m_Frame = Frame;
    m_Cycle = 0;
    m_bAtEnd = false;
}

//=========================================================================

inline void simple_anim_player::SetFrame(int iFrame)
{
    assert(iFrame >= 0);
    assert(iFrame < m_nFrames);

    m_Frame = (float)iFrame;
    m_Cycle = 0;
    m_bAtEnd = false;
}

//=========================================================================

inline void simple_anim_player::SetTime(float Time)
{
    // No anim?
    if (m_iAnim == -1) {
        return;
    }

    // Lookup anim info
    const AnimInfo& AnimInfo = GetAnimInfo();

    // Compute frame from time
    float Frame = Time * AnimInfo.GetFPS();

    // Peg at end (-2 is because the last 2 frames are the same from the exporter)
    float EndFrame = (float)(AnimInfo.GetNFrames() - 2);
    if (Frame > EndFrame) {
        Frame = EndFrame;
    }

    // Peg at beginning
    if (Frame < 0.0f) {
        Frame = 0.0f;
    }

    // Record so events fire off correctly
    m_PrevFrame = m_Frame;

    // Set it
    SetFrame(Frame);
}

inline float simple_anim_player::GetFrame() const
{
    return m_Frame;
}

inline float simple_anim_player::GetPrevFrame() const
{
    return m_PrevFrame;
}

inline int simple_anim_player::GetCycle() const
{
    return m_Cycle;
}

inline int simple_anim_player::GetPrevCycle() const
{
    return m_PrevCycle;
}

inline float simple_anim_player::GetRate() const
{
    return m_Rate;
}

inline void simple_anim_player::SetRate(float Rate)
{
    m_Rate = Rate;
}

inline int simple_anim_player::GetNFrames() const
{
    return m_nFrames;
}

inline int simple_anim_player::GetAnimIndex() const
{
    return m_iAnim;
}

inline int simple_anim_player::GetNEvents()
{
    return GetAnimInfo().GetNEvents();
}

inline bool simple_anim_player::IsEventActive(int iEvent)
{
    return GetAnimInfo().IsEventActive(iEvent, m_Frame, m_PrevFrame);
}

inline const anim_event& simple_anim_player::GetEvent(int iEvent)
{
    return GetAnimInfo().GetEvent(iEvent);
}
