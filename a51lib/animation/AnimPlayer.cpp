
#include "animData.h"
#include "AnimPlayer.h"
//#include "e_ScratchMem.hpp"

#include <cassert>

simple_anim_player::simple_anim_player(ResourceManager* rm) : m_hGroup(rm)
{
    m_hGroup.setName("");
    m_iAnim = -1;
    m_Frame = 0;
    m_Cycle = 0;
    m_PrevFrame = 0;
    m_PrevCycle = 0;
    m_Rate = 1.0f;
    m_RootNodePos.set(0, 0, 0);
    m_L2W.Identity();
    m_StopAtEnd = false;
    m_bAtEnd = false;
    m_LoopDelay = 0;

    m_pCachedGroup = nullptr;
    m_iCachedAnim = -1;
    m_CachedFrame = -1;
    m_bCachedApplyBind = true;

    m_pTrackController[0] = nullptr;
    m_pTrackController[1] = nullptr;
    m_pTrackController[2] = nullptr;
    m_pTrackController[3] = nullptr;
}

//=========================================================================

simple_anim_player::~simple_anim_player()
{
}

//=========================================================================
void simple_anim_player::SetStopAtEnd(bool StopAtEnd)
{
    m_IsLooping = !StopAtEnd;
    m_StopAtEnd = StopAtEnd;
}

//=========================================================================

void simple_anim_player::SetAnimGroup(const AnimGroup::handle& hGroup)
{
    m_hGroup = hGroup;

    const AnimGroup* pGroup = GetAnimGroup();
    if (pGroup == NULL) {
        return;
    }

    m_iAnim = -1;
    m_nFrames = 0;
    m_Frame = 0;
    m_Cycle = 0;
    m_bAtEnd = false;
}

//=========================================================================

void simple_anim_player::SetLooping(bool Looping)
{
    m_IsLooping = Looping;
    m_StopAtEnd = !Looping;
}

//=========================================================================

void simple_anim_player::SetLoopDelay(float LoopDelay)
{
    m_LoopDelay = LoopDelay;
}

//=========================================================================

void simple_anim_player::SetSimpleAnim(int iAnim)
{
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);
    assert(iAnim != -1);
    SetAnim(iAnim, pGroup->GetAnimInfo(iAnim).DoesLoop());
}

//=========================================================================

void simple_anim_player::SetSimpleAnim(const char* pAnimName)
{
    assert(pAnimName);
    SetSimpleAnim(GetAnimIndex(pAnimName));
}

//=========================================================================

void simple_anim_player::SetAnim(int iAnim, bool IsLooping)
{
    m_IsLooping = IsLooping;
    m_LoopDelay = 0;

    if (iAnim == -1) {
        m_iAnim = -1;
        m_nFrames = 2;
        return;
    }

    // Lookup group
    const AnimGroup* pGroup = m_hGroup.getPointer();
    if (!pGroup) {
        return;
    }

    // Select random anim
    assert(pGroup);
    iAnim = pGroup->GetRandomAnimIndex(iAnim, m_iAnim);
    assert((iAnim >= 0) && (iAnim < pGroup->GetNAnims()));

    m_iAnim = iAnim;
    m_nFrames = pGroup->GetAnimInfo(m_iAnim).GetNFrames();
    m_Frame = 0;
    m_Cycle = 0;
    m_bAtEnd = false;
}

//=========================================================================

void simple_anim_player::SetAnim(const char* pAnimName,
                                 bool        IsLooping /*=true*/)
{
    assert(pAnimName);
    SetAnim(GetAnimIndex(pAnimName), IsLooping);
}

//=========================================================================

void simple_anim_player::SetTrackController(int iTrackController, track_controller* pTrackController)
{
    if ((iTrackController < 0) || (iTrackController >= 4)) {
        return;
    }

    m_pTrackController[iTrackController] = pTrackController;

    DirtyCachedL2W();
}

//=========================================================================

track_controller* simple_anim_player::GetTrackController(int iTrackController)
{
    if ((iTrackController < 0) || (iTrackController >= 4)) {
        return NULL;
    }

    return m_pTrackController[iTrackController];
}

//=========================================================================

