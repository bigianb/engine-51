
#include "AnimTrack.h"
#include "BasePlayer.h"
#include "../render/Geom.h"
#include "../xfiles/x_plus.h"

#include <cassert>

#define INTERP_TEST InterpolateSRT

track_controller::track_controller()
{
}

track_controller::~track_controller()
{
}

anim_track_controller::anim_track_controller(ResourceManager* rm) : m_hAnimGroup(rm)
{
    m_pBlendKey = nullptr;
    Clear();
}

//=========================================================================

anim_track_controller::~anim_track_controller()
{
    Clear();
    delete[] m_pBlendKey;
}

//=========================================================================

void anim_track_controller::Clear()
{
    m_iAnim = -1;
    m_nFrames = 0;
    m_Frame = 0;
    m_Cycle = 0;
    m_Weight = 0;
    m_PrevFrame = 0;
    m_PrevCycle = 0;
    m_Rate = 1.0f;
    m_BlendLength = 0;
    m_BlendFrame = 0;
    m_bPreviousManualYaw = false;
    m_bRemoveTurnYaw = false;
    m_iRefFrame = 0;
    m_MixMode = MIX_BLENDED;
}

//=========================================================================

void anim_track_controller::SetOverrideRootBlend(bool bOverrideRootBlend)
{
    m_bOverrideRootBlend = bOverrideRootBlend;
}

//=========================================================================

void anim_track_controller::SetRemoveTurnYaw(bool bRemove)
{
    m_bRemoveTurnYaw = bRemove;
}

//=========================================================================

void anim_track_controller::SetAnimGroup(const AnimGroup::handle& animGroup)
{
    delete[] m_pBlendKey;

    m_hAnimGroup = animGroup;
    const AnimGroup& AG = GetAnimGroup();

    assert(AG.numBones <= MAX_ANIM_BONES);

    m_pBlendKey = new AnimKey[AG.GetNBones()];
    assert(m_pBlendKey);

    Clear();
}

//=========================================================================

void anim_track_controller::GetInterpKeys(float Frame, AnimKey* pKey)
{
    // Lookup anim data
    const AnimInfo& AnimData = GetAnimInfo();

    // Grab the keys
    AnimData.GetInterpKeys(Frame, pKey);

    // Remove yaw from root anim?
    if ((m_bRemoveTurnYaw) && (IsPlayingTurnAnim())) {
        // Remove yaw from root node key
        Radian RootYaw = pKey[0].rotation.GetRotation().yaw;
        pKey[0].rotation *= Quaternion(Vector3(0, 1, 0), -RootYaw);
    }
}

//=========================================================================

void anim_track_controller::SetAnim(int iAnim, float BlendTime, bool ResetFrameCount)
{
    assert((iAnim >= 0) && (iAnim < GetAnimGroup().GetNAnims()));

    // If we are already playing this anim, don't reset
    if (iAnim == m_iAnim) {
        // If the animation is looping and we are pegged at the end then restart
        if (!(GetAnimInfo().DoesLoop() && IsAtEnd())) {
            if (ResetFrameCount) {
                m_Frame = 0;
                m_Cycle = 0;
            }
            return;
        }
    }

    // Check for using a specified blend time
    if (iAnim != -1) {
        // Lookup anim info
        const AnimGroup& AnimGroup = GetAnimGroup();
        const AnimInfo&  AnimInfo = AnimGroup.GetAnimInfo(iAnim);

        // Use blend from animation if it's specified
        if (AnimInfo.GetBlendTime() >= 0.0f) {
            BlendTime = AnimInfo.GetBlendTime();
        }
    }

    // Allocate mix buffer
    AnimKey* MixBuffer = base_player::GetMixBuffer(base_player::MIX_BUFFER_CONTROLLER);
    assert(MixBuffer);

    // Setup blend information
    if ((m_iAnim != -1) && (iAnim != m_iAnim)) {
        // If we care about blending then prepare blend keys
        if (BlendTime > 0) {

            // If blending has not finished from previous animation then we
            // need to combine the blend keys and the current keys back into
            // the blend keys and use that as our 'previous' animation
            if (m_BlendLength > 0) {
                float T = m_BlendFrame / m_BlendLength;
                int   nBones = GetAnimGroup().GetNBones();
                GetInterpKeys(m_Frame, MixBuffer);
                for (int i = 0; i < nBones; i++) {
                    m_pBlendKey[i].Interpolate(m_pBlendKey[i], MixBuffer[i], T);
                }
            } else {
                GetInterpKeys(m_Frame, m_pBlendKey);
            }
            if (m_bPreviousManualYaw || m_bOverrideRootBlend) {
                AnimKey        Key;
                const AnimInfo AnimData = GetAnimGroup().GetAnimInfo(iAnim);
                AnimData.GetRawKey(0, 0, Key);
                m_pBlendKey[0].rotation = Key.rotation;
            }
        }

        m_BlendLength = BlendTime;
        m_BlendFrame = 0.0f;
    }

    m_iAnim = iAnim;
    m_nFrames = GetAnimInfo().GetNFrames();
    m_Frame = 0;
    m_Cycle = 0;
    m_PrevFrame = 0;
    m_PrevCycle = 0;
    m_bPreviousManualYaw = m_bManualYaw;
    m_bOverrideRootBlend = false;
}

