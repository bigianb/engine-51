#pragma once

//#include "BasePlayer.hpp"
//#include "ResourceMgr\ResourceMgr.hpp"
#include "animData.h"

#define DEFAULT_BLEND_TIME (0.125f)

//=========================================================================

class track_controller
{

public:
    track_controller();
    virtual ~track_controller();

    // Sets location of animation data package
    virtual void SetAnimGroup(const AnimGroup::handle& hGroup) = 0;

    // Clears the animation to a safe unused state
    virtual void Clear() = 0;

    // Advances the current track by logic time
    virtual void Advance(float nSeconds) = 0;

    // Controls the influence this anim has during the mixing process
    virtual void  SetWeight(float ParametricWeight) = 0;
    virtual float GetWeight() = 0;

    // Returns the raw keyframe data
    virtual void GetInterpKeys(AnimKey* pKey) = 0;
    virtual void GetInterpKey(int iBone, AnimKey& Key) = 0;

    // Mixes the anims keyframes into the dest keyframes
    virtual void MixKeys(AnimKey* pDestKey) = 0;

    // Removes yaw from root node of turn animations
    virtual void SetRemoveTurnYaw(bool bRemove) { (void)bRemove; }
};

//=========================================================================

class anim_track_controller : public track_controller
{
public:
    enum mix_mode
    {
        MIX_BLENDED,
        MIX_ADDITIVE,
    };

public:
    anim_track_controller(ResourceManager *);
    virtual ~anim_track_controller();

    // Sets location of animation data package
    void SetAnimGroup(const AnimGroup::handle& AnimGroup);

    // Clears the animation to a safe unused state
    void Clear();

    // Gets keys for requested frame
    void GetInterpKeys(float Frame, AnimKey* pKey);

    // Sets a new animation and initializes the blend buffer
    void SetAnim(int iAnim, float BlendTime = DEFAULT_BLEND_TIME, bool ResetFrameCount = false);
    void SetAnim(const char* pAnimName, float BlendTime = DEFAULT_BLEND_TIME);

    // Advances the current animation
    void Advance(float nSeconds);

    // Overrides the current cursor time in the anim
    void  SetFrame(float Frame);
    void  SetCycle(int Cycle);
    float GetFrame();
    int   GetCycle();
    float GetFrameParametric();
    void  SetFrameParametric(float Frame);

    // Controls the influence this anim has during the mixing process
    void  SetWeight(float ParametricWeight);
    float GetWeight();

    // Returns the raw keyframe data
    void GetInterpKeys(AnimKey* pKey);
    void GetInterpKey(int iBone, AnimKey& Key);

    // Mixes the anims keyframes into the dest keyframes
    void SetMixMode(mix_mode Mode);

    void MixKeys(AnimKey* pDestKey);

    void SetAdditveRefFrame(int iRefFrame);
    int  GetAdditiveRefFrame();

    // Controls the playback rate of the anim, 1.0 = original speed
    void  SetRate(float PlaybackRateScale);
    float GetRate();

    // Returns the frame and cycle before the previous call to Advance
    float GetPrevFrame();
    int   GetPrevCycle();

    // Returns true if pegged at end or has looped past first cycle
    bool IsAtEnd();

    // Returns true if the current animation equals AnimName
    bool IsPlaying(const char* pAnimName);

    // Returns true if the current animations is a turn
    bool IsPlayingTurnAnim();

    // Returns true if the current animations is a transition
    bool IsPlayingTransitionAnim();

    // Returns info about animation package
    int              GetNAnims();
    const AnimGroup& GetAnimGroup();
    int              GetAnimIndex(const char* pAnimName);

    // Returns info on animation currently being played
    int             GetAnimIndex();
    const char*     GetAnimName();
    int             GetNFrames();
    const AnimInfo& GetAnimInfo();

    // Returns vector the root node traversed from start to end of anim
    Vector3 GetTotalTranslation();

    // Returns information about bones in skeleton
    int             GetNBones();
    const AnimBone& GetBone(int iBone);
    int             GetBoneIndex(const char* pBoneName);

    // Returns information on events in animation
    int               GetNEvents();
    const anim_event& GetEvent(int iEvent);
    bool              IsEventActive(int iEvent);
    bool              IsEventTypeActive(int Type);
    inline void       SetManualYaw(bool byaw) { m_bManualYaw = byaw; }
    void              SetOverrideRootBlend(bool bOverrideRootBlend);
    void              SetRemoveTurnYaw(bool bRemove);

protected:
    void BlendedMixKeys(AnimKey* pDestKey);
    void AdditiveMixKeys(AnimKey* pDestKey);

private:
    AnimGroup::handle m_hAnimGroup; // Group of anims we are using

    int   m_iAnim;     // Index of current anim
    int   m_nFrames;   // nFrames in animation
    float m_Frame;     // Current modulated frame
    int   m_Cycle;     // Current Cycle, 0,1,2,3
    float m_Weight;    // influence at mixing time
    float m_PrevFrame; // frame before Advance()
    int   m_PrevCycle; // cycle before Advance()
    float m_Rate;      // playback rate in frames per second

    AnimKey* m_pBlendKey;   // Stores keys we are blending from
    float    m_BlendLength; // Stores total time we are blending
    float    m_BlendFrame;  // Stores point in blending we are at
    bool     m_bManualYaw;
    bool     m_bPreviousManualYaw;
    bool     m_bOverrideRootBlend;
    bool     m_bRemoveTurnYaw; // Removes yaw from turn animations
    int      m_iRefFrame;      // Reference frame used for additive blending

    mix_mode m_MixMode;
};