void simple_anim_player::Advance(float nSeconds)
{
    // No animation?
    if (m_iAnim == -1) {
        return;
    }

    // No time?
    if (nSeconds == 0.0f) {
        return;
    }

    // No rate?
    if (m_Rate == 0.0f) {
        return;
    }

    //
    // Remember previous frame and cycle
    //
    m_PrevFrame = m_Frame;
    m_PrevCycle = m_Cycle;

    m_bAtEnd = false;

    //
    // Advance frame
    //
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);
    const AnimInfo& animInfo = pGroup->GetAnimInfo(m_iAnim);

    float nFramesToStep = m_Rate * nSeconds * (float)animInfo.GetFPS();
    float nFramesWithDelay = m_nFrames + m_LoopDelay * (float)animInfo.GetFPS();

    if (m_StopAtEnd && (m_Frame + nFramesToStep > (nFramesWithDelay - 2))) {
        m_Frame = nFramesWithDelay - 2;
        m_bAtEnd = true;
    } else if (m_StopAtEnd && ((m_Frame + nFramesToStep) < 0)) {
        m_Frame = 0;
        m_bAtEnd = true;
    } else {
        m_Frame += nFramesToStep;

        while (m_Frame > (nFramesWithDelay - 1)) {
            if (!m_IsLooping) {
                m_Frame = (float)(nFramesWithDelay - 1);
                m_Cycle = 0;
            } else {
                m_Frame -= (nFramesWithDelay - 1);
                m_Cycle++;
            }
        }

        while (m_Frame < 0) {
            if (!m_IsLooping) {
                m_Frame = 0;
                m_Cycle = 0;
            } else {
                m_Frame += (nFramesWithDelay - 1);
                m_Cycle--;
            }
        }
    }

    assert((m_Frame >= 0) && (m_Frame < nFramesWithDelay));

    // Disable loop blending?
    if ((m_Cycle != m_PrevCycle) && (animInfo.BlendLoop() == false)) {
        // Round frame to nearest integer frame
        int iFrame = (int)m_Frame;
        m_Frame = (float)iFrame;
    }
}

//=========================================================================

void simple_anim_player::SetL2W(const Matrix4& L2W)
{
    m_L2W = L2W;

    // Invalidate the cache
    DirtyCachedL2W();
}

//=========================================================================

float simple_anim_player::ComputeInterpFrame(void)
{
    // No anim?
    if (m_iAnim == -1) {
        return 0.0f;
    }

    // Get group
    const AnimGroup* pGroup = GetAnimGroup();
    if (!pGroup) {
        return 0.0f;
    }

    // Get anim info
    const AnimInfo& animInfo = pGroup->GetAnimInfo(m_iAnim);

    // Compute valid frame
    float Frame = m_Frame;
    if (Frame < 0) {
        Frame = 0;
    }
    if (Frame > (animInfo.GetNFrames() - 2)) {
        Frame = (float)(animInfo.GetNFrames() - 2);
    }

    // This is what anim_info::GetKeys does!
    Frame = fmod(Frame, (float)(animInfo.GetNFrames() - 1));

    // Disable frame blending?
    if (animInfo.BlendFrames() == false) {
        // Truncate to nearest integer frame
        int iFrame = (int)Frame;
        Frame = (float)iFrame;
    }

    return Frame;
}

//=========================================================================

void simple_anim_player::GetInterpKeys(AnimKey* pKey)
{
    int i;

    // Get group
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);

    // Get bone bount
    int nBones = pGroup->GetNBones();

    // Clear keys
    if (m_iAnim == -1) {
        for (i = 0; i < nBones; i++) {
            pKey[i].Identity();
        }

        return;
    }

    // Get anim info
    const AnimInfo& animInfo = pGroup->GetAnimInfo(m_iAnim);

    // Get keys
    float Frame = ComputeInterpFrame();
    animInfo.GetInterpKeys(Frame, pKey);
}

//=========================================================================

int simple_anim_player::GetNAnims()
{
    const AnimGroup* pGroup = GetAnimGroup();

    int nAnims = pGroup->GetNAnims();
    return nAnims;
}

//=========================================================================