//=========================================================================

void anim_track_controller::SetAnim(const char* pAnimName, float BlendTime)
{
    SetAnim(GetAnimIndex(pAnimName), BlendTime);
}

//=========================================================================

void anim_track_controller::Advance(float nSeconds)
{
    if (m_iAnim == -1) {
        return;
    }

    //
    // Remember previous frame and cycle
    //
    m_PrevFrame = m_Frame;
    m_PrevCycle = m_Cycle;

    //
    // Count down blend time
    //
    m_BlendFrame += abs(nSeconds);
    if (m_BlendFrame >= m_BlendLength) {
        m_BlendFrame = 0.0f;
        m_BlendLength = 0.0f;
    }

    //
    // Advance frame
    //
    float nFrames = nSeconds * (float)GetAnimInfo().GetFPS() * m_Rate;
    m_Frame += nFrames;

    // Update which cycle we are in and modulate the frame
    while (m_Frame >= (m_nFrames - 1)) {
        m_Frame -= (m_nFrames - 1);
        m_Cycle++;
    }

    // Lookup anim data
    const AnimInfo& AnimData = GetAnimInfo();

    // Get looping flag
    bool DoesLoop = AnimData.DoesLoop();

    // If the anim doesn't loop and we are past the end then peg at the end
    if ((!DoesLoop) && ((m_Cycle > 0) || (m_Frame >= (m_nFrames - 2)))) {
        m_Cycle = 0;
        m_Frame = (float)(m_nFrames - 2);
    }
}

//=========================================================================

void anim_track_controller::GetInterpKeys(AnimKey* pKey)
{
    int i;
    int nBones = GetAnimGroup().GetNBones();

    // Clear keys if no animation
    if (m_iAnim == -1) {
        for (i = 0; i < nBones; i++) {
            pKey[i].Identity();
        }

        return;
    }

    // Read interpolated keys from the animation
    GetInterpKeys(m_Frame, pKey);

    // Blend with previous anim exit keyframes
    if (m_BlendLength > 0.0f) {
        float T = m_BlendFrame / m_BlendLength;
        for (i = 0; i < nBones; i++) {
            pKey[i].Interpolate(m_pBlendKey[i], pKey[i], T);
        }
    }
}

//=========================================================================

void anim_track_controller::GetInterpKey(int iBone, AnimKey& Key)
{
    // Clear keys if no animation
    if (m_iAnim == -1) {
        Key.Identity();
        return;
    }

    // Read interpolated keys from the animation
    GetAnimInfo().GetInterpKey(m_Frame, iBone, Key);

    // Blend with previous anim exit keyframes
    if (m_BlendLength > 0.0f) {
        float T = m_BlendFrame / m_BlendLength;
        Key.Interpolate(m_pBlendKey[iBone], Key, T);
    }
}

const AnimGroup& anim_track_controller::GetAnimGroup()
{
    AnimGroup* pGroup = m_hAnimGroup.getPointer();
    assert(pGroup);
    return *pGroup;
}

//=========================================================================

bool anim_track_controller::IsAtEnd()
{
    return ((m_Cycle > 0) || (m_Frame >= (m_nFrames - 2)));
}

//=========================================================================

void anim_track_controller::SetFrame(float Frame)
{
    m_Frame = fmod(Frame, (float)m_nFrames - 1);
}