int simple_anim_player::GetBoneIndex(const char* pBoneName, bool bFindAnywhere)
{
    // No anim group loaded?
    const AnimGroup* pGroup = GetAnimGroup();
    if (!pGroup) {
        return -1;
    }

    // Found bone?
    int iBone = pGroup->GetBoneIndex(pBoneName, bFindAnywhere);
    return iBone;
}

//=========================================================================

int simple_anim_player::GetNBones()
{
    const AnimGroup* pGroup = GetAnimGroup();

    assert(pGroup);

    int nBones = pGroup->GetNBones();
    return nBones;
}

//=========================================================================

int simple_anim_player::GetAnimIndex(const char* pAnimName)
{
    const AnimGroup* pGroup = GetAnimGroup();

    assert(pGroup);

    int iAnim = pGroup->GetAnimIndex(pAnimName);
    return iAnim;
}

//=========================================================================

int simple_anim_player::GetCollisionBone()
{
    const AnimGroup* pGroup = GetAnimGroup();
    if (!pGroup) {
        return -1;
    }
    int iBone = pGroup->GetBoneIndex("_coll", true);
    if (iBone == -1) {
        iBone = 0;
    }
    return iBone;
}

//=========================================================================

// Returns true if cache is valid
bool simple_anim_player::IsCachedL2WValid(bool bApplyTheBindPose)
{
    // Check if the actual cashe is dirty
    if (m_CachedL2W.IsDirty()) {
        return false;
    }

    // Lookup anim group
    const AnimGroup* pAnimGroup = GetAnimGroup();
    if (!pAnimGroup) {
        return false;
    }

    // Anim group mis-match?
    if (pAnimGroup != m_pCachedGroup) {
        return false;
    }

    // Matrices dirty?
    if (m_CachedL2W.IsValid(pAnimGroup->GetNBones()) == false) {
        return false;
    }

    // Animation index mis-match?
    if (m_iCachedAnim != m_iAnim) {
        return false;
    }

    // Frame mis-match?
    if (m_CachedFrame != ComputeInterpFrame()) {
        return false;
    }

    // Apply bind mis-match?
    if (m_bCachedApplyBind != bApplyTheBindPose) {
        return false;
    }

    // Cache is valid!
    return true;
}

//=========================================================================

// Flags cached matrices as dirty
void simple_anim_player::DirtyCachedL2W(void)
{
    // Clear cache vars
    m_CachedL2W.SetDirty(true);
    m_pCachedGroup = NULL;
    m_iCachedAnim = -1;
    m_CachedFrame = -1;
    m_bCachedApplyBind = true;
}

//=========================================================================

// Updates the cache if it's out of date
const Matrix4* simple_anim_player::UpdateCachedL2W(bool bApplyTheBindPose)
{
    // If valid, just use the cache
    if (IsCachedL2WValid(bApplyTheBindPose)) {
        return m_CachedL2W.GetMatrices();
    }

    // Clear incase of fail
    DirtyCachedL2W();

    // No anim?
    if (m_iAnim == -1) {
        return nullptr;
    }

    // Lookup group
    const AnimGroup* pGroup = GetAnimGroup();
    if (!pGroup) {
        return nullptr;
    }

    // Get cache matrices
    Matrix4* pMatrices = m_CachedL2W.GetMatrices(pGroup->GetNBones());
    if (!pMatrices) {
        return nullptr;
    }

    // Record cache values
    m_pCachedGroup = pGroup;
    m_iCachedAnim = m_iAnim;
    m_CachedFrame = ComputeInterpFrame();
    m_bCachedApplyBind = bApplyTheBindPose;

    // Allocate mix buffer
    AnimKey* MixBuffer = base_player::GetMixBuffer(base_player::MIX_BUFFER_PLAYER);
    assert(MixBuffer);

    // Grab all keys
    GetInterpKeys(MixBuffer);

    // Mix in any additional track controllers
    if (m_pTrackController[0]) {
        m_pTrackController[0]->MixKeys(MixBuffer);
    }
    if (m_pTrackController[1]) {
        m_pTrackController[1]->MixKeys(MixBuffer);
    }
    if (m_pTrackController[2]) {
        m_pTrackController[2]->MixKeys(MixBuffer);
    }
    if (m_pTrackController[3]) {
        m_pTrackController[4]->MixKeys(MixBuffer);
    }

    // Setup matrices
    pGroup->ComputeBonesL2W(m_L2W, MixBuffer, pGroup->GetNBones(), pMatrices, bApplyTheBindPose);

    // Keep root node pos
    m_RootNodePos = pMatrices[0].GetTranslation();

    // Flag cache is valid
    m_CachedL2W.SetDirty(false);

    // Return new matrices
    return pMatrices;
}