//=========================================================================

void anim_track_controller::SetCycle(int Cycle)
{
    m_Cycle = Cycle;
}

//=========================================================================

void anim_track_controller::SetRate(float Rate)
{
    m_Rate = Rate;
}

//=========================================================================

float anim_track_controller::GetRate()
{
    return m_Rate;
}

//=========================================================================

void anim_track_controller::SetWeight(float Weight)
{
    m_Weight = Weight;
    m_Weight = std::min(1.0f, m_Weight);
    m_Weight = std::max(0.0f, m_Weight);
}

//=========================================================================

float anim_track_controller::GetWeight()
{
    return m_Weight;
}

//=========================================================================

float anim_track_controller::GetFrame()
{
    return m_Frame;
}

//=========================================================================

int anim_track_controller::GetCycle()
{
    return m_Cycle;
}

//=========================================================================

float anim_track_controller::GetFrameParametric()
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

void anim_track_controller::SetFrameParametric(float Frame)
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

float anim_track_controller::GetPrevFrame()
{
    return m_PrevFrame;
}

//=========================================================================

int anim_track_controller::GetPrevCycle()
{
    return m_PrevCycle;
}

//=========================================================================

int anim_track_controller::GetNFrames()
{
    return m_nFrames;
}

//=========================================================================

int anim_track_controller::GetAnimIndex()
{
    return m_iAnim;
}

//=========================================================================

int anim_track_controller::GetNAnims()
{
    return GetAnimGroup().GetNAnims();
}

//=========================================================================

int anim_track_controller::GetNBones()
{
    return GetAnimGroup().GetNBones();
}

//=========================================================================

int anim_track_controller::GetAnimIndex(const char* pAnimName)
{
    return GetAnimGroup().GetAnimIndex(pAnimName);
}

//=========================================================================

bool anim_track_controller::IsPlaying(const char* pAnimName)
{
    if (m_iAnim == -1) {
        return false;
    }

    if (m_iAnim == GetAnimIndex(pAnimName)) {
        return true;
    }

    return false;
}

//=========================================================================

bool anim_track_controller::IsPlayingTurnAnim()
{
    if (m_iAnim == -1) {
        return false;
    }

    // Lookup anim data
    const AnimInfo& AnimData = GetAnimInfo();

    // Is this a turn animation?
    bool bIsTurn = (x_stristr(AnimData.GetName(), "turn") != nullptr) || (x_stristr(AnimData.GetName(), "blend") != nullptr);

    return bIsTurn;
}

//=========================================================================

// Returns true if the current animations is a transition
bool anim_track_controller::IsPlayingTransitionAnim()
{
    if (m_iAnim == -1) {
        return false;
    }

    // Lookup anim data
    const AnimInfo& AnimData = GetAnimInfo();

    // Is this a transition animation?
    bool bIsTransition = (x_stristr(AnimData.GetName(), "blend") != nullptr);

    return bIsTransition;
}

//=========================================================================

Vector3 anim_track_controller::GetTotalTranslation()
{
    return GetAnimInfo().GetTotalTranslation();
}

//=========================================================================

const AnimInfo& anim_track_controller::GetAnimInfo()
{
    return GetAnimGroup().GetAnimInfo(m_iAnim);
}

//=========================================================================

int anim_track_controller::GetBoneIndex(const char* pBoneName)
{
    return GetAnimGroup().GetBoneIndex(pBoneName);
}

//=========================================================================

const AnimBone& anim_track_controller::GetBone(int iBone)
{
    return GetAnimGroup().GetBone(iBone);
}

//=========================================================================

void anim_track_controller::SetAdditveRefFrame(int iRefFrame)
{
    m_iRefFrame = iRefFrame;
}

//=========================================================================

int anim_track_controller::GetAdditiveRefFrame()
{
    return m_iRefFrame;
}

//=========================================================================

void anim_track_controller::SetMixMode(mix_mode Mode)
{
    m_MixMode = Mode;
}

int anim_track_controller::GetNEvents()
{
    if (m_iAnim == -1) {
        return 0;
    }

    return GetAnimInfo().GetNEvents();
}

const anim_event& anim_track_controller::GetEvent(int iEvent)
{
    return GetAnimInfo().GetEvent(iEvent);
}

bool anim_track_controller::IsEventActive(int iEvent)
{
    return GetAnimInfo().IsEventActive(iEvent, m_Frame, m_PrevFrame);
}