//=========================================================================

// Returns the cached L2W matrix for that bone
const Matrix4* simple_anim_player::GetBoneL2Ws(bool bApplyTheBindPose /*= true*/)
{
    // Compute bones
    return UpdateCachedL2W(bApplyTheBindPose);
}

//=========================================================================

// Returns cached L2W for particular bone
const Matrix4* simple_anim_player::GetBoneL2W(int iBone, bool bApplyTheBindPose /*= true*/)
{
    // Compute bones
    const Matrix4* pL2Ws = UpdateCachedL2W(bApplyTheBindPose);
    if (!pL2Ws) {
        return nullptr;
    }

    // Return specific bone
    return &pL2Ws[iBone];
}

//=========================================================================
bool simple_anim_player::AnimDone(void) const
{
    //    assert( !m_IsLooping ); // looping anims are never done
    //return ((m_nFrames - 1) - m_Frame) < 0.001f;
    return (!m_IsLooping) && ((m_bAtEnd) || (m_Frame >= (m_nFrames - 1))); // Handles forward or backward playing
}

//=========================================================================

float simple_anim_player::GetTimeLeft(void)
{
    const float      FramesLeft = (m_nFrames - 1) - m_Frame;
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);
    const AnimInfo& animInfo = pGroup->GetAnimInfo(m_iAnim);

    return FramesLeft / (float)animInfo.GetFPS();
}

//==============================================================================

float simple_anim_player::GetStartSpeed(float Time /*= 1.0f*/)
{
    assert(Time > 0);

    int              CurFrame = (int)m_Frame;
    const AnimGroup* pGroup = GetAnimGroup();
    const AnimInfo&  animInfo = pGroup->GetAnimInfo(m_iAnim);

    // Make sure we're not looking too far ahead
    Time = std::min(Time, (m_nFrames / (float)animInfo.GetFPS()));

    // find start position
    SetFrame(0);
    AnimKey animKey;
    GetInterpKeys(&animKey);
    Vector3 Start = animKey.translation;

    // find end position
    int EndFrame = (int)(Time * animInfo.GetFPS());
    if ((EndFrame == 0) && (m_nFrames > 0)) {
        // not enough time to get a frame
        EndFrame = 1;
        Time = (1.0f / animInfo.GetFPS());
    }

    SetFrame(EndFrame);
    GetInterpKeys(&animKey);
    Vector3 End = animKey.translation;

    SetFrame(CurFrame);

    return (Start - End).Length() / Time;
}

//==============================================================================

float simple_anim_player::GetEndSpeed(float Time /*= 1.0f*/)
{
    assert(Time > 0);

    int              CurFrame = (int)m_Frame;
    const AnimGroup* pGroup = GetAnimGroup();
    const AnimInfo&  animInfo = pGroup->GetAnimInfo(m_iAnim);

    // Make sure we're not looking too far ahead
    Time = std::min(Time, (m_nFrames / (float)animInfo.GetFPS()));

    // find end position
    const int LastFrame = m_nFrames - 2;
    SetFrame(LastFrame);
    AnimKey animKey;
    GetInterpKeys(&animKey);
    Vector3 End = animKey.translation;

    // find start position
    int StartFrame = (int)(LastFrame - (Time * animInfo.GetFPS()));

    if ((StartFrame == (LastFrame)) && (m_nFrames > 0)) {
        // not enough time to get a frame
        StartFrame = LastFrame - 1;
        Time = (1.0f / animInfo.GetFPS());
    }

    SetFrame(StartFrame);
    GetInterpKeys(&animKey);
    Vector3 Start = animKey.translation;

    SetFrame(CurFrame);

    return (Start - End).Length() / Time;
}

//=========================================================================