bool anim_track_controller::IsEventTypeActive(int Type)
{
    return GetAnimInfo().IsEventTypeActive(Type, m_Frame, m_PrevFrame);
}

void anim_track_controller::MixKeys(AnimKey* pDestKey)
{
    switch (m_MixMode) {
    case MIX_BLENDED:
        BlendedMixKeys(pDestKey);
        break;
    case MIX_ADDITIVE:
        AdditiveMixKeys(pDestKey);
        break;
    }
}

void anim_track_controller::BlendedMixKeys(AnimKey* pDestKey)
{
    // If we aren't playing anything then just return
    if ((m_iAnim == -1) || (m_Weight == 0.0f)) {
        return;
    }

    int i;
    int nBones = GetAnimGroup().GetNBones();

    // Allocate mix buffer
    AnimKey* MixBuffer = base_player::GetMixBuffer(base_player::MIX_BUFFER_CONTROLLER);
    assert(MixBuffer);

    // Read interpolated keys from the animation
    GetInterpKeys(m_Frame, MixBuffer);

    // Blend with previous anim exit keyframes
    if (m_BlendLength > 0.0f) {
        float T = m_BlendFrame / m_BlendLength;
        for (i = 0; i < nBones; i++) {
            MixBuffer[i].Interpolate(m_pBlendKey[i], MixBuffer[i], T);
        }
    }

    // Check if this animation has bone masks
    if (GetAnimInfo().HasMasks()) {
        // Blend destination into track keys by weight amount
        for (i = 0; i < nBones; i++) {
            if (!GetAnimInfo().IsBoneMasked(i)) {
                pDestKey[i].Interpolate(pDestKey[i], MixBuffer[i], m_Weight);
            }
        }
    } else {
        // Blend destination into track keys by weight amount
        for (i = 0; i < nBones; i++) {
            pDestKey[i].Interpolate(pDestKey[i], MixBuffer[i], m_Weight);
        }
    }
}

//=========================================================================

void anim_track_controller::AdditiveMixKeys(AnimKey* pDestKey)
{
    // If we aren't playing anything then just return
    if ((m_iAnim == -1) || (m_Weight == 0.0f)) {
        return;
    }

    // Bail on bad param
    if (nullptr == pDestKey) {
        return;
    }

    // Get anim group
    const AnimGroup& AnimGroup = GetAnimGroup();

    // Lookup anim info
    const AnimInfo& AnimInfo = AnimGroup.GetAnimInfo(m_iAnim);

    // Loop over all bones and mix keys
    int NBones = AnimGroup.GetNBones();

    for (int i = 0; i < NBones; i++) {
        // Skip if masked
        if (AnimInfo.IsBoneMasked(i)) {
            continue;
        }

        // Read reference key
        AnimKey InvRefKey;
        AnimInfo.GetRawKey(m_iRefFrame, i, InvRefKey);

        // Lookup current key
        AnimKey CurrKey;
        AnimInfo.GetInterpKey(m_Frame, i, CurrKey);

        // Eyely applies the difference between reference frame and the current frame as follows:
        //
        // Variables:   CurrRot,   CurrPos   = Current key frame
        //              AnimRot,   AnimPos   = Animation key frame
        //              InvRefRot, InvRefPos = Inverse key frame of reference frame
        //
        //  Apply delta to current keys
        //          CurrRot = AnimRot * InvRefRot * CurrRot;
        //          CurrPos = AnimPos + InvRefPos + CurrPos;
        //
        //

        // Get inverse of reference key
        InvRefKey.rotation.Invert();
        InvRefKey.translation = -InvRefKey.translation;

        // Compute eye key
        AnimKey AddKey;
        AddKey.rotation = CurrKey.rotation * InvRefKey.rotation;
        AddKey.translation = CurrKey.translation + InvRefKey.translation;

        // Weighted?
        if (m_Weight != 1.0f) {
            AddKey.rotation = BlendToIdentity(AddKey.rotation, 1.0f - m_Weight);
            AddKey.translation *= m_Weight;
        }

        // Add delta on top of current keys
        pDestKey[i].rotation = AddKey.rotation * pDestKey[i].rotation;
        pDestKey[i].translation = AddKey.translation + pDestKey[i].translation;
    }
}