int simple_anim_player::GetEventIndex(const char* pEventName)
{
    AnimInfo animInfo = GetAnimInfo();
    int      i;
    for (i = 0; i < animInfo.GetNEvents(); ++i) {
        const anim_event& Event = animInfo.GetEvent(i);

        if (strcmp(pEventName, Event.GetName()) == 0) {
            return i;
        }
    }
    return -1;
}

//=========================================================================

Vector3 simple_anim_player::GetEventPosition(const anim_event& Event)
{
    // Compute matrices
    const Matrix4* pL2Ws = GetBoneL2Ws();
    if (!pL2Ws) {
        return Vector3(0, 0, 0);
    }

    // Compute world pos
    const Matrix4& BoneL2W = pL2Ws[Event.GetInt(anim_event::INT_IDX_BONE)];
    Vector3        P = BoneL2W * Event.GetPoint(anim_event::POINT_IDX_OFFSET);
    return P;
}

//=========================================================================

Radian3 simple_anim_player::GetEventRotation(const anim_event& Event)
{
    // Compute matrices
    const Matrix4* pL2Ws = GetBoneL2Ws();
    if (!pL2Ws) {
        return Radian3(0, 0, 0);
    }

    const Matrix4& BoneM = pL2Ws[Event.GetInt(anim_event::INT_IDX_BONE)];

    Vector3 ERot(Event.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());

    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rot = WorldRot.GetRotation();

    return Rot;
}

//=========================================================================

Vector3 simple_anim_player::GetEventPosition(int iEvent)
{
    // Compute matrices
    const Matrix4* pL2Ws = GetBoneL2Ws();
    if (!pL2Ws) {
        return Vector3(0, 0, 0);
    }

    // Lookup event
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);
    assert(m_iAnim != -1);
    const AnimInfo& animInfo = pGroup->GetAnimInfo(m_iAnim);
    assert((iEvent >= 0) && (iEvent < animInfo.GetNEvents()));
    const anim_event& Event = animInfo.GetEvent(iEvent);

    // Compute world pos
    const Matrix4& BoneL2W = pL2Ws[Event.GetInt(anim_event::INT_IDX_BONE)];
    Vector3        P = BoneL2W * Event.GetPoint(anim_event::POINT_IDX_OFFSET);
    return P;
}

//=========================================================================

Radian3 simple_anim_player::GetEventRotation(int iEvent)
{
    // Compute matrices
    const Matrix4* pL2Ws = GetBoneL2Ws();
    if (!pL2Ws) {
        return Radian3(0, 0, 0);
    }

    // Lookup event
    const AnimGroup* pGroup = GetAnimGroup();
    assert(pGroup);
    assert(m_iAnim != -1);
    const AnimInfo& animInfo = pGroup->GetAnimInfo(m_iAnim);
    assert((iEvent >= 0) && (iEvent < animInfo.GetNEvents()));
    const anim_event& EV = animInfo.GetEvent(iEvent);

    const Matrix4& BoneM = pL2Ws[EV.GetInt(anim_event::INT_IDX_BONE)];

    Vector3 ERot(EV.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());

    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rot = WorldRot.GetRotation();

    return Rot;
}

const Vector3 simple_anim_player::GetBoneBindPosition(int iBone)
{
    const AnimGroup* pAnimGroup = GetAnimGroup();
    if (NULL == pAnimGroup) {
        return Vector3(0, 0, 0);
    }

    const AnimBone& animBone = pAnimGroup->GetBone(iBone);

    return animBone.bindTranslation;
}

//=============================================================================

// Returns the combined bbox of all the current playing animations
BBox simple_anim_player::ComputeBBox() const
{
    // Lookup anim group and return default if not loaded
    const AnimGroup* pAnimGroup = m_hGroup.getPointer();
    if (pAnimGroup == nullptr) {
        // Use default bbox
        BBox bb;
        bb.Set(Vector3(0.0f, 0.0f, 0.0f), 100.0f);
        return bb;
    }

    // If no anim playing, then use bind pose (always present) bbox
    if (m_iAnim == -1) {
        const AnimInfo& animInfo = pAnimGroup->GetAnimInfo(0);
        return animInfo.GetBBox();
    }

    // Use bbox of current animation
    const AnimInfo& animInfo = pAnimGroup->GetAnimInfo(m_iAnim);
    return animInfo.GetBBox();
}
